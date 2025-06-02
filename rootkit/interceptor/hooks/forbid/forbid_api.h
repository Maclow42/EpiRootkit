#ifndef FORBID_API_H
#define FORBID_API_H

#include <linux/dcache.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/types.h>

int forbid_init(void);
void forbid_exit(void);
int forbid_file(const char *path);
int unforbid_file(const char *path);
int forbid_contains(const char __user *u_path);
int forbid_contains_str(const char *k_path)
int forbid_list_get(char *buf, size_t buf_size);

#endif /* FORBID_API_H */
