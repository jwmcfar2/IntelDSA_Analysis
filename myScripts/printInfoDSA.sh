#!/bin/bash

# Device Overview
echo -e "\n### Device List and Overview:\n"
lspci | grep 0b25 | awk '{print $1}' | xargs -I {} lspci -vvv -s {}

echo ""

# Device sysfs directories
echo -e "### Sysfs Files Systems for DSA:\n"
ls -df /sys/bus/dsa/devices/dsa*

echo ""

# Linux Interfaces
echo -e "### Linux Interfaces:"
ls -df /sys/bus/dsa/devices/dsa* | xargs -I{} bash -c 'echo -e "\n{}:"; ls "{}"'

echo ""

# WQ Permissions
echo -e "### WQ Permissions:\n"
ls -la /dev/dsa/wq*

echo ""