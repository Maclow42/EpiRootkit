#ifndef PERSIST_H
#define PERSIST_H

#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <linux/slab.h>

int persist(void);

#endif // PERSIST_H