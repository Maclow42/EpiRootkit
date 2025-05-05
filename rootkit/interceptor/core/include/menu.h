// interceptor/core/menu.h
#ifndef HOOKS_MENU_H
#define HOOKS_MENU_H

/**
 * Handle the top‐level `hooks` command.
 *
 * @param args Everything after the word "hooks", so "<subcmd> [<path>] [<other>...]"
 * @return 0 on success, negative errno on failure.
 */
int hooks_menu_handler(char *args);

#endif // HOOKS_MENU_H