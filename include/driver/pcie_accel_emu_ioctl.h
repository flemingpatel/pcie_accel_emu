/*
 * pcie_accel_emu_ioctl.h: Provides driver IOCTL interface related definitions
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#ifndef PCIE_ACCEL_EMU_IOCTL_H
#define PCIE_ACCEL_EMU_IOCTL_H

#include "common/common_ioctl.h"
#include <linux/fs.h>

/* forward declaration */
typedef struct pcie_accel_emu_dev pcie_accel_emu_dev;

/* IOCTL Buffer Handlers */
long pcie_accel_emu_ioctl_alloc_buffer(pcie_accel_emu_dev *dev,
				       struct vai_alloc_buffer_arg __user *arg);
long pcie_accel_emu_ioctl_free_buffer(pcie_accel_emu_dev *dev, uint64_t __user *arg);

/* IOCTL Model Handlers */
long pcie_accel_emu_ioctl_load_model(pcie_accel_emu_dev *dev,
				     struct vai_load_model_arg __user *arg);
long pcie_accel_emu_ioctl_unload_model(pcie_accel_emu_dev *dev, uint32_t __user *arg);
long pcie_accel_emu_ioctl_run_inference(pcie_accel_emu_dev *dev,
					struct vai_run_inference_arg __user *arg);

/* Main IOCTL Handler */
long pcie_accel_emu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);

#endif /* PCIE_ACCEL_EMU_IOCTL_H */
