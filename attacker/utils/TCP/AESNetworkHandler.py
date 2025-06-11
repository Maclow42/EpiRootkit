from utils.TCP.TCPServer import CryptoHandler
import socket

# 10 bytes now for header :  
# [0..3] = total_chunks (32 bits big-endian)  
# [4..7] = chunk_index  (32 bits big-endian)  
# [8..9] = chunk_len    (16 bits big-endian)  


"""
@class AESNetworkHandler
@brief Handles AES-encrypted network communication over TCP sockets.
This class provides methods to send and receive data over a TCP socket using AES encryption.
Data is split into fixed-size chunks with headers and an end-of-transmission marker to ensure
reliable and secure transmission.
"""
class AESNetworkHandler:
    """
    @brief Initializes the AESNetworkHandler.
    @param crypto An instance of CryptoHandler used for encryption and decryption.
    @param buffer_size The size of the buffer for each chunk (default: 1024 bytes).
    """
    def __init__(self, crypto: CryptoHandler, buffer_size: int=1024):
        self._crypto = crypto
        self._buffer_size = buffer_size
        self._header_size = 10
        self._chunk_overhead = self._header_size + 1
        self._max_chunk_body_size = buffer_size - self._chunk_overhead

    """
    @brief Encrypts and sends data over a TCP socket in fixed-size chunks.
    The data is encrypted using the provided CryptoHandler, split into chunks,
    and each chunk is sent with a header containing metadata and an EOT marker.
    @param sock The TCP socket to send data through.
    @param data The data to send (string or bytes).
    @return True if the data was sent successfully, False otherwise.
    """
    def send(self, sock: socket.socket, data: str | bytes) -> bool:
        try:
            encrypted = self._crypto.encrypt(data)
            total_len = len(encrypted)
            nb_chunks = (total_len + self._max_chunk_body_size - 1) // self._max_chunk_body_size

            for i in range(nb_chunks):
                chunk_data = encrypted[i * self._max_chunk_body_size : (i + 1) * self._max_chunk_body_size]
                
                chunk = bytearray(self._buffer_size)
                chunk[0] = (nb_chunks >> 24) & 0xFF
                chunk[1] = (nb_chunks >> 16) & 0xFF
                chunk[2] = (nb_chunks >> 8) & 0xFF
                chunk[3] = (nb_chunks >> 0) & 0xFF

                chunk[4] = (i >> 24) & 0xFF
                chunk[5] = (i >> 16) & 0xFF
                chunk[6] = (i >> 8) & 0xFF
                chunk[7] = (i >> 0) & 0xFF

                chunk[8] = (len(chunk_data) >> 8) & 0xFF
                chunk[9] = len(chunk_data) & 0xFF

                chunk[10:10 + len(chunk_data)] = chunk_data
                chunk[10 + len(chunk_data)] = 0x04

                sock.sendall(chunk)
            return True
        except Exception as e:
            print(f"[SEND ERROR] {e}")
            return False
    
    """
    @brief Receives and decrypts data from a TCP socket.
    Reads encrypted data in chunks, validates headers and EOT markers, reconstructs
    the original encrypted message, and decrypts it using the CryptoHandler.
    @param sock The TCP socket to receive data from.
    @return The decrypted data as a string if successful, or False on error.
    """
    def receive(self, sock: socket.socket) -> str | bool:
        buffer = bytearray()
        received_chunks = None
        total_chunks = None

        try:
            while True:
                sock.settimeout(10)
                head = sock.recv(self._header_size)
                if head is None:
                    print("[RECEIVE ERROR] Timeout or socket closed before header")
                    return False

                total_chunks_read = (
                    (head[0] << 24) |
                    (head[1] << 16) |
                    (head[2] << 8)  |
                    (head[3] << 0)
                )
                chunk_index = (
                    (head[4] << 24) |
                    (head[5] << 16) |
                    (head[6] << 8)  |
                    (head[7] << 0)
                )
                chunk_len = (head[8] << 8) | head[9]

                needed = self._header_size + chunk_len + 1
                if needed > self._buffer_size:
                    print(f"[RECEIVE ERROR] chunk_len {chunk_len} inconsistent (need {needed} > {self._buffer_size})")
                    return False

                # Read payload plus EOT
                payload_plus_eot = self._recv_exact(sock, chunk_len + 1)
                if payload_plus_eot is None:
                    print(f"[RECEIVE ERROR] Timeout or socket closed while reading payload for chunk {chunk_index}")
                    return False

                # Check EOT marker
                if payload_plus_eot[-1] != 0x04:
                    print(f"[RECEIVE ERROR] Missing or misplaced EOT for chunk {chunk_index}")
                    return False

                remainder_padding = self._buffer_size - needed
                if remainder_padding > 0:
                    pad = self._recv_exact(sock, remainder_padding)
                    if pad is None:
                        print(f"[RECEIVE ERROR] Timeout or socket closed while reading padding for chunk {chunk_index}")
                        return False

                # Initialize tracking on the first chunk
                if total_chunks is None:
                    total_chunks = total_chunks_read
                    if total_chunks <= 0:
                        print(f"[RECEIVE ERROR] invalid total_chunks = {total_chunks}")
                        return False
                    received_chunks = [False] * total_chunks

                # Validate chunk_index
                if not (0 <= chunk_index < total_chunks):
                    print(f"[RECEIVE ERROR] chunk_index {chunk_index} out of bounds (total {total_chunks})")
                    return False
                
                if received_chunks[chunk_index]:
                    print(f"[RECEIVE ERROR] duplicate chunk {chunk_index}")
                    return False

                # Extract and append the ciphertext
                ciphertext = payload_plus_eot[:chunk_len]
                buffer.extend(ciphertext)
                received_chunks[chunk_index] = True

                # If all chunks are received, break
                if all(received_chunks):
                    break

            return self._crypto.decrypt(bytes(buffer))

        except Exception as e:
            print(f"[RECEIVE EXCEPTION] {e}")
            return False
