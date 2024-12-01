# pcie_accel_emu.mk - Buildroot package makefile for the pcie_accel_emu driver

PCIE_ACCEL_EMU_VERSION = 1.0

# define the project root relative to Buildroot's top directory
PROJECT_ROOT = $(BR2_EXTERNAL)/..

# location of the driver source code
PCIE_ACCEL_EMU_SITE = $(PROJECT_ROOT)/src/driver
PCIE_ACCEL_EMU_SITE_METHOD = local

# dependencies
PCIE_ACCEL_EMU_DEPENDENCIES = linux

# license
PCIE_ACCEL_EMU_LICENSE = GPLv2

# specify the modules to build
PCIE_ACCEL_EMU_MODULES = pcie_accel_emu

# pass additional include directory
PCIE_ACCEL_EMU_MODULE_MAKE_OPTS = EXTRA_CFLAGS+="-I$(PROJECT_ROOT)/include -DDEBUG=y"

$(eval $(kernel-module))
$(eval $(generic-package))
