\page keylog Keylogger

\tableofcontents

In the context of Epirootkit, the keylogger is an essential tool for monitoring keyboard inputs. It's used to capture user inputs, which can be useful for detecting identification keystroke patterns or monitoring user activity.

## ⚙️ Technical Operation

Epirootkit's keylogger is implemented as a Linux kernel module, using the keyboard notification system (`keyboard_notifier`) and the `debugfs` interface to expose captured keystrokes.

Enable with `klgon` command, disable with `klgoff`.

### Architecture

## Data Retrieval

- **Keystroke capture**:

  The module registers with the kernel via a `notifier_block` (`epikeylog_blk`). At each keyboard event (press or release), the callback function epikeylog_callback() is called.Captured keystrokes can be retrieved using the `klg` command.

  ```c

  // Notifier block structure for keyboard events## Storage

  static struct notifier_block epikeylog_blk = {

      .notifier_call = epikeylog_callback,Keystroke data is stored in kernel memory and can be exported to the attacker.

  };
  ```
- **Key code translation**:
  Key codes (`keycode`) are converted into readable strings thanks to the `keycode_to_string()` function, which relies on a correspondence table (`char *keymap[][2]`) to handle normal keys and keys with shift.
- **Exposure via debugfs**:
  Keystrokes are accessible for reading in a `keys` file located in a hidden directory under `/sys/kernel/debug/`, dynamically created at module initialization.

> **Note**: The idea of writing to debugfs comes from various projects seen on Github and it's used for its ease of use and its rich API already present in the Linux kernel. The initial objective was to store keystrokes in a file at the root of the rootkit's hidden directory. However, at the time of these tests we couldn't write to a file the way we manage to do it here with a debugfs file. Thus, to still protect the file, we named it with the prefix `stdbool_bypassed_ngl_` which is a default prefix used by the [hooks](#hooks-introduction) to designate hidden files by default. Thus, the file will be invisible to a regular user, but accessible by the rootkit.

- **Sending to server**:
  The `epikeylog_send_to_server` function reads the buffer content and sends it to a remote server via a dedicated function.

### Main functions

| Function | Description |
|:----------|:-------------|
| `epikeylog_init()` | Initializes keylogger, creates debugfs directory and file, and registers keyboard notifier. |
| `epikeylog_callback()` | Function called at each keyboard event, converts and stores the key. |
| `epikeylog_send_to_server()` | Exports keylogger content to a server. |
| `epikeylog_exit()` | Cleans up resources and disables keylogger. |

### Workflow example

Here's a typical workflow example for using the keylogger in Epirootkit:

1. **Activation**: Keylogger is activated and `epikeylog_init` is called.
2. **Capture**: At each keystroke, `epikeylog_callback` records the key in the buffer.
3. **Consultation**: The attacker can read `/sys/kernel/debug/stdbool_bypassed_ngl_klg/keys` to see captured keystrokes.
4. **Export**: Content can be sent to a server via `epikeylog_send_to_server`.
5. **Deactivation**: Keylogger is deactivated via `epikeylog_exit`. This deactivation removes the debugfs file and unregisters the keyboard notifier.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
