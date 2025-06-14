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

# 1. Create the project directory.
if [ ! -d "$BASE_DIR" ]; then
    echo "[DEBUG] Creating project directory $BASE_DIR..."
    mkdir -p "$BASE_DIR" || { echo "[ERROR] Error: Cannot create $BASE_DIR."; exit 1; }
else
    echo "[DEBUG] Project directory already exists at $BASE_DIR."
fi

# 2. Get the disk images zip for the attacker and victim VMs.
if [ ! -f "$BASE_DIR/victim_disk.zip" ] && [ ! -f "$BASE_DIR/victim_disk.qcow2" ]; then
    echo "[DEBUG] Getting victim disk image…" 
    wget http://109.30.250.114/victim_disk.zip -P "$BASE_DIR"
    echo "[DEBUG] victim_disk.zip complete"
else
    echo "[DEBUG] victim_disk.zip already exists."
fi

if [ ! -f "$BASE_DIR/attacker_disk.zip" ] && [ ! -f "$BASE_DIR/attacker_disk.qcow2" ]; then
    echo "[DEBUG] Getting attacker disk image…" 
    wget http://109.30.250.114/attacker_disk.zip -P "$BASE_DIR"
    echo "[DEBUG] attacker_disk.zip complete"
else
    echo "[DEBUG] attacker_disk.zip already exists."
fi

# 3. Unzip the disk images.
if [ ! -f "$BASE_DIR/victim_disk.qcow2" ]; then
    echo "[DEBUG] Unzipping victim disk image…" 
    unzip -o "$BASE_DIR/victim_disk.zip" -d "$BASE_DIR" || { echo "[ERROR] Error unzipping victim disk image."; exit 1; }
    rm -f "$BASE_DIR/victim_disk.zip"
else
    echo "[DEBUG] Victim disk image already unzipped."
fi

if [ ! -f "$BASE_DIR/attacker_disk.qcow2" ]; then
    echo "[DEBUG] Unzipping attacker disk image…" 
    unzip -o "$BASE_DIR/attacker_disk.zip" -d "$BASE_DIR" || { echo "[ERROR] Error unzipping attacker disk image."; exit 1; }
    rm -f "$BASE_DIR/attacker_disk.zip"
else
    echo "[DEBUG] Attacker disk image already unzipped."
fi

# 4. Permissions... need to test again
chmod 777 "$BASE_DIR"
chmod 777 "$BASE_DIR/*

# 5. Create and configure the bridge (br0).
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

# 6. Set up iptables rules to allow traffic on the bridge.
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

# 7. Create TAP interface tap0 for the attacker VM if it does not exist.
if ip link show tap0 &>/dev/null; then
    echo "[DEBUG] TAP interface tap0 already exists."
else
    echo "[DEBUG] Creating TAP interface tap0..."
    ip tuntap add dev tap0 mode tap || { echo "[ERROR] Error creating TAP interface tap0."; exit 1; }
fi

# 8. Create TAP interface tap1 for the victim VM if it does not exist.
if ip link show tap1 &>/dev/null; then
    echo "[DEBUG] TAP interface tap1 already exists."
else
    echo "[DEBUG] Creating TAP interface tap1..."
    ip tuntap add dev tap1 mode tap || { echo "[ERROR] Error creating TAP interface tap1."; exit 1; }
fi

# 9. Attach both TAP interfaces to the bridge.
echo "[DEBUG] Attaching tap0/1 to bridge $BRIDGE_NAME..."
ip link set tap0 master "$BRIDGE_NAME"
ip link set tap1 master "$BRIDGE_NAME"

# 10. Bring up the TAP interfaces.
ip link set tap0 up
ip link set tap1 up

echo "================================================="
echo "Project Directory   : $BASE_DIR"
echo "Attacker Disk Image : victim_disk.qcow2 (10G)"
echo "Victim Disk Image   : attacker_disk.qcow2 (10G)"
echo "Bridge              : $BRIDGE_NAME with IP $BRIDGE_IP"
echo "TAP Interface       : tap0, tap1 attached to $BRIDGE_NAME"
echo "================================================="
echo "            Completed Setup Phase                "
echo "================================================="
