#ifndef IO_H
#define IO_H

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/types.h>

int _read_file(const char *path, char **out_buf);
int _write_file(const char *path, const char *buf, size_t len);
void build_cfg_path(const char *fname, char *out, size_t sz);

#endif // IO_H