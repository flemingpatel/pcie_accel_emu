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
        default:
            ret = -ENOTTY;
    }

    /* Unlock the ioctl mutex */
    mutex_unlock(&pemu_dev->ioctl_lock);

    return ret;
}
