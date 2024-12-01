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


/* IOCTL Buffer Handlers */
long pcie_accel_emu_ioctl_alloc_buffer(struct file *fp, struct vai_alloc_buffer_arg __user *arg);
long pcie_accel_emu_ioctl_free_buffer(struct file *fp, uint64_t __user *arg);

/* IOCTL Model Handlers */
long pcie_accel_emu_ioctl_load_model(struct file *fp, struct vai_load_model_arg __user *arg);
long pcie_accel_emu_ioctl_unload_model(struct file *fp, uint32_t __user *arg);
long pcie_accel_emu_ioctl_run_inference(struct file *fp, struct vai_run_inference_arg __user *arg);

/* Main IOCTL Handler */
long pcie_accel_emu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);

#endif /* PCIE_ACCEL_EMU_IOCTL_H */
