# Configuration générale
HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024
PASSWORD = "epiroot"

UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

# Configuration AES (128 bits)
AES_KEY = b'1234567890abcdef'  # 16 bytes = 128 bits
AES_IV = b'abcdef1234567890'   # 16 bytes = 128 bits

# Configuration DNS
DNS_PORT = 53
DNS_DOMAIN = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL = 0.2
