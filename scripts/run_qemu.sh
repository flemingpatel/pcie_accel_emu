#!/bin/bash
# run_qemu.sh:  Run QEMU virt instance using custom qemu binary for given buildroot generated image
# Author:       Fleming Patel

source "$(dirname "$0")/shared.sh"


KERNEL_IMAGE="$BUILDROOT_DIR/output/images/Image"
ROOTFS_IMAGE="$BUILDROOT_DIR/output/images/rootfs.ext4"

if [[ ! -f "$KERNEL_IMAGE" ]]; then
    error_exit "Kernel image not found at $KERNEL_IMAGE. Ensure setup_buildroot completed successfully"
fi

if [[ ! -f "$ROOTFS_IMAGE" ]]; then
    error_exit "Root filesystem not found at $ROOTFS_IMAGE. Ensure setup_buildroot completed successfully"
fi

[ -x "$QEMU_DIR/build/qemu-system-aarch64" ] || error_exit "QEMU binary not found; Ensure setup_qemu completed successfully"

cd "$QEMU_DIR/build" || error_exit "Failed to change directory to $QEMU_DIR/build"
./qemu-system-aarch64 \
    -M virt  \
    -cpu cortex-a53 -nographic -smp 1 \
    -kernel "$KERNEL_IMAGE" \
    -append "rootwait root=/dev/vda console=ttyAMA0" \
    -drive file="$ROOTFS_IMAGE",if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 -device virtio-rng-pci \
    -device pciemu,id=pciemu1 || error_exit "QEMU startup failed"
