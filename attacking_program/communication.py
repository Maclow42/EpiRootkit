import socket
from config import BUFFER_SIZE
from encryption import aes_encrypt, aes_decrypt

def send_to_server(sock, data):
    # Encrypt the data
    data = aes_encrypt(data)

    # Define the maximum chunk size
    max_chunk_body_size = BUFFER_SIZE - 5
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
        chunk = bytearray(BUFFER_SIZE)
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
            print(f"send_to_server: failed to send message (chunk {i}): {e}")
            return False

    return True

def receive_from_server(sock):
    total_received = 0
    nb_chunks_needed = 0
    received_chunks = []
    buffer = bytearray()

    while True:
        # Receive a chunk
        chunk = sock.recv(BUFFER_SIZE)
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
            print(f"receive_from_server: all chunks received")
            break


    result = bytes(buffer)

    # Decrypt the data
    decrypted = aes_decrypt(result)

    return decrypted
