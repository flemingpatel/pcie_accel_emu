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

int register_buffer(struct pcie_accel_emu_dev *dev, size_t size, struct pcie_accel_emu_buffer **out_buffer)
{
	struct pcie_accel_emu_buffer *buffer;

	/* allocate buffer structure */
	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	/* initialize buffer structure */
	buffer->handle = atomic64_inc_return(&dev->buffer_id_counter);
	buffer->size = size;

	// TODO implement

	/* add buffer to the list */
	mutex_lock(&dev->buffer_list_lock);
	list_add_tail(&buffer->list, &dev->buffer_list);
	mutex_unlock(&dev->buffer_list_lock);

	*out_buffer = buffer;

	dev_dbg(&dev->pdev->dev, "%s: buffer allocated (handle=%llu, size=%zu, cpu_addr=%p)", __func__,
		buffer->handle, buffer->size, buffer->cpu_addr);

	return 0;
}

int deregister_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	if (!buffer)
		return -EINVAL;

	/* unmap the scatter-list */
	dma_unmap_sg(&dev->pdev->dev, buffer->sgl, buffer->num_pages, DMA_BIDIRECTIONAL);
	/* unpin all pages */
	unpin_user_pages(buffer->pages, buffer->num_pages);
	/* remove from buffer_list */
	mutex_lock(&dev->buffer_list_lock);
	list_del(&buffer->list);
	mutex_unlock(&dev->buffer_list_lock);
	/* free buffer structure */
	kfree(buffer->sgl);
	kfree(buffer->pages);
	kfree(buffer);

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

	return 0;
}

int deregister_buffer_locked(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	if (!buffer)
		return -EINVAL;

	/* unmap the scatter-list */
	dma_unmap_sg(&dev->pdev->dev, buffer->sgl, buffer->num_pages, DMA_BIDIRECTIONAL);
	/* unpin all pages */
	unpin_user_pages(buffer->pages, buffer->num_pages);
	/* remove from buffer_list (expects caller to lock the list) */
	list_del(&buffer->list);
	/* free buffer structure */
	kfree(buffer->sgl);
	kfree(buffer->pages);
	kfree(buffer);

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

	return 0;
}

int deregister_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle)
{
	struct pcie_accel_emu_buffer *buffer;

	/* find buffer */
	buffer = find_buffer_by_handle(dev, handle);
	if (!buffer)
		return -ENOENT;

	/* call free buffer */
	return deregister_buffer(dev, buffer);
}

void deregister_all_buffers(struct pcie_accel_emu_dev *dev)
{
	struct pcie_accel_emu_buffer *buffer, *next_buffer;
	mutex_lock(&dev->buffer_list_lock);
	list_for_each_entry_safe(buffer, next_buffer, &dev->buffer_list, list) {
		/* call free_buffer_locked */
		deregister_buffer_locked(dev, buffer);
	}
	mutex_unlock(&dev->buffer_list_lock);
}
