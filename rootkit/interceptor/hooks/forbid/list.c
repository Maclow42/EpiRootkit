#include <linux/errno.h>
#include <linux/namei.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "forbid.h"

int add_forbidden_file(const char *path) {
    struct forbidden_file *f;

    spin_lock(&forbidden_files_lock);
    list_for_each_entry(f, &forbidden_files_list, list) {
        if (strcmp(f->path, path) == 0) {
            spin_unlock(&forbidden_files_lock);
            return 0;
        }
    }
    spin_unlock(&forbidden_files_lock);

    f = kmalloc(sizeof(*f), GFP_KERNEL);
    if (!f)
        return -ENOMEM;
    f->path = kstrdup(path, GFP_KERNEL);
    if (!f->path) {
        kfree(f);
        return -ENOMEM;
    }

    spin_lock(&forbidden_files_lock);
    list_add_tail(&f->list, &forbidden_files_list);
    spin_unlock(&forbidden_files_lock);

    pr_info("forbidden: added %s\n", path);
    return 0;
}

int remove_forbidden_file(const char *path) {
    struct forbidden_file *f, *tmp;
    int found = 0;

    spin_lock(&forbidden_files_lock);
    list_for_each_entry_safe(f, tmp, &forbidden_files_list, list) {
        if (strcmp(f->path, path) == 0) {
            list_del(&f->list);
            kfree(f->path);
            kfree(f);
            found = 1;
            break;
        }
    }
    spin_unlock(&forbidden_files_lock);
    return found ? 0 : -ENOENT;
}

int list_forbidden_files(char *buf, size_t buf_size) {
    struct forbidden_file *f;
    size_t len = 0;

    spin_lock(&forbidden_files_lock);
    list_for_each_entry(f, &forbidden_files_list, list) {
        len += scnprintf(buf + len, buf_size - len, "%s\n", f->path);
        if (len >= buf_size - 1)
            break;
    }
    spin_unlock(&forbidden_files_lock);
    return len;
}

bool path_is_forbidden(const char __user *u_path) {
    struct path p;
    char *tmp;
    char buf[512];
    bool deny = false;
    struct forbidden_file *f;

    if (kern_path(u_path, LOOKUP_FOLLOW, &p) == 0) {
        tmp = d_path(&p, buf, sizeof(buf));
        path_put(&p);
        if (!IS_ERR(tmp)) {
            spin_lock(&forbidden_files_lock);
            list_for_each_entry(f, &forbidden_files_list, list) {
                if (strcmp(tmp, f->path) == 0) {
                    deny = true;
                    break;
                }
            }
            spin_unlock(&forbidden_files_lock);
        }
    }
    return deny;
}
