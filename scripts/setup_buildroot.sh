#!/bin/bash
# setup_buildroot.sh: Setup Buildroot for generating kernel image
# Author:             Fleming Patel

# Source shared definitions
source "$(dirname "$0")/shared.sh"

# Parse options
VERBOSE=0
ACTION=""
while [[ $# -gt 0 ]]; do
  case $1 in
    -v|--verbose) VERBOSE=1 ;;
    save_config) ACTION="save_config" ;;
    *) error_exit "Unknown option: $1" ;;
  esac
  shift
done

# Enable verbose mode if requested
if [[ $VERBOSE -eq 1 ]]; then
  set -x
fi

# Ensure submodule is initialized
git submodule update --init --recursive

# Absolute paths for configurations
DEFAULT_DEFCONFIG="$BUILDROOT_DIR/configs/qemu_aarch64_virt_defconfig"
MODIFIED_DEFCONFIG="$REPOSITORY_ROOT/base_external/configs/pcie_accel_emu_qemu_defconfig"

# Buildroot configuration
cd "$BUILDROOT_DIR" || error_exit "Failed to change directory to $BUILDROOT_DIR"
if [[ ! -e .config ]]; then
  echo "Missing Buildroot configuration file"
  if [[ -e "$MODIFIED_DEFCONFIG" ]]; then
    echo "Using custom configuration from $MODIFIED_DEFCONFIG"
    make defconfig BR2_EXTERNAL="$REPOSITORY_ROOT/base_external" BR2_DEFCONFIG="$MODIFIED_DEFCONFIG"
  else
    echo "Using default configuration from $DEFAULT_DEFCONFIG"
    echo "Run ./setup_buildroot.sh save_config to save this as your default configuration in $MODIFIED_DEFCONFIG"
    echo "Add packages as needed to complete the installation, re-running ./setup_buildroot.sh save_config"
    echo "You may now build Buildroot (./setup_buildroot.sh)"
    make defconfig BR2_DEFCONFIG="$DEFAULT_DEFCONFIG"
  fi
else
  # Save configuration if requested
  if [[ "$ACTION" == "save_config" ]]; then
    echo "Saving current configuration as user default configuration in $MODIFIED_DEFCONFIG"
    mkdir -p "$(dirname "$MODIFIED_DEFCONFIG")"
    # save config to user modified location
    make savedefconfig BR2_DEFCONFIG="$MODIFIED_DEFCONFIG"
  else
    echo "Building using existing configuration"
    echo "To force update, delete .config or make changes using make menuconfig, save_config and build again"
    make BR2_EXTERNAL="$REPOSITORY_ROOT/base_external"
  fi
fi

