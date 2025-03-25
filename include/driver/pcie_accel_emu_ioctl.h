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
struct pcie_accel_emu_dev;

/**
 * @brief Handle buffer registration in the driver
 *
 * @param dev Pointer to the device structure
 * @param arg User-space pointer to the allocation argument structure
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl_register_buffer(struct pcie_accel_emu_dev *dev,
					  struct vai_register_buffer_arg __user *arg);

/**
 * @brief Handle buffer deregistration in the driver
 *
 * @param dev Pointer to the device structure
 * @param arg User-space pointer to the buffer handle to free
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl_deregister_buffer(struct pcie_accel_emu_dev *dev, uint64_t __user *arg);

/**
 * @brief Handle model loading IOCTL
 *
 * @param dev Pointer to the device structure
 * @param arg User-space pointer to the model loading argument structure
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl_load_model(struct pcie_accel_emu_dev *dev, struct vai_load_model_arg __user *arg);

/**
 * @brief Handle model unloading IOCTL
 *
 * @param dev Pointer to the device structure
 * @param arg User-space pointer to the model ID to unload
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl_unload_model(struct pcie_accel_emu_dev *dev, uint32_t __user *arg);

/**
 * @brief Handle inference execution IOCTL
 *
 * @param dev Pointer to the device structure
 * @param arg User-space pointer to the inference argument structure
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl_run_inference(struct pcie_accel_emu_dev *dev,
					struct vai_run_inference_arg __user *arg);

/**
 * @brief Main IOCTL handler; Dispatches IOCTL commands to the appropriate handler functions
 *
 * @param fp File pointer associated with the device
 * @param cmd IOCTL command code
 * @param arg Argument passed from user space
 * @return 0 on success or negative error code on failure
 */
long pcie_accel_emu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);

#endif /* PCIE_ACCEL_EMU_IOCTL_H */
