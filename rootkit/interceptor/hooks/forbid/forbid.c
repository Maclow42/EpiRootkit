#include "forbid.h"

LIST_HEAD(forbidden_files_list);
spinlock_t forbidden_files_lock = __SPIN_LOCK_UNLOCKED(forbidden_files_lock);

asmlinkage long (*__orig_openat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_newfstatat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_fstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_lstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_stat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_chdir)(const struct pt_regs *regs) = NULL;

asmlinkage long openat_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->si;
    if (path_is_forbidden(u_path))
        return -ENOENT;
    return __orig_openat(regs);
}

asmlinkage long stat_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->si;
    if (path_is_forbidden(u_path))
        return -ENOENT;

    switch ((int)regs->orig_ax) {
    case __NR_stat:
        return __orig_stat(regs);
    case __NR_lstat:
        return __orig_lstat(regs);
    case __NR_fstat:
        return __orig_fstat(regs);
    case __NR_newfstatat:
        return __orig_newfstatat(regs);
    default:
        return -ENOENT;
    }
}

asmlinkage long chdir_hook(const struct pt_regs *regs)
{
    const char __user *u_path = (const char __user *)regs->di;
    if (path_is_forbidden(u_path))
        return -ENOENT;
    return __orig_chdir(regs);
}