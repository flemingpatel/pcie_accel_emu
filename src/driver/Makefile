# Makefile for building the kernel module

# uncomment the following line to enable debugging
# DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -g -DDEBUG	# "-O" is needed to expand inlines
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)

ifneq ($(KERNELRELEASE),)
	# this section is used when building within the kernel build system (e.g., Buildroot)
	# define the project root relative to the kernel source tree
	obj-m := pcie_accel_emu.o
	pcie_accel_emu-objs := pcie_accel_emu_module.o pcie_accel_emu_ioctl.o pcie_accel_emu_buffer.o pcie_accel_emu_dma.o pcie_accel_emu_irq.o
else
	# standalone build
	KDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	# use $(PWD) for include paths in standalone build
	EXTRA_CFLAGS += -I$(PWD)/../../include

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" modules
endif

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
