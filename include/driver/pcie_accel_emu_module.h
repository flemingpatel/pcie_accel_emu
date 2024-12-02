/*
 * pcie_accel_emu.h: Provides driver module definitions
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#ifndef PCIE_ACCEL_EMU_MODULE_H
#define PCIE_ACCEL_EMU_MODULE_H

#include "hw/pciemu_hw.h"
#include "pcie_accel_emu_ioctl.h"

#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/completion.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/genalloc.h>

/* forward declaration */
struct pcie_accel_emu_dev;

/* BAR Structure */
struct pcie_accel_emu_bar {
	uint64_t start;
	uint64_t end;
	uint64_t len;
	void __iomem *mmio;
};

/* IRQ Structure */
struct pcie_accel_emu_irq {
	int irq_nums[PCIEMU_HW_IRQ_CNT]; /* Array to store all IRQ numbers */
	void __iomem *mmio_ack_irq; /* Single MMIO ACK address */
};

/*
 * Model Structure
 * without tracking models, the driver cannot validate whether a model has been loaded before running inference
 * Malicious or buggy applications could attempt to run inference on invalid buffers or data
 * Usually device doesn't know the concept of model id so keeping only in driver
 */
struct pcie_accel_emu_model {
	uint32_t model_id;
	uint64_t buffer_handle;
	size_t data_size;
	struct list_head list;
};

struct pcie_accel_emu_buffer {
	uint64_t handle; /* Unique handle for the buffer */
	size_t size; /* Size of the buffer in bytes */
	void *cpu_addr; /* CPU-accessible address */
	dma_addr_t dma_handle; /* DMA address */
	struct list_head list; /* Linked list node */
};

struct pcie_accel_emu_dev {
	struct pci_dev *pdev;
	/* In this simple PCIe device, we only a single BAR (0)
	 * We could have an array of size PCI_STD_NUM_BARS to
	 * hold information about all bars.
	 */
	struct pcie_accel_emu_bar bar;
	struct pcie_accel_emu_irq irq;
	dev_t minor;
	dev_t major;
	struct cdev cdev;
	struct mutex ioctl_lock; /* Lock for ioctl ops */

	/* Model related fields */
	struct completion model_ctrl_done; /* Completion for all model operations */

	/* Model List */
	atomic_t model_id_counter; /* Counter for assigning unique model IDs */
	struct list_head model_list;
	struct mutex model_list_lock;

	/* Buffer List */
	atomic64_t buffer_id_counter; /* Counter for buffer handles */
	struct list_head buffer_list;
	struct mutex buffer_list_lock;

	/* Region List */
	struct list_head region_list;
	struct mutex region_list_lock;

	/*
	 * DMA Memory Pool
	 * we will use it to allocate buffer and get dma handle,
	 * however, we will not use as device internal buffer offset for simplicity(TODO)
	 */
	size_t dma_area_size; /* Size of DMA memory */
	void *dma_area_cpu_addr; /* CPU virtual address of DMA memory */
	dma_addr_t dma_area_phys_addr; /* Physical address of DMA memory */
	struct gen_pool *dma_pool; /* genalloc pool for DMA memory */
};

// move it to irq.h (TODO)
int pcie_accel_emu_irq_enable(struct pcie_accel_emu_dev *pemu_dev);
void pcie_accel_emu_irq_disable(struct pcie_accel_emu_dev *pemu_dev);

// create a buffer header and source files that has all buffer components including genalloc (TODO)
struct pcie_accel_emu_buffer *find_buffer_by_handle(struct pcie_accel_emu_dev *dev,
						    uint64_t handle);

#endif /* PCIE_ACCEL_EMU_MODULE_H */
