#!/bin/bash
# This script removes the TAP interfaces (tap0 and tap1), the bridge (br0),
# and optionally the project directory (rootkit_project) that were created during setup.
#
# Usage (as root):
#   sudo ./3__clean.sh

# Check for root privileges.
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root." >&2
    exit 1
fi

echo "============================================="
echo "            Starting Host Cleanup            "
echo "============================================="

# Remove TAP interfaces tap0 and tap1.
for tap in tap0 tap1; do
    if ip link show "$tap" &>/dev/null; then
        echo "[DEBUG] Bringing down TAP interface $tap..."
        ip link set "$tap" down
        echo "[DEBUG] Deleting TAP interface $tap..."
        ip tuntap del dev "$tap" mode tap
    else
        echo "[DEBUG] TAP interface $tap not found."
    fi
done

# Remove the bridge interface br0.
BRIDGE="br0"
if ip link show "$BRIDGE" &>/dev/null; then
    echo "[DEBUG] Bringing down bridge $BRIDGE..."
    ip link set "$BRIDGE" down
    echo "[DEBUG] Deleting bridge $BRIDGE..."
    ip link del "$BRIDGE" type bridge
else
    echo "[DEBUG] Bridge $BRIDGE not found."
fi

# Ask if the user also wants to remove the project directory.
PROJECT_DIR="$(pwd)/rootkit_project"
if [ -d "$PROJECT_DIR" ]; then
    read -p "Do you also want to remove the project directory ($PROJECT_DIR)? [y/N] " answer
    if [[ "$answer" =~ ^[Yy]$ ]]; then
        echo "[DEBUG] Removing project directory $PROJECT_DIR..."
        rm -rf "$PROJECT_DIR"
        echo "[DEBUG] Project directory removed."
    else
        echo "[DEBUG] Project directory preserved."
    fi
else
    echo "[DEBUG] Project directory not found."
fi

echo "============================================="
echo "              Cleanup Completed              "
echo "============================================="
