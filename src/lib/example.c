/*
 * example.c: User-space example that uses libvai and communicates with the device via driver ioctl
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "lib/libvai.h"
#include <sys/mman.h>

/* TODO find dynamically */
#define PCIEMU_DEVICE_PATH "/dev/pcie_accel_emu/d0b0d3f0_bar0"

int main()
{
	return EXIT_SUCCESS;
}
