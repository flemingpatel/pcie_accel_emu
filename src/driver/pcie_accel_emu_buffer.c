/*
* pcie_accel_emu_buffer.c: Driver buffer related implementation
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2024 Fleming Patel
*/

#include "driver/pcie_accel_emu_module.h"

/* Find buffer by handle */
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

/* Allocate Buffer */
int allocate_buffer(struct pcie_accel_emu_dev *dev, size_t size, struct pcie_accel_emu_buffer **out_buffer)
{
	struct pcie_accel_emu_buffer *buffer;
	unsigned long vaddr;
	dma_addr_t dma_addr;

	/* allocate buffer structure */
	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	/* allocate memory from gen_pool */
	vaddr = gen_pool_alloc(dev->dma_pool, size);
	if (!vaddr) {
		kfree(buffer);
		return -ENOMEM;
	}

	/* calculate DMA address */
	dma_addr = dev->dma_area_phys_addr + (vaddr - (unsigned long)dev->dma_area_cpu_addr);

	/* initialize buffer structure */
	buffer->handle = atomic64_inc_return(&dev->buffer_id_counter);
	buffer->size = size;
	buffer->cpu_addr = (void *)vaddr;
	buffer->dma_handle = dma_addr;

	/* add buffer to the list */
	mutex_lock(&dev->buffer_list_lock);
	list_add_tail(&buffer->list, &dev->buffer_list);
	mutex_unlock(&dev->buffer_list_lock);

	*out_buffer = buffer;

	dev_dbg(&dev->pdev->dev, "%s: buffer allocated (handle=%llu, size=%zu, cpu addr=%p, dma Addr=%pad)",
		__func__, buffer->handle, buffer->size, buffer->cpu_addr, &buffer->dma_handle);

	return 0;
}

/* Free buffer by pointer */
int free_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	if (!buffer)
		return -EINVAL;

	/* free memory back to gen_pool */
	gen_pool_free(dev->dma_pool, (unsigned long)buffer->cpu_addr, buffer->size);

	/* remove from buffer_list */
	mutex_lock(&dev->buffer_list_lock);
	list_del(&buffer->list);
	mutex_unlock(&dev->buffer_list_lock);

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

	/* free buffer structure */
	kfree(buffer);

	return 0;
}

/* Free buffer by handle */
int free_buffer_by_handle(struct pcie_accel_emu_dev *dev, uint64_t handle)
{
	struct pcie_accel_emu_buffer *buffer;

	/* find buffer */
	buffer = find_buffer_by_handle(dev, handle);
	if (!buffer)
		return -ENOENT;

	/* free buffer */
	return free_buffer(dev, buffer);
}

/* Free all buffers */
void free_all_buffers(struct pcie_accel_emu_dev *dev)
{
	struct pcie_accel_emu_buffer *buffer, *next_buffer;
	mutex_lock(&dev->buffer_list_lock);
	list_for_each_entry_safe(buffer, next_buffer, &dev->buffer_list, list) {
		/* free memory back to gen_pool */
		gen_pool_free(dev->dma_pool, (unsigned long)buffer->cpu_addr, buffer->size);

		/* remove from buffer_list */
		list_del(&buffer->list);

		dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);

		/* free buffer structure */
		kfree(buffer);
	}
	mutex_unlock(&dev->buffer_list_lock);
}
