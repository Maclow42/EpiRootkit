#ifndef FTRACE_HELPER_H
#define FTRACE_HELPER_H

#include <linux/errno.h>
#include <linux/ftrace.h>
#include <linux/module.h>
#include <linux/types.h>


// Structure to store information about an ftrace hook.
struct ftrace_hook
{
    const char *name;       /* Name of the target symbol */
    void *function;         /* Address of the hook function */
    void *original;         /* Pointer to storage for the original address */
    unsigned long address;  /* Resolved address of the target symbol */
    struct ftrace_ops ops;  /* ftrace operations data */
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define SYSCALL_NAME(name) ("__x64_" name)
#define HOOK(_name, _hook, _orig)                                              \
    {                                                                          \
        .name = SYSCALL_NAME(_name),                                           \
        .function = (_hook),                                                   \
        .original = (_orig),                                                   \
    }

unsigned long (*fh_init_kallsyms_lookup(void))(const char *);

int fh_install_hook(struct ftrace_hook *hook);
void fh_remove_hook(struct ftrace_hook *hook);
int fh_install_hooks(struct ftrace_hook *hooks, size_t count);
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count);

#endif /* FTRACE_HELPER_H */