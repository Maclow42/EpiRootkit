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

static bool ulist_has_item(struct ulist *ul, const char *value) {
    struct ulist_item *it;

    // Check only the value in the list
    // (we don't care about flags or payload)
    list_for_each_entry(it, &ul->head, list) {
        if (strcmp(it->value, value) == 0)
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
    char *p, *newline;
    int ret;

    if (!ul || !ul->filename) {
        ERR_MSG("ulist_load: invalid arguments ul=%p filename=%p\n", ul,
                ul ? ul->filename : NULL);
        return -EINVAL;
    }
    build_cfg_path(ul->filename, cfgpath, sizeof(cfgpath));

    ret = _read_file(cfgpath, &buf);
    if (ret < 0) {
        if (ret == -ENOENT) {
            DBG_MSG("ulist_load: no config file '%s'\n", cfgpath);
            return 0;
        }
        ERR_MSG("ulist_load: error reading '%s': %d\n", cfgpath, ret);
        return ret;
    }

    if (!buf[0]) {
        DBG_MSG("ulist_load: empty config '%s'\n", cfgpath);
        kfree(buf);
        return 0;
    }

    p = buf;
    while (*p) {
        char line[ULIST_LINE_MAX];
        size_t len;
        char *fields[3] = { NULL, NULL, NULL };
        char *cursor;
        u32 flags = 0;
        char *payload = NULL;
        int rc;

        // Find end of this line
        newline = strpbrk(p, "\r\n");
        len = newline ? (newline - p) : strlen(p);

        // Skip empty lines
        if (len == 0) {
            if (!newline)
                break;
            p = newline + 1;
            continue;
        }

        // Clamp overly long lines
        if (len >= ULIST_LINE_MAX) {
            ERR_MSG("ulist_load: skipping too-long line\n");
            if (!newline)
                break;
            p = newline + 1;
            continue;
        }

        memcpy(line, p, len);
        line[len] = '\0';

        // Split into exactly three fields around '|'
        cursor = line;
        fields[0] = strsep(&cursor, "|");
        fields[1] = strsep(&cursor, "|");
        fields[2] = cursor;

        // The field[0] must be non-empty eheh
        if (!fields[0] || !*fields[0]) {
            ERR_MSG("ulist_load: missing value, skipping line\n");
            if (!newline)
                break;
            p = newline + 1;
        }

        // Parse flags (default to 0 on error)
        if (fields[1] && *fields[1]) {
            int err = kstrtou32(fields[1], 10, &flags);
            if (err) {
                ERR_MSG("ulist_load: bad flags '%s', using 0\n", fields[1]);
                flags = 0;
            }
        }

        // Payload may be empty string so treat it as NULL lol
        if (fields[2] && *fields[2])
            payload = fields[2];

        // Add to list
        rc = ulist_add(ul, fields[0], flags, payload);
        if (rc < 0)
            ERR_MSG("ulist_load: ulist_add(%s) failed: %d\n", fields[0], rc);

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
        int n = scnprintf(q, left, "%s|%u|%s\n", it->value, it->flags,
                          it->payload ?: "");
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

int ulist_add(struct ulist *ul, const char *value, u32 flags,
              const char *payload) {
    if (ulist_has_item(ul, value)) {
        return -EEXIST;
    }

    struct ulist_item *it = kzalloc(sizeof(*it), GFP_KERNEL);
    if (!it)
        return -ENOMEM;

    it->value = kstrdup(value, GFP_KERNEL);
    it->flags = flags;
    it->payload = payload ? kstrdup(payload, GFP_KERNEL) : NULL;
    INIT_LIST_HEAD(&it->list);

    spin_lock(&ul->lock);
    list_add_tail(&it->list, &ul->head);
    spin_unlock(&ul->lock);
    return SUCCESS;
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
        n = scnprintf(p, left, "%s|%u|%s\n", it->value, it->flags,
                      it->payload ?: "");
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
