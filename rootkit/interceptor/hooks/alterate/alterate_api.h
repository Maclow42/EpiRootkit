#ifndef ALTERATE_API_H
#define ALTERATE_API_H

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/types.h>

extern struct ulist alt_list;

int alterate_init(void);
void alterate_exit(void);
int alterate_add(const char *path, int hide_line, const char *hide_substr, const char *src, const char *dst);
int alterate_remove(const char *path);
int alterate_contains(const char __user *u_path);
int alterate_list_get(char *buf, size_t buf_size);

#endif // ALTERATE_API_H
