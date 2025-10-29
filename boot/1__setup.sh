#!/bin/bash

ROOT_PATH="./"
if [ -n "$1" ]; then
  ROOT_PATH="$1"
fi

BASE_DIR="$ROOT_PATH/vms"
BRIDGE_NAME="br0"
BRIDGE_IP="192.168.100.1/24"

# Ensure the script is run as root.
if [ "$(id -u)" -ne 0 ]; then
    echo "[ERROR] This script must be run as root."
    exit 1
fi

echo "================================================="
echo "            Privileged Setup Phase               "
echo "================================================="
echo ""
echo "IMPORTANT: You must prepare QEMU VMs in advance!"
echo ""
echo "Required VM specifications:"
echo "  - OS: A Linux 6 kernel based distribution (tested on Ubuntu 24.10)"
echo "  - Disk format: QCOW2"
echo "  - Minimum specs:"
echo "    * RAM: 2GB (4GB recommended)"
echo "    * Disk: 10GB minimum"
echo ""
echo "Required files in '$BASE_DIR':"
echo "  - attacker_disk.qcow2 (Attacker VM disk)"
echo "  - victim_disk.qcow2 (Victim VM disk)"
echo ""
echo "Network configuration:"
echo "  - Bridge: br0 (192.168.100.1/24)"
echo "  - Attacker IP: 192.168.100.2"
echo "  - Victim IP: 192.168.100.3"
echo ""
echo "================================================="
echo ""

# 1. Create the project directory.
if [ ! -d "$BASE_DIR" ]; then
    echo "[DEBUG] Creating project directory $BASE_DIR..."
    mkdir -p "$BASE_DIR" || { echo "[ERROR] Error: Cannot create $BASE_DIR."; exit 1; }
else
    echo "[DEBUG] Project directory already exists at $BASE_DIR."
fi

# 2. Check if VM disk images exist
if [ ! -f "$BASE_DIR/victim_disk.qcow2" ]; then
    echo "[ERROR] victim_disk.qcow2 not found in $BASE_DIR!"
    echo "[ERROR] Please create or place your victim VM disk image in this directory."
    echo "[ERROR] The VM should be running Ubuntu 24.10 with static IP 192.168.100.3"
    exit 1
fi

if [ ! -f "$BASE_DIR/attacker_disk.qcow2" ]; then
    echo "[ERROR] attacker_disk.qcow2 not found in $BASE_DIR!"
    echo "[ERROR] Please create or place your attacker VM disk image in this directory."
    echo "[ERROR] The VM should be running Ubuntu 24.10 with static IP 192.168.100.2"
    exit 1
fi

echo "[DEBUG] VM disk images found:"
echo "  - $BASE_DIR/victim_disk.qcow2"
echo "  - $BASE_DIR/attacker_disk.qcow2"

# 3. Set permissions
chmod 777 "$BASE_DIR"
chmod 666 "$BASE_DIR"/*.qcow2

# 4. Create and configure the bridge (br0).
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

# 5. Set up iptables rules to allow traffic on the bridge.
echo "[DEBUG] Setting up iptables rules..."
iptables -A INPUT -i br0 -p tcp --dport 4242 -j ACCEPT
iptables -I INPUT  -p udp --dport 53 -j ACCEPT
iptables -I OUTPUT -p udp --sport 53 -j ACCEPT

sysctl -w net.ipv4.ip_forward=1

# Trying to get the connected interface of user lol (a bit ugly I now)
IFACE=$(ip -o link show | awk '/state UP/ && /LOWER_UP/ {print $2}' | sed 's/://g' | grep -Ev '^(lo|br0|docker|tap|virbr)' | head -n 1)
if [ -z "$IFACE" ]; then
    echo "[ERROR] No valid interface... Aborting."
    exit 1
fi

iptables -A FORWARD -i "$IFACE" -o br0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i br0 -o "$IFACE" -j ACCEPT
iptables -t nat -A POSTROUTING -s 192.168.100.0/24 -o "$IFACE" -j MASQUERADE

# 6. Create TAP interface tap0 for the attacker VM if it does not exist.
if ip link show tap0 &>/dev/null; then
    echo "[DEBUG] TAP interface tap0 already exists."
else
    echo "[DEBUG] Creating TAP interface tap0..."
    ip tuntap add dev tap0 mode tap || { echo "[ERROR] Error creating TAP interface tap0."; exit 1; }
fi

# 7. Create TAP interface tap1 for the victim VM if it does not exist.
if ip link show tap1 &>/dev/null; then
    echo "[DEBUG] TAP interface tap1 already exists."
else
    echo "[DEBUG] Creating TAP interface tap1..."
    ip tuntap add dev tap1 mode tap || { echo "[ERROR] Error creating TAP interface tap1."; exit 1; }
fi

# 8. Attach both TAP interfaces to the bridge.
echo "[DEBUG] Attaching tap0/1 to bridge $BRIDGE_NAME..."
ip link set tap0 master "$BRIDGE_NAME"
ip link set tap1 master "$BRIDGE_NAME"

# 9. Bring up the TAP interfaces.
ip link set tap0 up
ip link set tap1 up

echo "================================================="
echo "Project Directory   : $BASE_DIR"
echo "Attacker Disk Image : attacker_disk.qcow2 (10G)"
echo "Victim Disk Image   : victim_disk.qcow2 (10G)"
echo "Bridge              : $BRIDGE_NAME with IP $BRIDGE_IP"
echo "TAP Interface       : tap0, tap1 attached to $BRIDGE_NAME"
echo "================================================="
echo "            Completed Setup Phase                "
echo "================================================="
