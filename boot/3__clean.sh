#!/bin/bash

# Check for root privileges.
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root." >&2
    exit 1
fi

echo "================================================="
echo "              Starting Host Cleanup              "
echo "================================================="

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

echo "[DEBUG] Killing vms..."
kill $PID_VICTIM $PID_ATTACKER || true

# Ask if the user wants to remove project directory.
BASE_DIR="./vms"
if [ -d "$BASE_DIR" ]; then
    read -p "Do you also want to remove the project directory ($BASE_DIR)? [y/N] " answer
    if [[ "$answer" =~ ^[Yy]$ ]]; then
        echo "[DEBUG] Removing project directory $BASE_DIR..."
        rm -rf "$BASE_DIR"
        echo "[DEBUG] Project directory removed."
    else
        echo "[DEBUG] Project directory preserved."
    fi
else
    echo "[DEBUG] Project directory not found."
fi
echo "================================================="
echo "                Cleanup Completed                "
echo "================================================="
