import os

HOST = '0.0.0.0'
PORT = 4242
BUFFER_SIZE = 1024
PASSWORD = "epiroot"
UPLOAD_FOLDER = "uploads"
DOWNLOAD_FOLDER = "downloads"

AES_KEY = b'1234567890abcdef'  # 16 bytes
AES_IV = b'abcdef1234567890'   # 16 bytes

DNS_PORT = 53
DNS_DOMAIN = "dns.google.com"
DNS_RESPONSE_TIMEOUT = 12.0
DNS_POLL_INTERVAL = 0.2

os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)
