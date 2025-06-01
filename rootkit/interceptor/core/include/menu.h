// interceptor/core/menu.h
#ifndef HOOKS_MENU_H
#define HOOKS_MENU_H

#include "config.h"

/**
 * Handle the top‐level `hooks` command.
 *
 * @param args Everything after the word "hooks", so "<subcmd> [<path>] [<other>...]"
 * @param protocol The protocol used for communication (TCP or DNS).
 * @return 0 on success, negative errno on failure.
 */
int hooks_menu_handler(char *args, enum Protocol protocol);

#endif // HOOKS_MENU_H