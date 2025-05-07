#ifndef INIT_H
#define INIT_H

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/string.h>

#include "alterate.h"
#include "config.h"
#include "epirootkit.h"
#include "forbid.h"
#include "ftrace.h"
#include "hide.h"

int init_interceptor(void);
void exit_interceptor(void);

int create_dir(char *path);

#endif // INIT_H