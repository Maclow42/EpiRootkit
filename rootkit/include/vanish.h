#ifndef VANISH_H
#define VANISH_H

#include <linux/types.h> 

bool check_hypervisor(void);
bool check_dmi(void);
bool is_running_in_virtual_env(void);

#endif // VANISH_H