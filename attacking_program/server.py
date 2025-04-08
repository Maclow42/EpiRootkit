import socket

HOST = '127.0.0.1'
PORT = 4242

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Reuse the address/port immediately after closing
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server_socket.bind((HOST, PORT))
server_socket.listen(1)

print(f"Server listening on {HOST}:{PORT}...")

conn, addr = server_socket.accept()
print(f"Connection accepted from {addr}")

data = conn.recv(1024).decode()
if not data or data.lower() == 'quit':
	print("Connection closed by the client.")

print(f"Client: {data}")

while True:
	# Read the server's message
	reply = ""
	line = input("You: ")

	reply += line + "\n"

	# If empty, send a default message or ignore
	if not reply:
		print("Empty message, nothing sent.")
		continue

	print(f"Sending: {reply}")
	conn.sendall(reply.encode())

	if 'killcom' in reply.lower():
		print("Connection closed by the server.")
		break

conn.close()
server_socket.close()
