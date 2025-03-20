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
 * @cpu_addr CPU-accessible address
 * @dma_handle DMA address
 * @list Linked list node
 */
struct pcie_accel_emu_buffer {
	uint64_t handle;
	size_t size;
	void *cpu_addr;
	dma_addr_t dma_handle;
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
 * @brief Allocates a buffer
 *
 * @param dev Pointer to the device structure
 * @param size Size of the buffer to allocate
 * @param out_buffer Pointer to store the allocated buffer pointer
 * @return 0 on success or negative error code on failure
 */
int allocate_buffer(struct pcie_accel_emu_dev *dev, size_t size, struct pcie_accel_emu_buffer **out_buffer);

/**
 * @brief Frees a buffer given its pointer with internal list lock
 *
 * @param dev Pointer to the device structure
 * @param buffer The buffer to free
 * @return 0 on success or negative error code on failure
 */
int free_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer);

/**
 * @brief Frees a buffer given its pointer without internal list lock
 *
 * @param dev Pointer to the device structure
 * @param buffer The buffer to free
 * @return 0 on success or negative error code on failure
 */
int free_buffer_locked(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer);

/**
 * @brief Frees a buffer given its handle
 *
 * @param dev Pointer to the device structure
 * @param handle The handle of the buffer to free
 * @return 0 on success or negative error code on failure
 */
int free_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle);

/**
 * @brief Frees all buffers
 *
 * @param dev The device structure
 */
void free_all_buffers(struct pcie_accel_emu_dev *dev);

#endif /* PCIE_ACCEL_EMU_BUFFER_H */
