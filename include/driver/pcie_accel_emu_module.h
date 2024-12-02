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
#include "pcie_accel_emu_buffer.h"

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

/**
 * @brief Represents a PCI BAR mapping
 * @start Start address of the BAR
 * @end End address of the BAR
 * @len Length of the BAR
 * @mmio Kernel virtual address for MMIO
 */
struct pcie_accel_emu_bar {
	uint64_t start;
	uint64_t end;
	uint64_t len;
	void __iomem *mmio;
};

/**
 * @brief Represents IRQ information
 * @irq_nums Array to store IRQ numbers
 * @mmio_ack_irq Single MMIO address to acknowledge IRQs
 */
struct pcie_accel_emu_irq {
	int irq_nums[PCIEMU_HW_IRQ_CNT];
	void __iomem *mmio_ack_irq;
};

/**
 * @brief Represents a loaded model
 * @model_id Unique identifier for the model
 * @buffer_handle Handle to the buffer containing the model data
 * @data_size Size of the model data
 * @list Linked list node
 *
 * without tracking models,
 * the driver cannot validate whether a model has been loaded before running inference
 * Malicious or buggy applications could attempt to run inference on invalid buffers or data
 * Usually device doesn't know the concept of model id so keeping only in driver
 *
 * TODO move this to model source files like buffer to simplify IOCTL
 */
struct pcie_accel_emu_model {
	uint32_t model_id;
	uint64_t buffer_handle;
	size_t data_size;
	struct list_head list;
};

/**
 * @brief Represents the PCIe accelerator emulation device
 * @pdev Pointer to the PCI device structure
 * @bar Structure containing BAR mapping information
 * @irq Structure containing IRQ information
 * @minor Minor device number
 * @major Major device number
 * @cdev Character device structure
 * @ioctl_lock Mutex to protect IOCTL operations
 * @model_ctrl_done Completion structure for model control operations
 * @model_id_counter Atomic counter for assigning unique model IDs
 * @model_list List head for tracking loaded models
 * @model_list_lock Mutex to protect access to the model list
 * @buffer_id_counter Atomic counter for assigning unique buffer handles
 * @buffer_list List head for tracking allocated buffers
 * @buffer_list_lock Mutex to protect access to the buffer list
 * @dma_area_size Size of the DMA memory area
 * @dma_area_cpu_addr CPU virtual address of the DMA memory
 * @dma_area_phys_addr Physical (DMA) address of the DMA memory
 * @dma_pool General-purpose memory pool for DMA allocations
 */
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
	struct mutex ioctl_lock;

	/* Model related fields */
	struct completion model_ctrl_done;

	/* Model List */
	atomic_t model_id_counter;
	struct list_head model_list;
	struct mutex model_list_lock;

	/* Buffer List */
	atomic64_t buffer_id_counter;
	struct list_head buffer_list;
	struct mutex buffer_list_lock;

	/*
	 * DMA Memory Pool
	 * we will use it to allocate buffer and get dma handle,
	 * however, we will not use as device internal buffer offset for simplicity(TODO)
	 */
	size_t dma_area_size;
	void *dma_area_cpu_addr;
	dma_addr_t dma_area_phys_addr;
	struct gen_pool *dma_pool;
};

/**
 * @brief Enable IRQs for the device
 *
 * @param pemu_dev Pointer to the device structure
 * @return 0 on success or negative error code on failure
 */
int pcie_accel_emu_irq_enable(struct pcie_accel_emu_dev *pemu_dev);

/**
 * @brief Disable IRQs for the device
 *
 * @param pemu_dev Pointer to the device structure
 */
void pcie_accel_emu_irq_disable(struct pcie_accel_emu_dev *pemu_dev);

#endif /* PCIE_ACCEL_EMU_MODULE_H */
