/*
* pciemu_hw_sg.h: Provides hardware scatter/gather descriptor structure
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2024 Fleming Patel
*/

#ifndef PCIEMU_HW_SG_H
#define PCIEMU_HW_SG_H

#ifdef __KERNEL__
/*
 * when building inside the Linux kernel driver,
 * we rely on kernel’s includes
 */
#include <linux/types.h>
#else
/*
 * when building in QEMU or any other non-kernel context,
 * we use the standard C integer headers.
 */
#include <stdint.h>
#endif

/**
 * @brief Hardware scatter/gather descriptor. Layout must match what the device expects.
 * @addr DMA address of the segment
 * @len Length in bytes
 * @flags flags (bit 0 = last descriptor)
 */
struct pciemu_hw_sg {
	uint64_t addr;
	uint32_t len;
	uint32_t flags;
} __attribute__((packed));

#endif /* PCIEMU_HW_SG_H */
