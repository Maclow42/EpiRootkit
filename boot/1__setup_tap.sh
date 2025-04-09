#!/bin/bash
# This script performs:
#   1. Creation of the project directory.
#   2. Creation of disk images for the attacker and victim VMs.
#   3. Setting correct ownership/permissions for non-root user access.
#   4. Creation and configuration of a Linux bridge (br0) with an IP.
#   5. Creation of a TAP interface (tap0) and attaching it to the bridge.
#
# Usage (run as root):
#   sudo ./setup_tap.sh

# Ensure the script is run as root.
if [ "$(id -u)" -ne 0 ]; then
    echo "[ERROR] This script must be run as root."
    exit 1
fi

# Determine the non-root user.
if [ -n "$SUDO_USER" ]; then
    NON_ROOT_USER="$SUDO_USER"
else
    read -p "Enter a non-root username to own the TAP device and disk images: " NON_ROOT_USER
fi

if ! id "$NON_ROOT_USER" &>/dev/null; then
    echo "[ERROR] User '$NON_ROOT_USER' does not exist."
    exit 1
fi

echo "============================================="
echo "            Privileged Setup Phase           "
echo "============================================="

# 1. Create the project directory.
BASE_DIR="$(pwd)/rootkit_project"
if [ ! -d "$BASE_DIR" ]; then
    echo "[DEBUG] Creating project directory rootkit_project..."
    mkdir -p "$BASE_DIR" || { echo "[ERROR] Error: Cannot create $BASE_DIR."; exit 1; }
else
    echo "[DEBUG] Project directory already exists at $BASE_DIR."
fi

# 2. Create disk images for the VMs.
ATTACKER_DISK="$BASE_DIR/attacker_disk.qcow2"
VICTIM_DISK="$BASE_DIR/victim_disk.qcow2"

if [ ! -f "$ATTACKER_DISK" ]; then
    echo "[DEBUG] Creating attacker disk image..."
    qemu-img create -f qcow2 "$ATTACKER_DISK" 10G &>/dev/null || { echo "[ERROR] Error creating attacker disk."; exit 1; }
else
    echo "[DEBUG] Attacker disk image already exists..."
fi

if [ ! -f "$VICTIM_DISK" ]; then
    echo "[DEBUG] Creating victim disk image..."
    qemu-img create -f qcow2 "$VICTIM_DISK" 10G &>/dev/null || { echo "[ERROR]  Error creating victim disk."; exit 1; }
else
    echo "[DEBUG] Victim disk image already exists..."
fi

# Fix permissions so non-root user can access the disk images.
chown "$NON_ROOT_USER":"$NON_ROOT_USER" "$ATTACKER_DISK" "$VICTIM_DISK"
chmod 664 "$ATTACKER_DISK" "$VICTIM_DISK"
echo "[DEBUG] Disk images ownership set to $NON_ROOT_USER."

# 3. Create and configure the bridge (br0).
BRIDGE_NAME="br0"
BRIDGE_IP="192.168.100.1/24"
if ip link show "$BRIDGE_NAME" &>/dev/null; then
    echo "[DEBUG] Bridge $BRIDGE_NAME already exists."
else
    echo "[DEBUG] Creating Linux bridge $BRIDGE_NAME..."
    ip link add name "$BRIDGE_NAME" type bridge || { echo "[ERROR] Error creating bridge."; exit 1; }
    echo "[DEBUG] Assigning IP $BRIDGE_IP to bridge $BRIDGE_NAME..."
    ip addr add "$BRIDGE_IP" dev "$BRIDGE_NAME" || { echo "[ERROR] Error assigning IP."; exit 1; }
fi
ip link set "$BRIDGE_NAME" up
echo "[DEBUG] Bridge $BRIDGE_NAME is up."


# Create TAP interface tap0 for the attacker VM if it does not exist.
if ip link show tap0 &>/dev/null; then
    echo "[DEBUG] TAP interface tap0 already exists."
else
    echo "[DEBUG] Creating TAP interface tap0 for user $NON_ROOT_USER..."
    ip tuntap add dev tap0 mode tap user "$NON_ROOT_USER" || { echo "[ERROR] Error creating TAP interface tap0."; exit 1; }
fi

# Create TAP interface tap1 for the victim VM if it does not exist.
if ip link show tap1 &>/dev/null; then
    echo "[DEBUG] TAP interface tap1 already exists."
else
    echo "[DEBUG] Creating TAP interface tap1 for user $NON_ROOT_USER..."
    ip tuntap add dev tap1 mode tap user "$NON_ROOT_USER" || { echo "[ERROR] Error creating TAP interface tap1."; exit 1; }
fi

# Attach both TAP interfaces to the bridge.
echo "[DEBUG] Attaching tap0/1 to bridge $BRIDGE_NAME..."
ip link set tap0 master "$BRIDGE_NAME"
ip link set tap1 master "$BRIDGE_NAME"

# Bring up the TAP interfaces.
ip link set tap0 up
ip link set tap1 up

echo "============================================="
echo "Project Directory    : $BASE_DIR"
echo "Attacker Disk Image  : $(basename $ATTACKER_DISK) (10G)"
echo "Victim Disk Image    : $(basename $VICTIM_DISK) (10G)"
echo "Bridge               : $BRIDGE_NAME with IP $BRIDGE_IP"
echo "TAP Interface        : tap0, tap1 attached to $BRIDGE_NAME"
echo "============================================="
echo "            Completed Setup Phase            "
echo "============================================="