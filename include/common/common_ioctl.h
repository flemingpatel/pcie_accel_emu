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

/**
 * @brief Arguments for buffer allocation IOCTL
 *
 * @size Size of the buffer to allocate (in bytes)
 * @handle Unique handle to the allocated buffer
 */
struct vai_alloc_buffer_arg {
	size_t size;
	uint64_t handle;
};

/**
 * @brief Arguments for model loading IOCTL
 *
 * @buffer_handle Handle to the buffer containing the model data
 * @data_size Size of the model buffer in bytes
 * @model_id Output assigned model ID
 */
struct vai_load_model_arg {
	uint64_t buffer_handle;
	size_t data_size;
	uint32_t model_id;
};

/**
 * @brief Arguments for running inference IOCTL
 *
 * @model_id ID of the loaded model to use for inference
 * @input_handle Handle to the input data buffer
 * @input_size Size of the input buffer in bytes
 * @output_handle Handle to the output data buffer
 * @output_size Size of the output buffer in bytes
 * @batch_size Number of inputs to process
 */
struct vai_run_inference_arg {
	uint32_t model_id;
	uint64_t input_handle;
	ssize_t input_size;
	uint64_t output_handle;
	ssize_t output_size;
	uint32_t batch_size;
};

/* IOCTL Magic Number */
#define PCIE_ACCEL_EMU_IOCTL_MAGIC 0xE1

/* Buffer Management IOCTL */
#define PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER _IOWR(PCIE_ACCEL_EMU_IOCTL_MAGIC, 3, struct vai_alloc_buffer_arg)
#define PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 4, uint64_t)

/* Model Management IOCTL */
#define PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 5, struct vai_load_model_arg)
#define PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 6, uint32_t)
#define PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 7, struct vai_run_inference_arg)

#endif /* COMMON_IOCTL_H */
