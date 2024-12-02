/*
 * pcie_accel_emu_ioctl.c: Driver IOCTL interface related implementation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "driver/pcie_accel_emu_module.h"
#include "driver/pcie_accel_emu_ioctl.h"

/* Buffer Allocation Helper Function */
static int allocate_buffer(struct pcie_accel_emu_dev *dev, size_t size,
			   struct pcie_accel_emu_buffer **out_buffer)
{
	return 0;
}

/* Buffer free Helper Function */
static int free_buffer(struct pcie_accel_emu_dev *dev, struct pcie_accel_emu_buffer *buffer)
{
	return 0;
}

/* Allocate Buffer */
long pcie_accel_emu_ioctl_alloc_buffer(struct pcie_accel_emu_dev *dev,
				       struct vai_alloc_buffer_arg __user *arg)
{
	struct pcie_accel_emu_buffer *buffer;
	struct vai_alloc_buffer_arg karg;
	int ret;

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

	dev_dbg(&dev->pdev->dev,
		"%s: buffer allocated (handle=%llu, size=%zu, cpu addr=%p, dma Addr=%pad", __func__,
		buffer->handle, buffer->size, buffer->cpu_addr, &buffer->dma_handle);

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
		break;

	case PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER:
		break;

	case PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL:
		/* signal the device to process the loaded model */
		iowrite32(PCIEMU_HW_MODEL_CMD_LOAD_MODEL,
			  pemu_dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_CONTROL);
		/* wait for device to acknowledge model load via IRQ */
		wait_for_completion(&pemu_dev->model_ctrl_done);

	case PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL:
		break;

	case PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE:
		break;

	default:
		ret = -ENOTTY;
	}

	/* Unlock the ioctl mutex */
	mutex_unlock(&pemu_dev->ioctl_lock);

	return ret;
}
