/*
* pcie_accel_emu_buffer.h: Provides driver buffer related definitions
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2024 Fleming Patel
*/

#ifndef PCIE_ACCEL_EMU_BUFFER_H
#define PCIE_ACCEL_EMU_BUFFER_H

/* forward declaration */
struct pcie_accel_emu_dev;

/**
 * @brief Represents a buffer in the driver
 *
 * @handle Unique handle for the buffer
 * @size Size of the buffer in bytes
 * @cpu_addr CPU-accessible address (vaddr)
 * @num_pages Number of pages pinned
 * @pages Array of pinned pages
 * @sgl Scatter-list built from the pinned pages
 * @sg_count Number of scatter-list entries
 * @list Linked list node
 */
// TODO we can have dma-coherent buffer just like before
struct pcie_accel_emu_buffer {
	uint64_t handle;
	size_t size;
	uint64_t *cpu_addr;
	int num_pages;
	struct page **pages;
	struct scatterlist *sgl;
	int sg_count;
	struct list_head list;
};

/**
 * @brief Find buffer by given handle
 *
 * @param dev Pointer to the device structure
 * @param handle The handle of the buffer to find
 * @return pointer to pcie_accel_emu_buffer on success or NULL failure
 */
struct pcie_accel_emu_buffer *find_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle);

/**
 * @brief Registers user space buffer (pin_user_pages_fast)
 *
 * @param dev Pointer to the device structure
 * @param size Size of the buffer to allocate
 * @param out_buffer Pointer to store the allocated buffer pointer
 * @return 0 on success or negative error code on failure
 */
int register_buffer(struct pcie_accel_emu_dev *dev, size_t size, struct pcie_accel_emu_buffer **out_buffer);

/**
 * @brief Deregisters user space buffer with internal list lock
 *
 * @param dev Pointer to the device structure
 * @param buffer The buffer to free
 * @return 0 on success or negative error code on failure
 */
int deregister_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer);

/**
 * @brief Deregisters user space buffer without internal list lock
 *
 * @param dev Pointer to the device structure
 * @param buffer The buffer to free
 * @return 0 on success or negative error code on failure
 */
int deregister_buffer_locked(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer);

/**
 * @brief Deregisters user space buffer by given its handle
 *
 * @param dev Pointer to the device structure
 * @param handle The handle of the buffer to free
 * @return 0 on success or negative error code on failure
 */
int deregister_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle);

/**
 * @brief Deregisters all buffers
 *
 * @param dev The device structure
 */
void deregister_all_buffers(struct pcie_accel_emu_dev *dev);

#endif /* PCIE_ACCEL_EMU_BUFFER_H */
