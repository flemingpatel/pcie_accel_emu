#!/bin/bash
# shared.sh:  Shared definitions and utilities for Buildroot and QEMU scripts.
# Author:     Fleming Patel

# Exit on errors or unset variables
set -euo pipefail

# Function to display an error message and exit
error_exit() {
  printf "%s\n" "Error: $1" >&2
  exit 1
}

# Ensure required tools are available
require_tool() {
  command -v "$1" >/dev/null || error_exit "$1 is not installed. Please install it and try again"
}

# Check required tools
require_tool git

# Locate the repository root
REPOSITORY_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || error_exit "Not inside a Git repository")

# Submodules path
BUILDROOT_DIR="$REPOSITORY_ROOT/submodules/buildroot"
QEMU_DIR="$REPOSITORY_ROOT/submodules/qemu"

# Export common variables for scripts
export REPOSITORY_ROOT BUILDROOT_DIR QEMU_DIR
export -f error_exit require_tool

