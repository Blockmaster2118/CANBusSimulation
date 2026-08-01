#!/bin/bash
# Sets up a virtual CAN interface called vcan0.
# Run this once per reboot, on a real Linux machine or VM (not a
# restricted container - it needs kernel module + network admin access).
#
#   chmod +x setup_vcan.sh
#   sudo ./setup_vcan.sh

set -e

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan 2>/dev/null || echo "vcan0 already exists"
sudo ip link set up vcan0

echo "vcan0 is up. Try: candump vcan0"
