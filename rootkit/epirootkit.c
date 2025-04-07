#include <linux/module.h>
#include <linux/net.h>
#include <linux/inet.h>

static struct socket *sock = NULL;
static char *ip = "127.0.0.1";
static int port = 4242;
static char *message = "Hello World! from kernel land";

module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(message, charp, 0644);

MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");
MODULE_PARM_DESC(message, "Message to send to the server");

static void *convert(void *ptr)
{
	return ptr;
}

static int __init network_init(void)
{
	struct sockaddr_in addr = { 0 };
	struct msghdr msg = { 0 };
	struct kvec vec = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret = 0;

	pr_info("network: insmoded\n");

	if ((ret = in4_pton(ip, -1, ip_binary, -1, NULL)) == 0)
	{
		pr_err("network: error converting the IPv4 address: %d\n", ret);
		return 1;
	}

	if ((ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock)) < 0)
	{
		pr_err("network: error creating the socket: %d\n", ret);
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	// equivalent to
	// addr.sin_addr.s_addr = *(unsigned int*)ip_binary;
	// without the explicit cast
	// I do not like explicit casts
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	if ((ret = sock->ops->connect(sock, convert(&addr), sizeof(addr), 0)) < 0)
	{
		pr_err("network: error connecting to %s:%d (%d)\n", ip, port, ret);
		sock_release(sock);
		return 1;
	}

	vec.iov_base = message;
	vec.iov_len = strlen(message);

	if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
	{
		pr_err("network: error sending the message: %d\n", ret);
		sock_release(sock);
		return 1;
	}

	pr_info("network: message '%s' sent to %s:%d\n", message, ip, port);

	// TODO: Handle the connection to keep communication with the server ;)
	return 42;
}

static void __exit network_exit(void)
{
	if (sock)
		sock_release(sock);

	pr_info("network: rmmoded\n");
}

module_init(network_init);
module_exit(network_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jules Aubert");
MODULE_DESCRIPTION("Connect to a TCP server using ipv4 and send a message");