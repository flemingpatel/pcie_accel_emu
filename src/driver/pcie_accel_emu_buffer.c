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

int register_buffer(struct pcie_accel_emu_dev *dev, size_t size, uint64_t cpu_addr,
		    struct pcie_accel_emu_buffer **out_buffer)
{
	struct pcie_accel_emu_buffer *buffer;

	/* allocate buffer structure */
	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	/* initialize buffer structure */
	buffer->size = size;
	buffer->cpu_addr = cpu_addr;

	int ret;
	int npages;

	/*
	 * compute how many pages are needed
	 *  - PAGE_SIZE is typically 4096 bytes.
	 *  - (size + PAGE_SIZE - 1) / PAGE_SIZE ensures we round up if 'size' is not a multiple of PAGE_SIZE
	 *  example: if size=4100 and PAGE_SIZE=4096, npages=2.
	 */
	npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	buffer->num_pages = npages;
	buffer->pages = kzalloc(npages * sizeof(struct page *), GFP_KERNEL);
	if (!buffer->pages) {
		ret = -ENOMEM;
		goto free_buffer;
	}

	buffer->sgl = kzalloc(npages * sizeof(struct scatterlist), GFP_KERNEL);
	if (!buffer->sgl) {
		ret = -ENOMEM;
		goto free_pages;
	}

	/* pin the user pages
	 * FOLL_WRITE indicates we intend to write to these pages (DMA might write, etc.)
	 */
	mmap_read_lock(current->mm);
	ret = pin_user_pages_fast(cpu_addr, npages, FOLL_WRITE, buffer->pages);
	mmap_read_unlock(current->mm);
	if (ret < npages) {
		ret = -EFAULT;
		goto free_sgl;
	}

	/* initialize scatterlist table */
	sg_init_table(buffer->sgl, npages);
	for (int i = 0; i < npages; i++) {
		size_t len = PAGE_SIZE;
		/*
		 * associates each scatterlist entry with one pinned page
		 * if it’s the last page and size isn't a multiple of PAGE_SIZE,
		 * then the length should be size % PAGE_SIZE
		 */
		if ((i == npages - 1) && (size % PAGE_SIZE != 0)) {
			len = size % PAGE_SIZE;
		}
		sg_set_page(&buffer->sgl[i], buffer->pages[i], len, 0);
	}

	/* map the scatterlist for DMA */
	buffer->sg_count = dma_map_sg(&dev->pdev->dev, buffer->sgl, npages, DMA_BIDIRECTIONAL);
	if (buffer->sg_count == 0) {
		ret = -ENOMEM;
		goto unpin_pages;
	}

	/* add buffer to the list */
	buffer->handle = atomic64_inc_return(&dev->buffer_id_counter);
	mutex_lock(&dev->buffer_list_lock);
	list_add_tail(&buffer->list, &dev->buffer_list);
	mutex_unlock(&dev->buffer_list_lock);

	/* return this buffer structure to the caller */
	*out_buffer = buffer;

	dev_dbg(&dev->pdev->dev, "%s: buffer allocated (handle=%llu, size=%zu, cpu_addr=%llu)", __func__,
		buffer->handle, buffer->size, buffer->cpu_addr);

	return 0;

unpin_pages:
	unpin_user_pages(buffer->pages, npages);
free_sgl:
	kfree(buffer->sgl);
free_pages:
	kfree(buffer->pages);
free_buffer:
	kfree(buffer);
	return ret;
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

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);
	kfree(buffer);

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

	dev_dbg(&dev->pdev->dev, "%s: buffer freed (handle=%llu)\n", __func__, buffer->handle);
	kfree(buffer);

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
