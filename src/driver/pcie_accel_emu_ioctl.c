/*
 * pcie_accel_emu_ioctl.c: Driver IOCTL interface related implementation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "driver/pcie_accel_emu_module.h"

/* Allocate Buffer */
long pcie_accel_emu_ioctl_register_buffer(struct pcie_accel_emu_dev *dev,
					  struct vai_register_buffer_arg __user *arg)
{
	struct pcie_accel_emu_buffer *buffer;
	struct vai_register_buffer_arg karg;
	int ret = 0;

	/* copy allocation parameters from user-space */
	if (copy_from_user(&karg, arg, sizeof(karg)))
		return -EFAULT;

	if (karg.size == 0 || karg.v_addr == 0 || karg.size > PCIEMU_HW_DMA_AREA_SIZE)
		return -EINVAL;

	ret = register_buffer(dev, karg.size, karg.v_addr, &buffer);
	if (ret)
		return ret;

	/* prepare output */
	karg.handle = buffer->handle;
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		/* cleanup on failure */
		deregister_buffer(dev, buffer);
		return -EFAULT;
	}

	return ret;
}

/* Free Buffer */
long pcie_accel_emu_ioctl_deregister_buffer(struct pcie_accel_emu_dev *dev, uint64_t __user *arg)
{
	uint64_t handle;
	int ret = 0;

	/* copy buffer handle from user-space */
	if (copy_from_user(&handle, arg, sizeof(handle)))
		return -EFAULT;

	/* find and free the buffer */
	ret = deregister_buffer_by_handle(dev, handle);
	if (ret)
		return ret;

	return ret;
}

/* Load model into device */
long pcie_accel_emu_ioctl_load_model(struct pcie_accel_emu_dev *dev, struct vai_load_model_arg __user *arg)
{
	struct vai_load_model_arg karg;
	struct pcie_accel_emu_model *model;
	struct pcie_accel_emu_buffer *buffer;

	/* copy data from user-space */
	if (copy_from_user(&karg, arg, sizeof(karg)))
		return -EFAULT;

	/* validate buffer_handle and data_size */
	if (karg.buffer_handle == 0 || karg.data_size == 0)
		return -EINVAL;

	/* find the buffer using buffer_handle */
	buffer = find_buffer_by_handle(dev, karg.buffer_handle);
	if (!buffer)
		return -EINVAL;

	/* verify that the user-requested data_size fits in the host buffer */
	if (karg.data_size > buffer->size) {
		dev_err(&dev->pdev->dev, "%s: model data_size (%zu) exceeds the host buffer size (%zu)",
			__func__, karg.data_size, buffer->size);
		return -EINVAL;
	}

	/* Allocate region in dedicated device memory (BAR1) using gen_pool.
	 * Here, the allocation is relative to the BAR1 mapping.
	 */
	dma_addr_t dev_mem_offset;
	unsigned long dev_mem_vaddr;

	dev_mem_vaddr = gen_pool_alloc(dev->device_mem_pool, karg.data_size);
	if (!dev_mem_vaddr) {
		dev_err(&dev->pdev->dev, "%s: not enough space in device memory for model of size %zu",
			__func__, karg.data_size);
		return -ENOMEM;
	}

	/*
	 * The absolute physical destination address is computed as:
	 * 	dev_mem_offset = bar1.start + (dev_mem_vaddr - (unsigned long)bar1.mmio)
	 */
	dev_mem_offset =
		dev->bars[BAR_IDX_1].start + (dev_mem_vaddr - (unsigned long)dev->bars[BAR_IDX_1].mmio);

	/* allocate and initialize model structure */
	model = kzalloc(sizeof(*model), GFP_KERNEL);
	if (!model) {
		/* free mem in pool */
		gen_pool_free(dev->device_mem_pool, dev_mem_vaddr, karg.data_size);
		return -ENOMEM;
	}
	model->model_id = atomic_inc_return(&dev->model_id_counter);
	model->data_size = karg.data_size;
	model->buffer_handle = karg.buffer_handle;
	model->device_mem_offset = dev_mem_offset;

	/* Add model to the list */
	mutex_lock(&dev->model_list_lock);
	list_add_tail(&model->list, &dev->model_list);
	mutex_unlock(&dev->model_list_lock);

	// TODO implement

	/* wait for device to acknowledge model load via IRQ */
	wait_for_completion(&dev->model_ctrl_done);

	dev_dbg(&dev->pdev->dev,
		"%s: model loaded successfully (id=%u, buffer_handle=%llu, device_mem_offset=%pad, size=%zu)",
		__func__, model->model_id, model->buffer_handle, &model->device_mem_offset, model->data_size);

	/* return model_id to user-space */
	karg.model_id = model->model_id;
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		return -EFAULT;
	}

	return 0;
}

/* run model inference */
long pcie_accel_emu_ioctl_run_inference(struct pcie_accel_emu_dev *dev,
					struct vai_run_inference_arg __user *arg)
{
	struct vai_run_inference_arg karg;
	struct pcie_accel_emu_model *model;
	struct pcie_accel_emu_buffer *input_buffer, *output_buffer;

	/* copy data from user-space */
	if (copy_from_user(&karg, arg, sizeof(karg)))
		return -EFAULT;

	/* validate model_id and buffer handles */
	if (karg.model_id == 0 || karg.input_handle == 0 || karg.output_handle == 0)
		return -EINVAL;

	// TODO implement

	/* wait for device to acknowledge model run inference via IRQ */
	wait_for_completion(&dev->model_ctrl_done);

	dev_dbg(&dev->pdev->dev, "%s: inference completed successfully (model_id=%u, batch_size=%d)",
		__func__, karg.model_id, karg.batch_size);

	return 0;
}

/* Main IOCTL Handler */
long pcie_accel_emu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct pcie_accel_emu_dev *pemu_dev = fp->private_data;
	long ret = 0;

	/* lock the ioctl mutex since we will be dealing with shared buffer */
	mutex_lock(&pemu_dev->ioctl_lock);

	switch (cmd) {
	case PCIE_ACCEL_EMU_IOCTL_REGISTER_BUFFER:
		ret = pcie_accel_emu_ioctl_register_buffer(pemu_dev,
							   (struct vai_register_buffer_arg __user *)arg);
		break;

	case PCIE_ACCEL_EMU_IOCTL_DEREGISTER_BUFFER:
		ret = pcie_accel_emu_ioctl_deregister_buffer(pemu_dev, (uint64_t __user *)arg);
		break;

	case PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL:
		ret = pcie_accel_emu_ioctl_load_model(pemu_dev, (struct vai_load_model_arg __user *)arg);
		break;

	case PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL:
		break;

	case PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE:
		ret = pcie_accel_emu_ioctl_run_inference(pemu_dev,
							 (struct vai_run_inference_arg __user *)arg);
		break;

	default:
		ret = -ENOTTY;
	}

	/* Unlock the ioctl mutex */
	mutex_unlock(&pemu_dev->ioctl_lock);

	return ret;
}
