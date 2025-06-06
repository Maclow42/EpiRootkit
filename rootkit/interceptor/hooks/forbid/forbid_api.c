#include "forbid_api.h"

#include "config.h"
#include "ulist.h"

struct ulist forbid_list;

int forbid_init(void) {
    int ret;

    ulist_init(&forbid_list, FORBID_CFG_FILE);
    ret = ulist_load(&forbid_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

void forbid_exit(void) {
    ulist_save(&forbid_list);
    ulist_clear(&forbid_list);
}

int forbid_file(const char *path)
{
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

    ret = ulist_add(&forbid_list, modpath, 0, NULL);
    if (ret < 0)
        return ret;

    ret = ulist_save(&forbid_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

int unforbid_file(const char *path) {
    int ret;

    ret = ulist_remove(&forbid_list, path);
    if (ret < 0)
        return ret;

    ret = ulist_save(&forbid_list);
    if (ret < 0)
        return ret;

    return SUCCESS;
}

static char *get_abs_path(const char __user *u_path, char *buf, int buflen)
{
    struct path path;
    char *full;
    int err;

    err = user_path_at(AT_FDCWD, u_path, LOOKUP_FOLLOW, &path);
    if (err)
        return NULL;

    full = d_path(&path, buf, buflen);
    path_put(&path);
    if (IS_ERR(full))
        return NULL;
    return full;
}

int forbid_contains(const char __user *u_path)
{
    char *buf;
    char *full;
    int blocked = 0;

    if (!u_path)
        return 0;

    buf = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!buf)
        return 0;

    full = get_abs_path(u_path, buf, PATH_MAX);
    if (full) {
        blocked = ulist_contains(&forbid_list, full);
    }

    kfree(buf);
    return blocked;
}

int forbid_contains_str(const char *k_path) {
    return ulist_contains(&forbid_list, k_path);
}

int forbid_list_get(char *buf, size_t buf_size) {
    return ulist_list(&forbid_list, buf, buf_size);
}
