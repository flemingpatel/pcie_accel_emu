/* libvai.c - User-space library implementation for driver interactions
 *
 * Implements functions to interact with the driver via IOCTL.
 *
 */

#include "../../include/lib/libvai.h"


/* Allocates a buffer in the device */
uint64_t vai_alloc_buffer(int fd, size_t size)
{
    struct vai_alloc_buffer_arg arg;
    int ret;

    memset(&arg, 0, sizeof(arg));
    arg.size = size;

    ret = ioctl(fd, PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER, &arg);
    if (ret < 0)
    {
        perror("PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER failed");
        return 0;
    }

    return arg.handle;
}

/* Frees a buffer in the device */
int vai_free_buffer(int fd, uint64_t handle)
{
    long ret;

    ret = ioctl(fd, PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER, &handle);
    if (ret < 0)
    {
        perror("PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER failed");
        return -errno;
    }

    return 0;
}

/* Loads an AI model into the device */
int vai_load_model(int fd, uint64_t buffer_handle, size_t data_size, uint32_t *model_id)
{
    struct vai_load_model_arg arg;
    int ret;

    memset(&arg, 0, sizeof(arg));
    arg.buffer_handle = buffer_handle;
    arg.data_size = data_size;

    ret = ioctl(fd, PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL, &arg);
    if (ret < 0)
    {
        perror("PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL failed");
        return -errno;
    }

    *model_id = arg.model_id;
    return 0;
}

/* Unloads an AI model from the device */
int vai_unload_model(int fd, uint32_t model_id)
{
    int ret;

    ret = ioctl(fd, PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL, &model_id);
    if (ret < 0)
    {
        perror("PCIE_ACCEL_EMU_IOCTL_UNLOAD_MODEL failed");
        return -errno;
    }

    return 0;
}

/* Runs inference using a loaded AI model */
int vai_run_inference(int fd, uint32_t model_id, uint64_t input_handle, uint64_t output_handle)
{
    struct vai_run_inference_arg arg;
    int ret;

    memset(&arg, 0, sizeof(arg));
    arg.model_id = model_id;
    arg.input_handle = input_handle;
    arg.output_handle = output_handle;

    ret = ioctl(fd, PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE, &arg);
    if (ret < 0)
    {
        perror("PCIE_ACCEL_EMU_IOCTL_RUN_INFERENCE failed");
        return -errno;
    }

    return 0;
}
