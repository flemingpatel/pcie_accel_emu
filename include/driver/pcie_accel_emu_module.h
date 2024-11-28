/**
 * pcie_accel_emu.h
 * Provides driver module definitions
 * Author: Fleming Patel
 *
 */

#ifndef PCIE_ACCEL_EMU_MODULE_H
#define PCIE_ACCEL_EMU_MODULE_H

#include "hw/pciemu_hw.h"
#include "pcie_accel_emu_ioctl.h"

#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/completion.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>


/* forward declaration */
struct pcie_accel_emu_dev;

/* BAR Structure */
struct pcie_accel_emu_bar
{
    u64 start;
    u64 end;
    u64 len;
    void __iomem *mmio;
};

/* IRQ Structure */
struct pcie_accel_emu_irq
{
    int irq_nums[PCIEMU_HW_IRQ_CNT];    /* Array to store all IRQ numbers */
    void __iomem *mmio_ack_irq;         /* Single MMIO ACK address */
};

/*
 * Model Structure
 * without tracking models, the driver cannot validate whether a model has been loaded before running inference
 * Malicious or buggy applications could attempt to run inference on invalid buffers or data
 * Usually device doesn't know the concept of model id so keeping only in driver
 */
struct pcie_accel_emu_model
{
    uint32_t model_id;
    uint64_t buffer_handle;
    size_t data_size;
    struct list_head list;
};

/*
 * Buffer Region Structure
 * Not currently used anywhere because looking at genalloc/genpool subsystem instead of custom logic (TODO)
 */
struct pcie_accel_emu_buffer_region
{
    size_t start;              /* Start offset within the DMA area */
    size_t size;               /* Size of the region in bytes */
    bool free;                 /* Allocation status: true if free, false if allocated */
    struct list_head list;     /* Linked list node for inclusion in region_list */
};

struct pcie_accel_emu_buffer
{
    uint64_t handle;                                /* Unique handle for the buffer */
    size_t size;                                    /* Size of the buffer in bytes */
    dma_addr_t dma_handle;                          /* DMA address */
    size_t region_offset;                           /* Offset within the DMA area */
    struct pcie_accel_emu_buffer_region *region;    /* Pointer to the associated region */
    struct list_head list;                          /* Linked list node */
};

struct pcie_accel_emu_dev
{
    struct pci_dev *pdev;
    /* In this simple PCIe device, we only a single BAR (0)
     * We could have an array of size PCI_STD_NUM_BARS to
     * hold information about all bars.
     */
    struct pcie_accel_emu_bar bar;
    struct pcie_accel_emu_irq irq;
    dev_t minor;
    dev_t major;
    struct cdev cdev;
    struct mutex ioctl_lock;            /* Lock for ioctl ops */

    /* Model related fields */
    struct completion model_ctrl_done;  /* Completion for all model operations */

    /* Model List */
    atomic_t model_id_counter;          /* Counter for assigning unique model IDs */
    struct list_head model_list;
    struct mutex model_list_lock;

    /* Buffer List */
    atomic64_t buffer_id_counter;       /* Counter for buffer handles */
    struct list_head buffer_list;
    struct mutex buffer_list_lock;

    /* Region List */
    struct list_head region_list;
    struct mutex region_list_lock;

    /* Device memory address for DMA operations */
    u64 device_memory_address;
};


// move it to irq.h
int pcie_accel_emu_irq_enable(struct pcie_accel_emu_dev *pemu_dev);
void pcie_accel_emu_irq_disable(struct pcie_accel_emu_dev *pemu_dev);

#endif /* PCIE_ACCEL_EMU_MODULE_H */
