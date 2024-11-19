/**
 * pcie_accel_emu_hw.h
 * Provides hardware definitions.
 *
 */

#ifndef PCIEMU_HW_H
#define PCIEMU_HW_H

#include <linux/pci_regs.h>
#include <linux/types.h>

/**
 * --------------------------------------------------------------------------------
 * Vendor and Device Identifiers
 *  - https://github.com/qemu/qemu/blob/stable-8.1/include/hw/pci/pci.h
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_VENDOR_ID 0x1234
#define PCIEMU_HW_DEVICE_ID 0x1001
#define PCIEMU_HW_REVISION  0x01

/**
 * --------------------------------------------------------------------------------
 * Base Address Registers (BARs)
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_BAR0  0
#define PCIEMU_HW_CNT   1

/**
 * --------------------------------------------------------------------------------
 * MMIO Register Addresses and Definitions
 * MMIO Registers in BAR 0
 * --------------------------------------------------------------------------------
 */
/* General Purpose Registers */
#define PCIEMU_HW_BAR0_REG_0    0x00
#define PCIEMU_HW_BAR0_REG_1    0x08
#define PCIEMU_HW_BAR0_REG_2    0x10
#define PCIEMU_HW_BAR0_REG_3    0x18

/* IRQ Registers */
#define PCIEMU_HW_BAR0_IRQ_0_RAISE  0x20
#define PCIEMU_HW_BAR0_IRQ_0_LOWER  0x28

/* DMA Configuration General Purpose Registers */
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC   0x30
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST   0x38
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN   0x40
#define PCIEMU_HW_BAR0_DMA_CFG_CMD          0x48
#define PCIEMU_HW_BAR0_DMA_DOORBELL_RING    0x50

/* After doorbell user does AI ops */
#define PCIEMU_HW_BAR0_AI_MODEL_CONTROL     0x58

/* MMIO BAR Boundaries */
#define PCIEMU_HW_BAR0_START    PCIEMU_HW_BAR0_REG_0
#define PCIEMU_HW_BAR0_END      PCIEMU_HW_BAR0_AI_MODEL_CONTROL

/**
 * --------------------------------------------------------------------------------
 * DMA Definitions
 * --------------------------------------------------------------------------------
 */

/* DMA Directions */
#define PCIEMU_HW_DMA_DIRECTION_TO_DEVICE   0x1 /* Host to Device */
#define PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE 0x2 /* Device to Host */

/* DMA Address Capability */
#define PCIEMU_HW_DMA_ADDR_CAPABILITY   64  /* Device supports 64-bit DMA addresses */

/* DMA Area in Device Memory */
#define PCIEMU_HW_DMA_AREA_SIZE     0x100000    /* 1 MB */
#define PCIEMU_HW_DMA_AREA_START    0x0         /* Start offset in device memory */

/**
 * --------------------------------------------------------------------------------
 * IRQ Definitions
 * --------------------------------------------------------------------------------
 */
/* Number of IRQs supported by the device */
#define PCIEMU_HW_IRQ_CNT   4

/* IRQ Vectors */
#define PCIEMU_HW_IRQ_VECTOR_START 0
#define PCIEMU_HW_IRQ_VECTOR_END   3

/* IRQ Line (INTx) */
#define PCIEMU_HW_IRQ_INTX  0   /* INTA# */

/* IRQ Vectors for Specific Events */
#define PCIEMU_HW_IRQ_DMA_ENDED_VECTOR      0   /* Vector for DMA completion */
#define PCIEMU_HW_IRQ_MODEL_LOADED_VECTOR   1   /* Vector for model loaded */
#define PCIEMU_HW_IRQ_MODEL_UNLOADED_VECTOR 2   /* Vector for model unloaded */
#define PCIEMU_HW_IRQ_INFERENCE_DONE_VECTOR 3   /* Vector for inference completion */

/* IRQ Addresses for Acknowledgment */
#define PCIEMU_HW_IRQ_DMA_ENDED_ADDR    PCIEMU_HW_BAR0_IRQ_0_RAISE
#define PCIEMU_HW_IRQ_DMA_ACK_ADDR      PCIEMU_HW_BAR0_IRQ_0_LOWER

/**
 * --------------------------------------------------------------------------------
 * Custom AI Control Commands
 * (We can do DMA directly from them without using a doorbell,
 * and can have dedicated registers)
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_AI_CMD_LOAD_MODEL     0x3 /* Command to load a model */
#define PCIEMU_HW_AI_CMD_UNLOAD_MODEL   0x4 /* Command to unload a model */
#define PCIEMU_HW_AI_CMD_RUN_INFERENCE  0x5 /* Command to run inference */

/**
 * --------------------------------------------------------------------------------
 * Constants for AI Operations
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_AI_MAX_MODEL_SIZE 0x100000    /* 1 MB */
#define PCIEMU_HW_AI_MAX_BATCH_SIZE 1024        /* Maximum number of inputs in a batch */

#endif /* PCIEMU_HW_H */
