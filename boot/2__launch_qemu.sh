#!/bin/bash
# This script launches:
#   - In "setup" mode: the VMs boot from the provided ISO for installation.
#   - In "debug" mode: the VMs boot directly from the disk images.
#
# It uses the TAP interface (tap0) attached to the Linux bridge (br0) configured earlier.
#
# Usage (as non-root):
#   ./launch_qemu.sh setup
#   or
#   ./launch_qemu.sh debug

MODE="$1"

if [ -z "$MODE" ]; then
    echo "Usage: $0 [setup|debug]" >&2
    exit 1
fi

echo "============================================="
echo "         Launching VMs in $MODE mode         "
echo "============================================="

# Verify the TAP interface exists.
if ! ip link show tap0 &>/dev/null; then
    echo "[ERROR] TAP interface tap0 not found. Run setup_tap.sh first."
    exit 1
fi

# Verify the TAP interface exists.
if ! ip link show tap1 &>/dev/null; then
    echo "[ERROR] TAP interface tap1 not found. Run setup_tap.sh first."
    exit 1
fi

# Verify the br0 bridge exists.
if ! ip link show br0 &>/dev/null; then
    echo "[ERROR] Bridge br0 not found. Run setup_tap.sh first."
    exit 1
fi

# Set base directory and disk image paths (must match the setup_tap.sh values)
BASE_DIR="$(pwd)/rootkit_project"
ATTACKER_DISK="$BASE_DIR/attacker_disk.qcow2"
VICTIM_DISK="$BASE_DIR/victim_disk.qcow2"
bridge_info="bridge: br0, ip: 192.168.100.1/24"

# For "setup" mode, prompt for the ISO path and set installation options.
if [ "$MODE" = "setup" ]; then
    read -p "Enter the full path to your Ubuntu Server ISO: " ISO_PATH
    if [ -z "$ISO_PATH" ] || [ ! -f "$ISO_PATH" ]; then
        echo "[ERROR] Invalid ISO path."
        exit 1
    fi
    BOOT_OPTIONS="-boot d -cdrom $ISO_PATH"
    echo "[DEBUG] Setup mode selected: Using ISO $ISO_PATH for installation."

# In debug mode, do not use installation options.
elif [ "$MODE" = "debug" ]; then
    BOOT_OPTIONS=""
    echo "[DEBUG] Debug mode selected: Booting from existing disk images."

# Else, it sucks.
else
    echo "[ERROR] Invalid mode. Use 'setup' or 'debug'."
    exit 1
fi

# Attacker VM.
echo "[DEBUG] Launching Attacker VM..."
qemu-system-x86_64 \
  -enable-kvm \
  -m 4096 \
  $BOOT_OPTIONS \
  -hda "$ATTACKER_DISK" \
  -netdev tap,ifname=tap0,script=no,downscript=no,id=net0 \
  -device virtio-net-pci,netdev=net0,mac=52:54:00:AA:BB:CC \
  -name "Attacker VM" &
PID_ATTACKER=$!

sleep 2

# Victim VM.
echo "[DEBUG] Launching Victim VM..."
qemu-system-x86_64 \
  -enable-kvm \
  -m 4069 \
  $BOOT_OPTIONS \
  -hda "$VICTIM_DISK" \
  -netdev tap,ifname=tap1,script=no,downscript=no,id=net1 \
  -device virtio-net-pci,netdev=net1,mac=52:54:00:DD:EE:FF\
  -name "Victim VM" & 
PID_VICTIM=$!

# Print debug info including process IDs.
echo "============================================="
echo "           Debug Information                 "
echo "============================================="
echo "  Project Directory       : $(basename $BASE_DIR)"
echo "  Attacker Disk           : $(basename $ATTACKER_DISK)"
echo "  Victim Disk             : $(basename $VICTIM_DISK)"
echo "---------------------------------------------"
echo "  Attacker VM PID         : $PID_ATTACKER    "
echo "  Attacker MAC Address    : 52:54:00:AA:BB:CC"
echo "  Attacker IP             : 192.168.100.2    "
echo "  Attacker TAP Interface  : tap0             "
echo "  Attacker Username       : attacker         "
echo "  Attacker Password       : attacker         "
echo "---------------------------------------------"
echo "  Victim VM PID           : $PID_VICTIM      "
echo "  Victim MAC Address      : 52:54:00:DD:EE:FF"
echo "  Victim IP               : 192.168.100.3    "
echo "  Victim TAP Interface    : tap1             "
echo "  Victim Username         : victim           "
echo "  Victim Password         : victim           "
echo "---------------------------------------------"
echo "  Bridge Name             : br0              "
echo "  Bridge IP               : 192.168.100.1    "
echo "============================================="
echo "           Completed Launching VMs           "
echo "============================================="