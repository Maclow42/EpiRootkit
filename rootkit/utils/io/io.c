#include "io.h"

#include "config.h"

/**
 * _read_file - Reads the contents of a file into a dynamically allocated buffer.
 * @path: Path to the file to be read.
 * @out_buf: Pointer to a char pointer that will receive the allocated buffer containing the file contents.
 * Return: On success, returns the number of bytes read (excluding the null terminator).
 *         On failure, returns a negative error code.
 */
int _read_file(const char *path, char **out_buf) {
    size_t tot = 0;
    size_t buf_size = STD_BUFFER_SIZE;
    struct file *f;
    loff_t pos = 0;
    char *buf;
    int ret;

    buf = kzalloc(buf_size, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    f = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(f)) {
        ret = PTR_ERR(f);
        kfree(buf);
        return ret;
    }

    // Pas bo
    while ((ret = kernel_read(f, buf + tot, buf_size - tot - 1, &pos)) > 0) {
        tot += ret;

        if (tot >= buf_size - 1) {
            size_t new_size = buf_size * 2;
            char *resized = krealloc(buf, new_size, GFP_KERNEL);
            if (!resized) {
                kfree(buf);
                filp_close(f, NULL);
                return -ENOMEM;
            }

            buf = resized;
            buf_size = new_size;
        }
    }

    filp_close(f, NULL);
    if (ret < 0) {
        kfree(buf);
        return ret;
    }

    buf[tot] = '\0';
    *out_buf = buf;
    return tot;
}

/**
 * _write_file - Write a buffer to a file in the kernel space.
 * @path: Path to the file to write.
 * @buf: Pointer to the buffer containing data to write.
 * @len: Number of bytes to write from the buffer.
 * Return: Number of bytes written on success, or a negative error code on failure.
 */
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

/**
 * build_cfg_path - Constructs a full path for a configuration file in the hidden directory.
 * @fname: Name of the configuration file.
 * @out: Output buffer to store the full path.
 * @sz: Size of the output buffer.
 */
void build_cfg_path(const char *fname, char *out, size_t sz) {
    snprintf(out, sz, "%s/%s", HIDDEN_DIR_PATH, fname);
}