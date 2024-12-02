/*
 * example.c: User-space example that uses libvai and communicates with the device via driver ioctl
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "lib/libvai.h"

/* is there a way to find dynamically (TODO) */
#define PCIEMU_DEVICE_PATH "/dev/pcie_accel_emu/d0b0d3f0_bar0"

int main()
{
	int fd;
	/* open PCIEMU device */
	fd = open(PCIEMU_DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("failed to open PCIEMU device");
		return EXIT_FAILURE;
	}

	/* sample test */
	vai_load_model(fd, 0, 0, 0);
	close(fd);

	return 0;
}
