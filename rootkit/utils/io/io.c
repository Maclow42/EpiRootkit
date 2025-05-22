#include "io.h"
#include "config.h"

int _read_file(const char *path, char **out_buf) {
    struct file *f;
    loff_t pos = 0;
    char *buf;
    int ret;

    buf = kzalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    f = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(f)) {
        ret = PTR_ERR(f);
        kfree(buf);
        return ret;
    }

    ret = kernel_read(f, buf, STD_BUFFER_SIZE - 1, &pos);
    filp_close(f, NULL);
    if (ret < 0) {
        kfree(buf);
        return ret;
    }

    buf[ret] = '\0';
    *out_buf = buf;
    return ret;
}

int _write_file(const char *path, const char *buf, size_t len) {
    struct file *f;
    loff_t pos = 0;
    int ret;

    f = filp_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (IS_ERR(f))
        return PTR_ERR(f);

    ret = kernel_write(f, buf, len, &pos);
    filp_close(f, NULL);
    return ret;
}

void build_cfg_path(const char *fname, char *out, size_t sz) {
    snprintf(out, sz, "%s/%s", HIDDEN_DIR_PATH, fname);
}