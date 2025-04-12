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
/*
 * when building inside the Linux kernel driver,
 * we rely on kernel’s includes
 */
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#else
/*
 * when building in QEMU or any other non-kernel context,
 * we use the standard C integer headers.
 */
#include "base.h"
#include <sys/ioctl.h>
#endif

/**
 * @brief Argument struct for buffer registration from libvai
 *
 * @v_addr User-space virtual address of the buffer
 * @size Size of the buffer to allocate (in bytes)
 * @handle Unique handle to the allocated buffer
 */
struct vai_register_buffer_arg {
	uint64_t v_addr;
	size_t size;
	uint64_t handle;
};

/**
 * @brief Argument struct for model loading from libvai
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
 * @brief Argument struct for running inference from libvai
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
#define PCIE_ACCEL_EMU_IOCTL_REGISTER_BUFFER \
	_IOWR(PCIE_ACCEL_EMU_IOCTL_MAGIC, 3, struct vai_register_buffer_arg)
#define PCIE_ACCEL_EMU_IOCTL_DEREGISTER_BUFFER _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 4, uint64_t)

/* Model Management IOCTL */
#define PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 5, struct vai_load_model_arg)
#define PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 6, uint32_t)
#define PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE _IOW(PCIE_ACCEL_EMU_IOCTL_MAGIC, 7, struct vai_run_inference_arg)

#endif /* COMMON_IOCTL_H */
