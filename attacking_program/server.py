import socket
import threading
import queue
import time
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend


# Handles encryption and decryption using AES-CBC with PKCS7 padding
class CryptoHandler:
    def __init__(self, key: bytes, iv: bytes):
        if len(key) != 16 or len(iv) != 16:
            raise ValueError("The key and IV must be exactly 16 bytes.")
        self.key = key
        self.iv = iv

    def encrypt(self, plaintext: str | bytes) -> bytes:
        if isinstance(plaintext, str):
            plaintext = plaintext.encode('utf-8')
        padder = padding.PKCS7(algorithms.AES.block_size).padder()
        padded_data = padder.update(plaintext) + padder.finalize()
        cipher = Cipher(algorithms.AES(self.key), modes.CBC(self.iv), backend=default_backend())
        encryptor = cipher.encryptor()
        return encryptor.update(padded_data) + encryptor.finalize()

    def decrypt(self, ciphertext: bytes) -> str:
        cipher = Cipher(algorithms.AES(self.key), modes.CBC(self.iv), backend=default_backend())
        decryptor = cipher.decryptor()
        padded_data = decryptor.update(ciphertext) + decryptor.finalize()
        unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
        data = unpadder.update(padded_data) + unpadder.finalize()
        return data.decode('utf-8', errors='ignore')


# Manages secure network communication using encrypted messages and chunked transmission
class SecureNetworkHandler:
    def __init__(self, crypto: CryptoHandler, buffer_size: int = 1024):
        self.crypto = crypto
        self.buffer_size = buffer_size
        self.header_size = 5  # Number of bytes reserved for header metadata
        self.max_chunk_body_size = buffer_size - self.header_size  # Actual payload size per chunk

    def send(self, sock: socket.socket, data: str | bytes) -> bool:
        try:
            encrypted = self.crypto.encrypt(data)
            total_len = len(encrypted)
            nb_chunks = (total_len + self.max_chunk_body_size - 1) // self.max_chunk_body_size

            # Send message in multiple chunks, each with header and EOT flag
            for i in range(nb_chunks):
                chunk_data = encrypted[i * self.max_chunk_body_size:(i + 1) * self.max_chunk_body_size]
                chunk = bytearray(self.buffer_size)
                chunk[0] = nb_chunks      # Total number of chunks
                chunk[1] = i              # Current chunk index
                chunk[2] = (len(chunk_data) >> 8) & 0xFF  # Length high byte
                chunk[3] = len(chunk_data) & 0xFF         # Length low byte
                chunk[4:4 + len(chunk_data)] = chunk_data
                chunk[4 + len(chunk_data)] = 0x04         # End-of-Transmission marker
                sock.sendall(chunk)

            return True
        except Exception as e:
            print(f"[SEND ERROR] {e}")
            return False

    def receive(self, sock: socket.socket) -> str | bool:
        buffer = bytearray()
        received_chunks = []
        nb_chunks_needed = 0

        try:
            while True:
                chunk = sock.recv(self.buffer_size)
                if not chunk:
                    print("[RECEIVE ERROR] Socket closed")
                    return False

                total_chunks = chunk[0]
                chunk_index = chunk[1]
                chunk_len = (chunk[2] << 8) | chunk[3]
                chunk_data = chunk[4:4 + chunk_len]

                # Validate end-of-transmission marker
                if chunk[4 + chunk_len] != 0x04:
                    print(f"[RECEIVE ERROR] Missing EOT for chunk {chunk_index}")
                    return False

                # Initialize chunk tracking on first chunk
                if nb_chunks_needed == 0:
                    nb_chunks_needed = total_chunks
                    received_chunks = [False] * nb_chunks_needed

                if received_chunks[chunk_index]:
                    print(f"[RECEIVE ERROR] Duplicate chunk {chunk_index}")
                    return False

                received_chunks[chunk_index] = True
                buffer.extend(chunk_data)

                # All chunks received
                if all(received_chunks):
                    break

            return self.crypto.decrypt(bytes(buffer))
        except Exception as e:
            print(f"[RECEIVE EXCEPTION] {e}")
            return False


