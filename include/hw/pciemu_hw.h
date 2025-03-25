/*
 * pcie_accel_emu_hw.h: Provides hardware definitions
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#ifndef PCIEMU_HW_H
#define PCIEMU_HW_H

#include <linux/pci_regs.h>

/**
 * --------------------------------------------------------------------------------
 * Vendor and Device Identifiers
 *  - https://github.com/qemu/qemu/blob/stable-8.1/include/hw/pci/pci.h
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_VENDOR_ID 0x1234
#define PCIEMU_HW_DEVICE_ID 0x1001
#define PCIEMU_HW_REVISION 0x01

/**
 * --------------------------------------------------------------------------------
 * Base Address Registers (BARs)
 * BAR0: Control registers, DMA config, AI commands
 * BAR1: Dedicated device (internal) memory
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_BAR0 0
#define PCIEMU_HW_BAR1 1
#define PCIEMU_HW_BAR_CNT 2

/**
 * --------------------------------------------------------------------------------
 * MMIO Register Addresses and Definitions
 * MMIO Registers in BAR 0
 * --------------------------------------------------------------------------------
 */
/* General Purpose Registers */
#define PCIEMU_HW_BAR0_REG_CNT 4
#define PCIEMU_HW_BAR0_REG_0 0x00
#define PCIEMU_HW_BAR0_REG_1 0x08
#define PCIEMU_HW_BAR0_REG_2 0x10
#define PCIEMU_HW_BAR0_REG_3 0x18

/* IRQ Registers */
#define PCIEMU_HW_BAR0_IRQ_0_RAISE 0x20
#define PCIEMU_HW_BAR0_IRQ_0_LOWER 0x28

/* DMA Configuration Registers */
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC 0x30 /* For contiguous/dma-coherent buffers */
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST 0x38
/* Registers for SG pointer and count (Scatter/Gather) */
#define PCIEMU_HW_BAR0_DMA_SG_PTR 0x40
#define PCIEMU_HW_BAR0_DMA_SG_CNT 0x48
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN 0x50
#define PCIEMU_HW_BAR0_DMA_CFG_CMD 0x58
#define PCIEMU_HW_BAR0_DMA_DOORBELL_RING 0x60

/* Model Management Registers */
#define PCIEMU_HW_BAR0_MODEL_LOAD_ADDR 0x68 /* Address to load the model from */
#define PCIEMU_HW_BAR0_MODEL_LOAD_SIZE 0x70 /* Size of the model to load */

/* Model Inference Registers */
#define PCIEMU_HW_BAR0_MODEL_INPUT_ADDR 0x78 /* Address of input data */
#define PCIEMU_HW_BAR0_MODEL_INPUT_SIZE 0x80 /* Size of input data */
#define PCIEMU_HW_BAR0_MODEL_OUTPUT_ADDR 0x88 /* Address for output data */
#define PCIEMU_HW_BAR0_MODEL_OUTPUT_SIZE 0x90 /* Size for output data */
#define PCIEMU_HW_BAR0_MODEL_BATCH_SIZE 0x98 /* Batch size for inference */

/* After doorbell user does model ops */
/* Control register for model operations */
#define PCIEMU_HW_BAR0_MODEL_CONTROL 0x100

/* MMIO BAR Boundaries */
#define PCIEMU_HW_BAR0_START PCIEMU_HW_BAR0_REG_0
#define PCIEMU_HW_BAR0_END PCIEMU_HW_BAR0_MODEL_CONTROL

/**
 * --------------------------------------------------------------------------------
 * DMA Definitions
 * --------------------------------------------------------------------------------
 */

/* DMA Directions */
#define PCIEMU_HW_DMA_DIRECTION_TO_DEVICE 0x1 /* Host to Device */
#define PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE 0x2 /* Device to Host */

/* DMA Address Capability */
#define PCIEMU_HW_DMA_ADDR_CAPABILITY 64 /* Device supports 64-bit DMA addresses */

/* Dedicated Device Memory Size (BAR1) */
#define PCIEMU_HW_DMA_AREA_SIZE 0x100000 /* 1 MB */
#define PCIEMU_HW_DMA_AREA_START 0x0 /* Start offset in device memory */

/**
 * --------------------------------------------------------------------------------
 * IRQ Definitions
 * --------------------------------------------------------------------------------
 */
/* Number of IRQs supported by the device */
#define PCIEMU_HW_IRQ_CNT 4

/* IRQ Vectors */
#define PCIEMU_HW_IRQ_VECTOR_START 0
#define PCIEMU_HW_IRQ_VECTOR_END 3

/* IRQ Line (INTx) */
#define PCIEMU_HW_IRQ_INTX 0 /* INTA# */

/* IRQ Vectors for Specific Events */
#define PCIEMU_HW_IRQ_DMA_ENDED_VECTOR 0 /* Vector for DMA completion */
#define PCIEMU_HW_IRQ_MODEL_LOADED_VECTOR 1 /* Vector for model loaded */
#define PCIEMU_HW_IRQ_MODEL_UNLOADED_VECTOR 2 /* Vector for model unloaded */
#define PCIEMU_HW_IRQ_MODEL_INFERENCE_DONE_VECTOR 3 /* Vector for inference completion */

/**
 * --------------------------------------------------------------------------------
 * Custom Model Control Commands
 * (We can do DMA directly from them without using a doorbell,
 * and can have dedicated registers)
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_MODEL_CMD_LOAD_MODEL 0x3 /* Command to load a model */
#define PCIEMU_HW_MODEL_CMD_UNLOAD_MODEL 0x4 /* Command to unload a model */
#define PCIEMU_HW_MODEL_CMD_RUN_INFERENCE 0x5 /* Command to run inference */

/**
 * --------------------------------------------------------------------------------
 * Constants for Model Operations
 * --------------------------------------------------------------------------------
 */
#define PCIEMU_HW_MAX_MODEL_SIZE 0x10000 /* 65 KB */
#define PCIEMU_HW_MAX_BATCH_SIZE 4 /* Maximum number of inputs in a batch */

#endif /* PCIEMU_HW_H */
