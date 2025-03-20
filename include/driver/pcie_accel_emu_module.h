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
 * @data_size Size of the model data
 * @buffer_handle Handle to the buffer containing the model data
 * @device_mem_offset Offset (physical address) in device memory where the model is loaded
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
	size_t data_size;
	uint64_t buffer_handle;
	dma_addr_t device_mem_offset;
	struct list_head list;
};

/**
 * @brief Represents the PCIe accelerator emulation device
 * @pdev Pointer to the PCI device structure
 * @bars Array of bars (BAR0 and BAR1)
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
 * @device_mem_pool Dedicated device memory is managed by a gen_pool built over BAR1
 */
struct pcie_accel_emu_dev {
	struct pci_dev *pdev;
	struct pcie_accel_emu_bar bars[PCIEMU_HW_BAR_CNT];
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

	/* device memory pool for management */
	struct gen_pool *device_mem_pool;
};

/* These are *array indices* within pemu_dev->bars[] */
enum pcie_accel_emu_bar_index { BAR_IDX_0, BAR_IDX_1 };

/* bar_ids[0] == 0 => BAR0 */
/* bar_ids[1] == 1 => BAR1 */
static const unsigned int
	bar_ids[PCIEMU_HW_BAR_CNT] = { [BAR_IDX_0] = PCIEMU_HW_BAR0, [BAR_IDX_1] = PCIEMU_HW_BAR1 };

/**
 * @brief DMA operations (host to device): program registers and ring the doorbell
 *
 * @param pemu_dev Pointer to the device structure
 * @param host_buffer Host DMA buffer
 * @param dev_dst The absolute physical address of device memory region(BAR1) as destination
 * @param len size of the data to transfer from host DMA buffer
 * @return 0 on success or negative error code on failure
 */
int pcie_accel_emu_dma_from_host_to_device(struct pcie_accel_emu_dev *pemu_dev,
					   struct pcie_accel_emu_buffer *host_buffer, dma_addr_t dev_dst,
					   size_t len);

/**
 * @brief DMA operations (device to host): program registers and ring the doorbell
 *
 * @param pemu_dev Pointer to the device structure
 * @param dev_src The absolute physical address of device memory region(BAR1) as source
 * @param host_buffer Host DMA buffer
 * @param len size of the data to transfer from host DMA buffer
 * @return 0 on success or negative error code on failure
 */
int pcie_accel_emu_dma_from_device_to_host(struct pcie_accel_emu_dev *pemu_dev, dma_addr_t dev_src,
					   struct pcie_accel_emu_buffer *host_buffer, size_t len);

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
