/*
* pcie_accel_emu_buffer.c: Driver buffer related implementation
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2024 Fleming Patel
*/

#include "driver/pcie_accel_emu_module.h"

struct pcie_accel_emu_buffer *find_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle)
{
	struct pcie_accel_emu_buffer *buffer;

	mutex_lock(&dev->buffer_list_lock);
	list_for_each_entry(buffer, &dev->buffer_list, list) {
		if (buffer->handle == handle) {
			mutex_unlock(&dev->buffer_list_lock);
			return buffer;
		}
	}
	mutex_unlock(&dev->buffer_list_lock);
	return NULL;
}

int allocate_buffer(struct pcie_accel_emu_dev *dev, size_t size, struct pcie_accel_emu_buffer **out_buffer)
{
	struct pcie_accel_emu_buffer *buffer;

	/* allocate buffer structure */
	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	/* initialize buffer structure */
	buffer->handle = atomic64_inc_return(&dev->buffer_id_counter);
	buffer->size = size;
	buffer->cpu_addr = dma_alloc_coherent(&dev->pdev->dev, size, &buffer->dma_handle, GFP_KERNEL);
	if (!buffer->cpu_addr) {
		kfree(buffer);
		return -ENOMEM;
	}

	/* add buffer to the list */
	mutex_lock(&dev->buffer_list_lock);
	list_add_tail(&buffer->list, &dev->buffer_list);
	mutex_unlock(&dev->buffer_list_lock);

	*out_buffer = buffer;

	dev_dbg(&dev->pdev->dev, "%s: buffer allocated (handle=%llu, size=%zu, cpu_addr=%p, dma_handle=%pad)",
		__func__, buffer->handle, buffer->size, buffer->cpu_addr, &buffer->dma_handle);

	return 0;
}

int free_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	if (!buffer)
		return -EINVAL;

	/* free dma coherent memory */
	dma_free_coherent(&dev->pdev->dev, buffer->size, buffer->cpu_addr, buffer->dma_handle);

	/* remove from buffer_list */
	mutex_lock(&dev->buffer_list_lock);
	list_del(&buffer->list);
	mutex_unlock(&dev->buffer_list_lock);

	/* free buffer structure */
	kfree(buffer);

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

	return 0;
}

int free_buffer_locked(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	if (!buffer)
		return -EINVAL;

	/* free dma coherent memory */
	dma_free_coherent(&dev->pdev->dev, buffer->size, buffer->cpu_addr, buffer->dma_handle);

	/* expects caller to lock the list */
	list_del(&buffer->list);

	/* free buffer structure */
	kfree(buffer);

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

	return 0;
}

int free_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle)
{
	struct pcie_accel_emu_buffer *buffer;

	/* find buffer */
	buffer = find_buffer_by_handle(dev, handle);
	if (!buffer)
		return -ENOENT;

	/* call free buffer */
	return free_buffer(dev, buffer);
}

void free_all_buffers(struct pcie_accel_emu_dev *dev)
{
	struct pcie_accel_emu_buffer *buffer, *next_buffer;
	mutex_lock(&dev->buffer_list_lock);
	list_for_each_entry_safe(buffer, next_buffer, &dev->buffer_list, list) {
		/* call free_buffer_locked */
		free_buffer_locked(dev, buffer);
	}
	mutex_unlock(&dev->buffer_list_lock);
}
