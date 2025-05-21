#include "ulist.h"

#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "config.h"
#include "io.h"

static bool ulist_has_item(struct ulist *ul, const char *value, const char *payload) {
    struct ulist_item *it;

    list_for_each_entry(it, &ul->head, list) {
        if (strcmp(it->value, value) != 0)
            continue;
        if (it->payload == NULL && payload == NULL)
            return true;
        if (it->payload && payload && strcmp(it->payload, payload) == 0)
            return true;
    }
    return false;
}

void ulist_clear(struct ulist *ul) {
    struct ulist_item *it, *tmp;
    spin_lock(&ul->lock);
    list_for_each_entry_safe(it, tmp, &ul->head, list) {
        list_del(&it->list);
        kfree(it->value);
        kfree(it->payload);
        kfree(it);
    }
    spin_unlock(&ul->lock);
}

int ulist_load(struct ulist *ul) {
    char cfgpath[512];
    char *buf = NULL;
    char *p;
    int ret;

    if (!ul || !ul->filename) {
        pr_err("ulist_load: invalid arguments ul=%p filename=%p\n", ul, ul ? ul->filename : NULL);
        return -EINVAL;
    }
    build_cfg_path(ul->filename, cfgpath, sizeof(cfgpath));

    ret = _read_file(cfgpath, &buf);
    if (ret < 0) {
        if (ret == -ENOENT) {
            pr_info("ulist_load: no config file '%s'\n", cfgpath);
            return 0;
        }
        pr_err("ulist_load: error reading '%s': %d\n", cfgpath, ret);
        return ret;
    }

    if (!buf[0]) {
        pr_info("ulist_load: empty config '%s'\n", cfgpath);
        kfree(buf);
        return 0;
    }

    // Parse each line: "value|flags|payload\n"
    p = buf;
    while (*p) {
        char line[ULIST_LINE_MAX];
        char *newline = strpbrk(p, "\r\n");
        int len = newline ? (newline - p) : strlen(p);
        char *fields[3] = { "", "0", "" };
        char *cursor;
        u32 flags;
        char *payload;
        int rc;

        // Clamp and copy the raw line
        if (len >= ULIST_LINE_MAX)
            len = ULIST_LINE_MAX - 1;
        memcpy(line, p, len);
        line[len] = '\0';

        // Split into fields
        cursor = line;
        fields[0] = strsep(&cursor, "|");
        fields[1] = strsep(&cursor, "|");
        fields[2] = cursor;

        if (!fields[1]) {
            if (!newline)
                break;
            p = newline + 1;
            continue;
        }

        flags = (u32)simple_strtoul(fields[1], NULL, 10);
        payload = *fields[2] ? fields[2] : NULL;

        // Add into the in-memory list
        rc = ulist_add(ul, fields[0], flags, payload);
        if (rc < 0) {
            pr_err("ulist_load: ulist_add(%s) failed: %d\n", fields[0], rc);
        }

        if (!newline)
            break;
        p = newline + 1;
    }

    kfree(buf);
    return 0;
}

int ulist_save(struct ulist *ul) {
    char cfgpath[128];
    char *buf, *q;
    size_t left = STD_BUFFER_SIZE;
    struct ulist_item *it;
    int ret;

    buf = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    q = buf;

    spin_lock(&ul->lock);
    list_for_each_entry(it, &ul->head, list) {
        int n = scnprintf(q, left, "%s|%u|%s\n", it->value, it->flags, it->payload ?: "");
        q += n;
        left -= n;
        if (left <= 0)
            break;
    }
    spin_unlock(&ul->lock);

    build_cfg_path(ul->filename, cfgpath, sizeof(cfgpath));
    ret = _write_file(cfgpath, buf, q - buf);
    kfree(buf);
    return ret > 0 ? 1 : ret;
}

int ulist_add(struct ulist *ul, const char *value, u32 flags, const char *payload) {
    struct ulist_item *it = kzalloc(sizeof(*it), GFP_KERNEL);
    if (!it)
        return -ENOMEM;

    it->value = kstrdup(value, GFP_KERNEL);
    it->flags = flags;
    it->payload = payload ? kstrdup(payload, GFP_KERNEL) : NULL;
    INIT_LIST_HEAD(&it->list);

    if (ulist_has_item(ul, value, payload)) {
        kfree(it->value);
        kfree(it->payload);
        kfree(it);
        return 0;
    }

    spin_lock(&ul->lock);
    list_add_tail(&it->list, &ul->head);
    spin_unlock(&ul->lock);
    return 0;
}

int ulist_remove(struct ulist *ul, const char *value) {
    struct ulist_item *it, *tmp;
    spin_lock(&ul->lock);
    list_for_each_entry_safe(it, tmp, &ul->head, list) {
        if (!strcmp(it->value, value)) {
            list_del(&it->list);
            kfree(it->value);
            kfree(it->payload);
            kfree(it);
        }
    }
    spin_unlock(&ul->lock);
    return 0;
}

int ulist_contains(struct ulist *ul, const char *value) {
    struct ulist_item *it;
    int found = 0;

    spin_lock(&ul->lock);
    list_for_each_entry(it, &ul->head, list) {
        if (!strcmp(it->value, value)) {
            found = 1;
            break;
        }
    }
    spin_unlock(&ul->lock);
    return found;
}

int ulist_list(struct ulist *ul, char *buf, size_t buf_size) {
    struct ulist_item *it;
    size_t left = buf_size;
    char *p = buf;
    int n;

    if (!ul || !buf || buf_size == 0)
        return -EINVAL;

    spin_lock(&ul->lock);
    list_for_each_entry(it, &ul->head, list) {
        n = scnprintf(p, left, "%s|%u|%s\n", it->value, it->flags, it->payload ?: "");
        if (n >= left)
            break;
        p += n;
        left -= n;
    }
    spin_unlock(&ul->lock);

    if (left > 0)
        *p = '\0';
    else
        buf[buf_size - 1] = '\0';

    return p - buf;
}
