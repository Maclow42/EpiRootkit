#ifndef FORBID_H
#define FORBID_H

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/ptrace.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "config.h"

struct forbidden_file {
    char *path;
    struct list_head list;
};

extern struct list_head forbidden_files_list;
extern spinlock_t forbidden_files_lock;

int add_forbidden_file(const char *path);
int remove_forbidden_file(const char *path);
int list_forbidden_files(char *buf, size_t buf_size);

bool path_is_forbidden(const char __user *u_path);

extern asmlinkage long (*__orig_openat)(const struct pt_regs *);
extern asmlinkage long (*__orig_newfstatat)(const struct pt_regs *);
extern asmlinkage long (*__orig_fstat)(const struct pt_regs *);
extern asmlinkage long (*__orig_lstat)(const struct pt_regs *);
extern asmlinkage long (*__orig_stat)(const struct pt_regs *);
extern asmlinkage long (*__orig_chdir)(const struct pt_regs *regs);
extern asmlinkage long (*__orig_ptrace)(const struct pt_regs *regs);

asmlinkage long notrace openat_hook(const struct pt_regs *regs);
asmlinkage long notrace stat_hook(const struct pt_regs *regs);
asmlinkage long notrace chdir_hook(const struct pt_regs *regs);
asmlinkage void notrace ptrace_hook(struct pt_regs *regs);

#endif // FORBID_H
