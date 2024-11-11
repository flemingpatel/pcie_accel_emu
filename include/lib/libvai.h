/* libvai.h - User-space library header for driver interactions via IOCTL
 *
 * Provides function prototypes to interact with the driver.
 *
 */

#ifndef LIBVAI_H
#define LIBVAI_H

#include "../../include/common/common_ioctl.h"


/**
 * @brief Allocates a buffer in the device.
 *
 * @param fd File descriptor of the device.
 * @param size Size of the buffer to allocate in bytes.
 * @return Handle to the allocated buffer on success, 0 on failure.
 */
uint64_t vai_alloc_buffer(int fd, size_t size);

/**
 * @brief Frees a previously allocated buffer in the device.
 *
 * @param fd File descriptor of the device.
 * @param handle Handle of the buffer to free.
 * @return 0 on success, negative error code on failure.
 */
int vai_free_buffer(int fd, uint64_t handle);

/**
 * @brief Loads an AI model into the device.
 *
 * @param fd File descriptor of the device.
 * @param buffer_handle Handle to the buffer containing the model data.
 * @param data_size Size of the model data in bytes.
 * @param model_id Pointer to store the assigned model ID.
 * @return 0 on success, negative error code on failure.
 */
int vai_load_model(int fd, uint64_t buffer_handle, size_t data_size, uint32_t *model_id);

/**
 * @brief Unloads an AI model from the device.
 *
 * @param fd File descriptor of the device.
 * @param model_id ID of the model to unload.
 * @return 0 on success, negative error code on failure.
 */
int vai_unload_model(int fd, uint32_t model_id);

/**
 * @brief Runs inference using a loaded AI model.
 *
 * @param fd File descriptor of the device.
 * @param model_id ID of the loaded model.
 * @param input_handle Handle to the input data buffer.
 * @param output_handle Handle to the output data buffer.
 * @return 0 on success, negative error code on failure.
 */
int vai_run_inference(int fd, uint32_t model_id, uint64_t input_handle, uint64_t output_handle);

#endif /* LIBVAI_H */
