#!/bin/bash

# Define the path to IOMMU groups
iommu_path="/sys/kernel/iommu_groups"

# Check for command-line argument or prompt for input
group_number="$1"
[[ -z "$1" ]] && read -p "Enter IOMMU Group Number: " group_number

# Check if the user input is empty
if [[ -z "$group_number" ]]; then
    echo "Missing Group Number. Exiting."
    exit
fi

# Check if the input is a valid number
if ! [[ "$group_number" =~ ^[0-9]+$ ]]; then
    echo "Invalid IOMMU Group Number. Please enter a numeric value."
    exit
fi

# Path to the IOMMU group directory
iommu_group_path="/sys/kernel/iommu_groups/${group_number}/devices/"

# Check if the specified IOMMU group exists
if [[ ! -d "$iommu_group_path" ]]; then
    echo "IOMMU Group ${group_number} does not exist."
    exit
fi

echo -e "\nDevices in IOMMU Group ${group_number}:"

# List all devices in the specified IOMMU group by iterating through the device links
for device_link in ${iommu_group_path}*; do
    # Extract the PCI device ID from the symlink
    device_id=$(basename "$device_link")

    # Use lspci to display information about the device
    echo -n "    "
    lspci -nns "$device_id"
done

echo -e "\nNOTE: Unique IOMMU Group #'s does NOT mean isolated IOMMU TLBs! This is 'H/w Implementation Specific'..."

exit