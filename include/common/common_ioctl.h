/*
 * common_ioctl.h: Provides common IOCTL definitions for user-space and driver
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#ifndef COMMON_IOCTL_H
#define COMMON_IOCTL_H

#ifdef __KERNEL__
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#else
#include "base.h"
#include <sys/ioctl.h>
#endif

/* IOCTL Structures */
struct vai_alloc_buffer_arg {
	size_t size; /* Size of the buffer in bytes */
	uint64_t handle; /* Handle to the allocated buffer (output) */
};

struct vai_load_model_arg {
	uint64_t buffer_handle; /* Handle to buffer containing model data */
	size_t data_size; /* Size of the model data in bytes */
	uint32_t model_id; /* Output: Assigned model ID */
};

struct vai_run_inference_arg {
	uint32_t model_id; /* ID of the loaded model */
	uint64_t input_handle; /* Handle to input data buffer */
	uint64_t output_handle; /* Handle to output data buffer */
	uint32_t batch_size; /* Number of inputs to process */
};

/* IOCTL Magic Number */
#define PCIE_ACCEL_EMU_IOCTL_MAGIC 0xE1

/* Buffer */
#define PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER \
	_IOWR(PCIE_ACCEL_EMU_IOCTL_MAGIC, 3, struct vai_alloc_buffer_arg)
#define PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 4, uint64_t)

/* Model */
#define PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL \
	_IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 5, struct vai_load_model_arg)
#define PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 6, uint32_t)
#define PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE \
	_IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 7, struct vai_run_inference_arg)

#endif /* COMMON_IOCTL_H */
