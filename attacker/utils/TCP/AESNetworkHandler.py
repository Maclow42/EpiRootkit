from utils.TCP.TCPServer import CryptoHandler
import socket

class AESNetworkHandler:
    def __init__(self, crypto: CryptoHandler, buffer_size: int = 1024):
        self._crypto = crypto
        self._buffer_size = buffer_size
        self._header_size = 5
        self._max_chunk_body_size = buffer_size - self._header_size

    def send(self, sock: socket.socket, data: str | bytes) -> bool:
        try:
            encrypted = self._crypto.encrypt(data)
            total_len = len(encrypted)
            nb_chunks = (total_len + self._max_chunk_body_size - 1) // self._max_chunk_body_size

            for i in range(nb_chunks):
                chunk_data = encrypted[i * self._max_chunk_body_size:(i + 1) * self._max_chunk_body_size]
                chunk = bytearray(self._buffer_size)
                chunk[0] = nb_chunks
                chunk[1] = i
                chunk[2] = (len(chunk_data) >> 8) & 0xFF
                chunk[3] = len(chunk_data) & 0xFF
                chunk[4:4 + len(chunk_data)] = chunk_data
                chunk[4 + len(chunk_data)] = 0x04
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
                chunk = sock.recv(self._buffer_size)
                if not chunk:
                    print("[RECEIVE ERROR] Socket closed")
                    return False

                if len(chunk) < self._header_size:
                    print("[RECEIVE ERROR] Incomplete chunk header")
                    return False

                total_chunks = chunk[0]
                chunk_index = chunk[1]
                chunk_len = (chunk[2] << 8) | chunk[3]

                if len(chunk) < self._header_size + chunk_len + 1:
                    print(f"[RECEIVE ERROR] Incomplete chunk data for chunk {chunk_index}")
                    remaining = self._buffer_size - len(chunk)
                    chunk += sock.recv(remaining)

                chunk_data = chunk[4:4 + chunk_len]

                if chunk[4 + chunk_len] != 0x04:
                    print(f"[RECEIVE ERROR] Missing EOT for chunk {chunk_index}")
                    return False

                if nb_chunks_needed == 0:
                    nb_chunks_needed = total_chunks
                    received_chunks = [False] * nb_chunks_needed

                if received_chunks[chunk_index]:
                    print(f"[RECEIVE ERROR] Duplicate chunk {chunk_index}")
                    return False

                received_chunks[chunk_index] = True
                buffer.extend(chunk_data)

                if all(received_chunks):
                    break

            return self._crypto.decrypt(bytes(buffer))
        except Exception as e:
            print(f"[RECEIVE EXCEPTION] {e}")
            return False
