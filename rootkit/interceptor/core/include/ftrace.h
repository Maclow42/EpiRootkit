#ifndef FTRACE_H
#define FTRACE_H

#include <linux/file.h>
#include <linux/ftrace.h>
#include <linux/types.h>

// Hooks and Ftrace parameters
struct ftrace_hook{
    const char *name;       							// Name of the target symbol
    void *function;         							// Address of the hook function
    void *original;        								// Pointer to storage for the original address
    unsigned long address; 	 							// Resolved address of the target symbol
    struct ftrace_ops ops;
};

#define SYSCALL_NAME(name) ("__x64_" name)
#define HOOK_SYS(_name, _hook, _orig) {					\
    .name = SYSCALL_NAME(_name),                        \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}

#define HOOK(_name, _hook, _orig) {					    \
    .name = (_name),                                    \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}

extern struct ftrace_hook hooks[];
extern size_t hook_array_size;

unsigned long (*fh_init_kallsyms_lookup(void))(const char *);
int fh_install_hook(struct ftrace_hook *hook);
void fh_remove_hook(struct ftrace_hook *hook);
int fh_install_hooks(struct ftrace_hook *hooks, size_t count);
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count);

#endif // FTRACE_H