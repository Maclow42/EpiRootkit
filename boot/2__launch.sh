#!/bin/bash

BASE_DIR="./vms"
ATTACKER_DISK="$BASE_DIR/attacker_disk.qcow2"
VICTIM_DISK="$BASE_DIR/victim_disk.qcow2"
bridge_info="bridge: br0, ip: 192.168.100.1/24"

echo "================================================="
echo "                  Launching VMs                  "
echo "================================================="

# Verify the TAP interface exists.
if ! ip link show tap0 &>/dev/null; then
    echo "[ERROR] TAP interface tap0 not found. Run 1__setup_tap.sh first."
    exit 1
fi

# Verify the TAP interface exists.
if ! ip link show tap1 &>/dev/null; then
    echo "[ERROR] TAP interface tap1 not found. Run 1__setup_tap.sh first."
    exit 1
fi

# Verify the br0 bridge exists.
if ! ip link show br0 &>/dev/null; then
    echo "[ERROR] Bridge br0 not found. Run 1__setup_tap.sh first."
    exit 1
fi

# Attacker VM
echo "[DEBUG] Launching Attacker VM..."
qemu-system-x86_64 \
  -enable-kvm \
  -m 4096 \
  -hda "$ATTACKER_DISK" \
  -netdev tap,ifname=tap0,script=no,downscript=no,id=net0 \
  -device virtio-net-pci,netdev=net0,mac=52:54:00:AA:BB:CC \
  -name "Attacker VM" &
PID_ATTACKER=$!

sleep 2

# Victim VM
echo "[DEBUG] Launching Victim VM..."
qemu-system-x86_64 \
  -enable-kvm \
  -m 4069 \
  -hda "$VICTIM_DISK" \
  -netdev tap,ifname=tap1,script=no,downscript=no,id=net1 \
  -device virtio-net-pci,netdev=net1,mac=52:54:00:DD:EE:FF\
  -name "Victim VM" & 
PID_VICTIM=$!

# Print debug info including process IDs.
echo "================================================="
echo "                   Information                   "
echo "================================================="
echo "  Project Directory       : $(basename $BASE_DIR)"
echo "  Attacker Disk           : $(basename $ATTACKER_DISK)"
echo "  Victim Disk             : $(basename $VICTIM_DISK)"
echo "-------------------------------------------------"
echo "  Attacker VM PID         : $PID_ATTACKER        "
echo "  Attacker MAC Address    : 52:54:00:AA:BB:CC    "
echo "  Attacker IP             : 192.168.100.2        "
echo "  Attacker TAP Interface  : tap0                 "
echo "  Attacker Username       : attacker             "
echo "  Attacker Password       : attacker             "
echo "-------------------------------------------------"
echo "  Victim VM PID           : $PID_VICTIM          "
echo "  Victim MAC Address      : 52:54:00:DD:EE:FF    "
echo "  Victim IP               : 192.168.100.3        "
echo "  Victim TAP Interface    : tap1                 "
echo "  Victim Username         : victim               "
echo "  Victim Password         : victim               "
echo "-------------------------------------------------"
echo "  Bridge Name             : br0                  "
echo "  Bridge IP               : 192.168.100.1        "
echo "================================================="
echo "             Completed Launching VMs             "
echo "================================================="
