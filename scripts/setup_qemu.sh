#!/bin/bash
# setup.sh : Setup QEMU for building pciemu
#

set -euo pipefail

# Display all commands (useful for debugging; remove `set -x` if not needed)
set -x

# Function to display an error message and exit
error_exit() {
  { set +x; } 2>/dev/null  # Disable tracing temporarily
  printf '%s\n' "Error: $1" >&2
  exit 1
}

# Ensure required tools are available
command -v git >/dev/null || error_exit "Git is not installed. Please install Git and try again."
command -v ln >/dev/null || error_exit "Symbolic link creation (ln) is unavailable."

# Repository information
REPOSITORY_DIR=$(git rev-parse --show-toplevel) || error_exit "Not inside a Git repository. Please run from the repo root."
REPOSITORY_NAME="pciemu"
QEMU_DIR="$REPOSITORY_DIR/submodules/qemu"

# Validate QEMU directory
if [[ ! -d "$QEMU_DIR" ]]; then
  error_exit "QEMU directory not found at $QEMU_DIR. Ensure the submodule is cloned and updated."
fi

[ -x "$QEMU_DIR/configure" ] || error_exit "QEMU configure script not found. Ensure QEMU submodule is initialized."

# Edit QEMU build files (check before appending)
KCONFIG_FILE="$QEMU_DIR/hw/misc/Kconfig"
MESON_FILE="$QEMU_DIR/hw/misc/meson.build"

if ! grep -q "source $REPOSITORY_NAME/Kconfig" "$KCONFIG_FILE"; then
  printf '%s\n' "source $REPOSITORY_NAME/Kconfig" >> "$KCONFIG_FILE"
fi

if ! grep -q "subdir('$REPOSITORY_NAME')" "$MESON_FILE"; then
  printf '%s\n' "subdir('$REPOSITORY_NAME')" >> "$MESON_FILE"
fi

# Create symbolic links
HW_SRC_LINK="$QEMU_DIR/hw/misc/"
INCLUDE_SRC="$REPOSITORY_DIR/include/hw/pciemu_hw.h"
INCLUDE_DEST="$REPOSITORY_DIR/src/hw/$REPOSITORY_NAME/pciemu_hw.h"

ln -sf "$REPOSITORY_DIR/src/hw/$REPOSITORY_NAME/" "$HW_SRC_LINK"
ln -sf "$INCLUDE_SRC" "$INCLUDE_DEST"

# Configure QEMU
cd "$QEMU_DIR" || error_exit "Failed to change directory to $QEMU_DIR."
./configure --enable-vde --enable-virtfs --target-list=arm-softmmu || error_exit "QEMU configuration failed."

{ set +x; } 2>/dev/null  # Disable tracing temporarily
printf '%s\n' "Setup finished. You may now build QEMU (cd $QEMU_DIR && make -j\$(nproc))"
