#ifndef HIDE_API_H
#define HIDE_API_H

#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/errno.h>

int hide_init(void);
void hide_exit(void);
int hide_file(const char *path);
int unhide_file(const char *path);
int hide_contains_str(const char *u_path);
int hide_list_get(char *buf, size_t buf_size);

#endif // HIDE_API_H
