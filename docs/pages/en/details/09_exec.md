\page exec Command Execution

\tableofcontents

# 🧑‍💻 Userland

In Epirootkit, userland command execution is handled by the userland.c module. The benefit is immediate: it allows executing shell commands in root mode on the machine, which opens total control over the system.

> **Warning:** This module doesn't allow obtaining a remote interactive shell, but only executing commands like shell scripts. However, as explained in the [Reverse Shell](#reverse-shell-doc) section, this functionality is also indeed used in Epirootkit to launch a userland reverse shell with `socat`.

## Output Capture

# 📋 Specifications

Both stdout and stderr are captured and returned to the attacker.

In our context of remote command execution by an attacker, the `userland.c` module had to meet several criteria:

- **Shell command execution**: The module must be able to execute any shell command, as if the user were connected as root.## Silent Mode

- **Result retrieval**: Command execution results must be returned to the attacker, thus allowing retrieval of standard output, errors, and return code.

- **Manual redirection handling**: If the user manually specifies redirections (for example, `> output.txt`), the module must handle them correctly so they don't conflict with the previously mentioned result retrieval.Use the `-s` flag for silent execution (returns only exit code).

- **Blocking command handling**: Command execution must include a timeout to avoid indefinite blocking. If a command exceeds the allotted time, it must be interrupted and an error message must be sent to the attacker. This is necessary since when sending a command, the attack server waits for a response from the module, and if the command is blocking, the server will never receive a response and will wait indefinitely.

## Timeout

# ⚙️ Implementation

Commands have a timeout to prevent hanging.

The `userland.c` module relies on using the Linux kernel's `call_usermodehelper` API to execute shell commands from kernel space. This allows Epirootkit to trigger execution of any command as root, with fine behavior management (redirection, timeout, etc.). Here's how each specification requirement is implemented:

## Shell commands

The main function, `exec_str_as_command_with_timeout()`, receives a user command string (`user_cmd`), applies various preparations, then triggers its execution using the `call_usermodehelper_exec()` function via:

```c
char *argv[] = { "/bin/sh", "-c", (char *)cmd_str, NULL };
```

This is equivalent to doing:
```sh
/bin/sh -c "<command>"
```

But this command is launched from kernel space.

## Results

Standard output (`stdout`) and error (`stderr`) retrieval is ensured by globally defined redirection files, typically:
```c
#define STDOUT_FILE HIDDEN_DIR_PATH "/std.out"
#define STDERR_FILE HIDDEN_DIR_PATH "/std.err"
```

If the user hasn't included redirections themselves (`>` or `2>`), the module manually redirects these flows to the above files. This allows the attacker to later retrieve execution results. The management code is located in the `build_full_command()` function, which assembles the final command based on redirection state detected by `detect_redirections()`.

## Manual redirections

The `detect_redirections()` function scrutinizes the user command to determine if explicit redirections are already present. It spots `>` (stdout) and `2>` (stderr) patterns. If the user has already handled redirections, the module doesn't add its own, avoiding any conflicting redundancy:

```c
*redirect_stderr = (strstr(cmd, "2>") != NULL);
*redirect_stdout = (strstr(cmd, ">") != strstr(cmd, "2>") && strstr(cmd, ">") != NULL);
```

## Blocking commands

An essential element to avoid blocking is the timeout mechanism. The module builds a `timeout` command prefix thanks to `build_timeout_prefix()`:

```c
timeout --signal=SIGKILL --preserve-status <seconds>
```

This prefix is automatically inserted into the final shell command. Thus, if the command doesn't finish within the allotted time, it's killed with `SIGKILL`, and the return code is preserved. Example of generated command:

```sh
timeout --signal=SIGKILL --preserve-status 5 ls -la > HIDDEN_DIR_PATH"/std.out" 2> HIDDEN_DIR_PATH"/std.err"
```

## Summary

1. Command cleaning: removal of leading spaces with `trim_leading_whitespace()`.
2. Redirection analysis: detection of `>` and `2>` via `detect_redirections()`.
3. Timeout prefix: if timeout is requested, `build_timeout_prefix()` builds the command.
4. Complete command construction: via `build_full_command()`, integrating:
   - Timeout
   - Redirections (automatic or manual)
5. Execution: triggered via `call_usermodehelper_exec()`.

## Advantages of this approach

- Clear separation of responsibilities: each aspect (timeout, redirection, parsing) is handled by a specific function.
- Robustness against tricky commands: explicit redirections are respected.
- Zombie prevention: thanks to `timeout`, no risk of leaving blocking/infinite processes in background threads.
- Transparent root execution: command executes in privileged kernel context, offering total system control.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