# TCP server that handles one client at a time with encrypted communication
class RobustTCPServer:
    def __init__(self, host='0.0.0.0', port=12345, crypto=None):
        self.host = host
        self.port = port
        self.server_socket = None
        self.client_socket = None
        self.client_ip = None
        self.running = False

        # Queues for thread-safe communication between server and client
        self.send_queue = queue.Queue()
        self.recv_queue = queue.Queue()
        self.ip_lock = threading.Lock()
        self.server_thread = None

        self.crypto = crypto or CryptoHandler(
            key=b'1234567890abcdef',
            iv=b'abcdef1234567890'
        )
        self.network_handler = SecureNetworkHandler(self.crypto)

    def start(self):
        # Start listening for client connections
        self.running = True
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(1)
        print(f"[SERVER] Listening on {self.host}:{self.port}")
        self.server_thread = threading.Thread(target=self._main_loop, daemon=True)
        self.server_thread.start()

    def _main_loop(self):
        while self.running:
            try:
                print("[SERVER] Waiting for connection...")
                self.client_socket, client_addr = self.server_socket.accept()
                with self.ip_lock:
                    self.client_ip = client_addr[0]
                print(f"[CONNECTED] Client IP: {self.client_ip}")

                # Start threads for receiving and sending
                recv_thread = threading.Thread(target=self._handle_receive, daemon=True)
                send_thread = threading.Thread(target=self._handle_send, daemon=True)
                recv_thread.start()
                send_thread.start()

                # Wait until both threads finish
                recv_thread.join()
                self.send_queue.put(None)  # Signal send thread to stop
                send_thread.join()

                # Clean up after client disconnect
                with self.ip_lock:
                    self.client_ip = None
                self.client_socket = None
                print("[SERVER] Client disconnected.")
            except Exception as e:
                print(f"[MAIN LOOP ERROR] {e}")

    def _handle_receive(self):
        # Thread function to continuously receive messages
        try:
            while True:
                message = self.network_handler.receive(self.client_socket)
                if message is False:
                    break
                self.recv_queue.put(message)
                print(f"[RECEIVED] {message}")
        except Exception as e:
            print(f"[RECEIVE ERROR] {e}")
        finally:
            if self.client_socket:
                self.client_socket.close()

    def _handle_send(self):
        # Thread function to continuously send messages from the queue
        try:
            while True:
                msg = self.send_queue.get()
                if msg is None:
                    break
                if not self.network_handler.send(self.client_socket, msg):
                    break
                print(f"[SENT] {msg}")
        except Exception as e:
            print(f"[SEND ERROR] {e}")
        finally:
            if self.client_socket:
                self.client_socket.close()

    # Public method to send a message to the connected client
    def send_to_client(self, message):
        self.send_queue.put(message)

    # Retrieve a message from the client if available
    def receive_from_client(self):
        try:
            return self.recv_queue.get_nowait()
        except queue.Empty:
            return None

    # Return the current client IP address
    def get_connected_ip(self):
        with self.ip_lock:
            return self.client_ip

    # Stop the server gracefully
    def stop(self):
        self.running = False
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
        with self.ip_lock:
            self.client_ip = None
        print("[SERVER] Stopped cleanly.")

    # Check if the server is running
    def is_running(self):
        return self.running

    # Check if a client is currently connected
    def is_client_connected(self):
        with self.ip_lock:
            return self.client_ip is not None


# Entry point: start server and wait for user input if a client is connected
if __name__ == "__main__":
    server = RobustTCPServer(port=4242)
    server.start()

    try:
        while True:
            # Only allow input when client is connected
            if server.is_client_connected():
                command = input(">> ").strip()
                if command:
                    server.send_to_client(command)

            time.sleep(0.1)
    except KeyboardInterrupt:
        print("[APP] Manual stop")
        server.stop()
