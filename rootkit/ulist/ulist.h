#ifndef ULIST_H
#define ULIST_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct ulist_item {
    char *value;
    u32 flags;
    char *payload;
    struct list_head list;
};

struct ulist {
    struct list_head head;
    spinlock_t lock;
    const char *filename;
};

static inline void ulist_init(struct ulist *ul, const char *fname)
{
    INIT_LIST_HEAD(&ul->head);
    spin_lock_init(&ul->lock);
    ul->filename = fname;
}

void ulist_clear(struct ulist *ul);

int ulist_contains(struct ulist *ul, const char *value);
int ulist_load(struct ulist *ul);
int ulist_save(struct ulist *ul);
int ulist_add(struct ulist *ul, const char *value, u32 flags, const char *payload);
int ulist_remove(struct ulist *ul, const char *value);
int ulist_contains(struct ulist *ul, const char *value);
int ulist_list(struct ulist *ul, char *buf, size_t buf_size);

#endif /* ULIST_H */
