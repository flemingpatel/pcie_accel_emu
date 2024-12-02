/*
 * pcie_accel_emu_ioctl.c: Driver IOCTL interface related implementation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "driver/pcie_accel_emu_module.h"

/* Allocate Buffer */
long pcie_accel_emu_ioctl_alloc_buffer(struct pcie_accel_emu_dev *dev,
				       struct vai_alloc_buffer_arg __user *arg)
{
	struct pcie_accel_emu_buffer *buffer;
	struct vai_alloc_buffer_arg karg;
	int ret = 0;

	/* copy allocation parameters from user-space */
	if (copy_from_user(&karg, arg, sizeof(karg)))
		return -EFAULT;

	if (karg.size == 0 || karg.size > PCIEMU_HW_DMA_AREA_SIZE)
		return -EINVAL;

	ret = allocate_buffer(dev, karg.size, &buffer);
	if (ret)
		return ret;

	/* prepare output */
	karg.handle = buffer->handle;
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		/* cleanup on failure */
		free_buffer(dev, buffer);
		return -EFAULT;
	}

	return ret;
}

/* Free Buffer */
long pcie_accel_emu_ioctl_free_buffer(struct pcie_accel_emu_dev *dev, uint64_t __user *arg)
{
	uint64_t handle;
	int ret = 0;

	/* copy buffer handle from user-space */
	if (copy_from_user(&handle, arg, sizeof(handle)))
		return -EFAULT;

	/* find and free the buffer */
	ret = free_buffer_by_handle(dev, handle);
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

	/* allocate and initialize model structure */
	model = kzalloc(sizeof(*model), GFP_KERNEL);
	if (!model)
		return -ENOMEM;

	model->model_id = atomic_inc_return(&dev->model_id_counter);
	model->buffer_handle = karg.buffer_handle;
	model->data_size = karg.data_size;

	/* Add model to the list */
	mutex_lock(&dev->model_list_lock);
	list_add_tail(&model->list, &dev->model_list);
	mutex_unlock(&dev->model_list_lock);

	/* write model buffer dma addr and size to device registers */
	iowrite32((uint64_t)buffer->dma_handle, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_LOAD_ADDR);
	iowrite32(buffer->size, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_LOAD_SIZE);

	/* signal the device to load model */
	iowrite32(PCIEMU_HW_MODEL_CMD_LOAD_MODEL, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_CONTROL);

	/* wait for device to acknowledge model load via IRQ */
	wait_for_completion(&dev->model_ctrl_done);

	dev_dbg(&dev->pdev->dev, "%s: model loaded successfully (id=%u, buffer_handle=%llu, size=%zu)",
		__func__, model->model_id, model->buffer_handle, model->data_size);

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

	/* find the model to validate it exist */
	mutex_lock(&dev->model_list_lock);
	list_for_each_entry(model, &dev->model_list, list) {
		if (model->model_id == karg.model_id)
			break;
	}
	mutex_unlock(&dev->model_list_lock);

	if (&model->list == &dev->model_list)
		return -ENOENT;

	/* find the input and output buffers */
	input_buffer = find_buffer_by_handle(dev, karg.input_handle);
	output_buffer = find_buffer_by_handle(dev, karg.output_handle);
	if (!input_buffer || !output_buffer)
		return -EINVAL;

	/* write model input buffer, output buffer dma addr and size to device registers */
	iowrite32((uint64_t)input_buffer->dma_handle, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_INPUT_ADDR);
	iowrite32(input_buffer->size, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_INPUT_SIZE);
	iowrite32((uint64_t)output_buffer->dma_handle, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_OUTPUT_ADDR);
	iowrite32(output_buffer->size, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_OUTPUT_SIZE);

	/* write batch size to device registers */
	iowrite32(karg.batch_size, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_BATCH_SIZE);

	/* signal the device to load model */
	iowrite32(PCIEMU_HW_MODEL_CMD_RUN_INFERENCE, dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_CONTROL);

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
	case PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER:
		ret = pcie_accel_emu_ioctl_alloc_buffer(pemu_dev, (struct vai_alloc_buffer_arg __user *)arg);
		break;

	case PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER:
		ret = pcie_accel_emu_ioctl_free_buffer(pemu_dev, (uint64_t __user *)arg);
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
