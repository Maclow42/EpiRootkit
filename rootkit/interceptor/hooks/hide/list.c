#include <linux/string.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "hide.h"

int is_hidden(const char *name) {
    struct hidden_dir_entry *e;
    int found = 0;

    spin_lock(&hidden_dirs_lock);
    list_for_each_entry(e, &hidden_dirs_list, list) {
        if (strcmp(e->dirname, name) == 0) {
            found = 1;
            break;
        }
    }
    spin_unlock(&hidden_dirs_lock);
    return found;
}

int add_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *e;
    if (is_hidden(dirname))
        return 0;

    if (dirname[0] != '/')
        return -EINVAL;

    e = kmalloc(sizeof(*e), GFP_KERNEL);
    if (!e) return -ENOMEM;
    e->dirname = kstrdup(dirname, GFP_KERNEL);
    if (!e->dirname) {
        kfree(e);
        return -ENOMEM;
    }

    spin_lock(&hidden_dirs_lock);
    list_add_tail(&e->list, &hidden_dirs_list);
    spin_unlock(&hidden_dirs_lock);

    return 0;
}

int remove_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *e, *tmp;
    int found = 0;

    spin_lock(&hidden_dirs_lock);
    list_for_each_entry_safe(e, tmp, &hidden_dirs_list, list) {
        if (strcmp(e->dirname, dirname) == 0) {
            list_del(&e->list);
            kfree(e->dirname);
            kfree(e);
            found = 1;
            break;
        }
    }
    spin_unlock(&hidden_dirs_lock);

    return found ? 0 : -ENOENT;
}

int list_hidden_dirs(char *buf, size_t buf_size) {
    struct hidden_dir_entry *e;
    size_t len = 0;

    spin_lock(&hidden_dirs_lock);
    list_for_each_entry(e, &hidden_dirs_list, list) {
        len += scnprintf(buf + len, buf_size - len, "%s\n", e->dirname);
        if (len >= buf_size - 1)
            break;
    }
    spin_unlock(&hidden_dirs_lock);

    return len;
}
