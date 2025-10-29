\page network Network
\tableofcontents

## 1. 🌐 Introduction
The rootkit implements two primary network communication channels between the attacker and victim machines:

- A **TCP** channel encrypted for exchanging commands and data.
- A stealth **DNS** channel for sending commands and receiving results via DNS queries.

Both channels use AES-128 to secure exchanged data. The TCP channel is the primary channel; the DNS channel is used as a covert or fallback communication method.

## 2. 🤝 TCP

### 2.1 🧠 Introduction

To secure our network communications, we chose to use AES-128 encryption for all data exchanged between the client and server. However, the drawback of this algorithm is that it doesn't allow transmitting data of arbitrary size. Indeed, AES-128 encryption produces a 16-byte block, meaning data must be split into blocks of this size before being encrypted.

When transmitting data via a socket, it's common for data to be of variable size, which poses a problem for encryption. The same applies to received data, which may be of variable size and not correspond to a multiple of 16 bytes.

To solve this problem, we implemented a custom chunked transmission protocol. This protocol allows splitting data into fixed-size chunks, each enriched with an (unencrypted) header for identification, reconstruction, and error detection. Thus, even if the data is of variable size, it can be split into fixed-size chunks, allowing secure and reliable transmission.

### 2.2 📦 Protocol {#tcp-protocol}

#### Important Constants

<div class="full_width_table">
| Constant         | Default Value     | Description                                |
|------------------|------------------|--------------------------------------------|
| `STD_BUFFER_SIZE`| 1024             | Fixed size of buffers used                 |
| `CHUNK_OVERHEAD` | 11               | 10 (header) + 1 (EOT_CODE)                 |
| `EOT_CODE`       | `0x04`           | ASCII code for "End of Transmission"       |
</div>

#### Objective

This custom protocol allows reliable transmission of arbitrary-size data (text or files) between a client and server via a kernel socket. Data is **encrypted** then **split into fixed chunks**, each enriched with a header for identification, reconstruction, and error detection.

#### General Structure

Each chunk is a constant-size buffer of `STD_BUFFER_SIZE` bytes structured as follows:

```
+-------------------+-------------------+-------------------+-------------------------------+------------+
| total_chunks (4B) | chunk_index (4B)  | data_len (2B)     | payload (≤ BODY_SIZE, var.)   | EOT (1B)   |
+-------------------+-------------------+-------------------+-------------------------------+------------+
```

#### Fields
<div class="full_width_table">
| Field         | Size       | Description                                                                 |
|---------------|------------|-----------------------------------------------------------------------------|
| `total_chunks`| 4 bytes    | Total number of chunks (big-endian)                                        |
| `chunk_index` | 4 bytes    | Index of this chunk in the sequence (big-endian)                           |
| `data_len`    | 2 bytes    | Actual data length in the chunk (big-endian)                               |
| `payload`     | variable   | Encrypted data                                                             |
| `EOT_CODE`    | 1 byte     | End of transmission code for the chunk (valid if set)                      |
| `padding`     | variable   | Padding to reach `STD_BUFFER_SIZE`, ignored on reception                   |
</div>

> 🔒 **All data sent in the payload is encrypted before being split into chunks.**

#### Sending

1. **Encryption:** Raw data is encrypted with AES-128 via `encrypt_buffer`.
2. **Splitting:** The encrypted buffer is segmented into chunks of `BODY_SIZE` (= `STD_BUFFER_SIZE - 11 (HEADER_SIZE + FOOTER_SIZE)`).
3. **Encapsulation:** Each chunk is prefixed with a structured header containing:
  - The total number of chunks
  - The chunk index
  - The useful data length
  - The `EOT_CODE` marker at the end of data
4. **Transmission:** Each chunk is sent via `kernel_sendmsg`.

#### Reception

1. **Progressive reading:**
  - Read the header (10 bytes).
  - Read the `payload` + `EOT` (useful data).
  - Read any padding bytes.
2. **Validation:**
  - Verify sizes.
  - Verify correct presence of `EOT_CODE`.
  - Ensure consistency of `total_chunks` and `chunk_index`.
3. **Assembly:**
  - Allocate a reception buffer if it's the first chunk.
  - Mark each received chunk as `seen`.
  - Copy data to the correct position.
  - Wait for all chunks to be received.
4. **Decryption:** Once all chunks are received, assemble and decrypt data with AES-128 algorithm.
5. **Processing received message:**
  - If data starts with `exec`, process as text command.
  - If a file transfer is in progress, received data is handled by the file transfer module.
  - Otherwise, it's copied to the user buffer.

#### Strengths

- **Reliability:** Each chunk contains meta-information for consistency verification.
- **Idempotence:** Chunks are managed so duplicates don't cause issues (direct data copy into array using chunk index).
- **Arbitrary size:** Protocol supports sending messages up to 4 TB.
- **Security:** All transfers are encrypted.
- **Flexibility:** Handles both raw text and binary file transfers.

#### Limitations

- Protocol doesn't handle retransmissions: it assumes sockets are reliable or transmission errors are handled by the underlying TCP protocol.
- No checksum is integrated to verify integrity after encryption.
- Wait time to receive all chunks is not limited (can block indefinitely).

### 2.3 🛠️ Implementation

The custom chunked transmission protocol is implemented in the `network.c` file (for the rootkit) and the `AESNetworkHandler.py` file (for the attacker). Here's an overview of the main functions:

The main functions of the chunked protocol are:

- `send_to_server_raw(const char *data, size_t len)`:
  This function encrypts the data to send, splits it into fixed-size chunks, adds a header to each chunk (total number of chunks, index, useful size, end marker), then sends them one by one via the kernel socket.
  Simplified example:

  ```c
  // Encrypt the data before sending
  if (encrypt_buffer(data, len, &encrypted_msg, &encrypted_len) < 0)
        return -EIO;

  // [... Calculate number of chunks and max chunk body size...]

  // Send each chunk separately
  for (i = 0; i < nb_chunks; ++i) {
      // Construction of the header 
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

      // Copy the encrypted message into the chunk
      memcpy(chunk + 10, encrypted_msg + i * max_chunk_body, chunk_len);

      // Add the EOT_CODE at the end
      chunk[10 + chunk_len] = EOT_CODE;
      
      // [... Send the chunk via kernel_sendmsg ...]
  }
  ```

- `receive_from_server(char *buffer, size_t max_len)`:
  This function reads data received from the kernel socket, reads each chunk, verifies its header, assembles data in a reception buffer, and decrypts the complete message once all chunks are received. These are basically the inverse operations of `send_to_server_raw`.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
