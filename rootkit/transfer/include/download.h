#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "config.h"

bool is_downloading(void);
void reset_download_state(void);

int download_handler(char *args, enum Protocol protocol);
int download(const char *command);

#endif
