#!/bin/bash

set -e

myname="CXY"
# shellcheck disable=SC2034
mount_point="sda1"


sudo make clean
sudo make

sudo mount /dev/sda1 /mnt/
sudo mkdir -p /mnt/EFI/BOOT

sudo cp ../bootloader/BootLoader.efi /mnt/EFI/BOOT/BOOTx64.EFI
sudo cp ../bootloader/Shell.efi /mnt/EFI/BOOT/Shell.EFI
sudo cp kernel.bin /mnt/kernel.bin

sudo sync
sudo umount /mnt/

echo -e "\n         done, $myname"    # -e: enable \

