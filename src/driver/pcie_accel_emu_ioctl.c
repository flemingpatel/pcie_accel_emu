/**
 * pcie_accel_emu_ioctl.c
 * Driver IOCTL interface related implementation
 * Author: Fleming Patel
 *
 */

#include "driver/pcie_accel_emu_module.h"
#include "driver/pcie_accel_emu_ioctl.h"


/* Main IOCTL Handler */
long pcie_accel_emu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    struct pcie_accel_emu_dev *pemu_dev = fp->private_data;
    long ret = 0;

    /* lock the ioctl mutex since we will be dealing with shared buffer */
    mutex_lock(&pemu_dev->ioctl_lock);

    switch (cmd)
    {
        case PCIE_ACCEL_EMU_IOCTL_ALLOC_BUFFER:
            break;

        case PCIE_ACCEL_EMU_IOCTL_FREE_BUFFER:
            break;

        case PCIE_ACCEL_EMU_IOCTL_LOAD_MODEL:
            /* signal the device to process the loaded model */
            iowrite32(PCIEMU_HW_MODEL_CMD_LOAD_MODEL, pemu_dev->bar.mmio + PCIEMU_HW_BAR0_MODEL_CONTROL);
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
