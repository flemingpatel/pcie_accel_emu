#!/bin/bash
# setup_qemu.sh:  Build custom QEMU binary that has custom pciemu device
# Author:         Fleming Patel

# Source shared definitions
source "$(dirname "$0")/shared.sh"

# Parse options
VERBOSE=0
while [[ $# -gt 0 ]]; do
  case $1 in
    -v|--verbose) VERBOSE=1 ;;
    *) error_exit "Unknown option: $1" ;;
  esac
  shift
done

# Enable verbose mode if requested
if [[ $VERBOSE -eq 1 ]]; then
  set -x
fi

# Check prerequisites
require_tool ln

# Ensure submodule is initialized
git submodule update --init --recursive

# Validate QEMU directory
if [[ ! -d "$QEMU_DIR" ]]; then
  error_exit "QEMU directory not found at $QEMU_DIR; Ensure the submodule is cloned"
fi

# QEMU build files
KCONFIG_FILE="$QEMU_DIR/hw/misc/Kconfig"
MESON_FILE="$QEMU_DIR/hw/misc/meson.build"
REPOSITORY_NAME="pciemu"

[ -x "$QEMU_DIR/configure" ] || error_exit "QEMU configure script not found; Ensure QEMU submodule is initialized"

# Edit QEMU build files if necessary
if ! grep -q "source $REPOSITORY_NAME/Kconfig" "$KCONFIG_FILE"; then
  echo "Adding pciemu Kconfig"
  echo "source $REPOSITORY_NAME/Kconfig" >> "$KCONFIG_FILE"
fi

if ! grep -q "subdir('$REPOSITORY_NAME')" "$MESON_FILE"; then
  echo "Adding pciemu meson subdir"
  echo "subdir('$REPOSITORY_NAME')" >> "$MESON_FILE"
fi

# Create symbolic links
ln -sf "$REPOSITORY_ROOT/src/hw/$REPOSITORY_NAME/" "$QEMU_DIR/hw/misc/"
ln -sf "$REPOSITORY_ROOT/include/hw/pciemu_hw.h" "$REPOSITORY_ROOT/src/hw/$REPOSITORY_NAME/pciemu_hw.h"

# Configure QEMU
cd "$QEMU_DIR" || error_exit "Failed to change directory to $QEMU_DIR"
./configure --target-list=aarch64-softmmu \
  --disable-bsd-user \
  --disable-guest-agent \
  --disable-gtk \
  --disable-werror \
  --enable-vde \
  --enable-virtfs || error_exit "QEMU configuration failed"

# Build qemu
make -j$(nproc) || error_exit "QEMU build failed"
