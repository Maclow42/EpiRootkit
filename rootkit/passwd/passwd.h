#ifndef PASSWD_H
#define PASSWD_H

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include "crypto.h"

#define PASSWD_HASH_SIZE SHA256_DIGEST_SIZE

extern u8 access_code_hash[PASSWD_HASH_SIZE];

int passwd_load(void);
int passwd_verify(const char *password);
int passwd_set(const char *new_password);

#endif /* PASSWD_H */