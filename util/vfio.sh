#!/bin/sh -e
# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# Binds devices to the vfio-pci driver. Requires root permissions.
#
# usage: vfio DBDF [DBDF ...]
#
#   DBDF: PCI Domain Bus Device Function identifier

for bdf in "$@"; do
    # Unbind the current driver
    if [ -e /sys/bus/pci/devices/"$bdf"/driver/unbind ]; then
        echo "$bdf" > /sys/bus/pci/devices/"$bdf"/driver/unbind
    fi

    # Bind the vfio-pci driver
    vendor_id=$(cat /sys/bus/pci/devices/"$bdf"/vendor)
    device_id=$(cat /sys/bus/pci/devices/"$bdf"/device)
    echo "$vendor_id" "$device_id" > /sys/bus/pci/drivers/vfio-pci/new_id

    # Add RW permissions for the vfio group to the device's iommu group
    iommu_group=$(basename "$(readlink -f /sys/bus/pci/devices/"$bdf"/iommu_group/)")
    chgrp vfio /dev/vfio/"$iommu_group"
    chmod g+rw /dev/vfio/"$iommu_group"

    # Show which driver is now bound to the PCI device
    driver=$(basename "$(readlink -f /sys/bus/pci/devices/"$bdf"/driver)")
    echo "$bdf" "$driver"
done

