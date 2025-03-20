/* dma.c - Direct Memory Access (DMA) operations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Modified by: Fleming Patel
 *
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "dma.h"
#include "irq.h"
#include "pciemu.h"

/* -----------------------------------------------------------------------------
 *  Private
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_dma_addr_mask: Mask the DMA address according to device's capability
 *
 * @dev: Instance of PCIEMUDevice object being used
 * @addr: Address to be masked
 */
static inline dma_addr_t
pciemu_dma_addr_mask(PCIEMUDevice *dev, dma_addr_t addr)
{
    dma_addr_t masked = addr & dev->dma.config.mask;
    if (masked != addr)
    {
        qemu_log_mask(LOG_GUEST_ERROR, "masked (%" PRIx64 ") != addr (%" PRIx64 ") \n", masked, addr);
    }
    return masked;
}

/**
 * pciemu_dma_inside_device_boundaries: Check if addr is inside boundaries
 *
 * @addr: Address to be checked (address in device address space)
 */
static inline bool
pciemu_dma_inside_device_boundaries(dma_addr_t addr)
{
    return (addr <= PCIEMU_HW_DMA_AREA_START + PCIEMU_HW_DMA_AREA_SIZE);
}

/**
 * pciemu_dma_execute: Execute the DMA operation
 *
 * This function is invoked by the QEMU device model when the doorbell register is written.
 * It reads the DMA configuration registers (source, destination, length, command)
 * and then performs a software‐emulated DMA transfer.
 *
 * In our improved design, the destination (for DMA to device) or source (for DMA from device)
 * is located in the dedicated device memory region. That memory is mapped via BAR1 and managed
 * by the driver. The DMA transfer uses the following logic:
 *
 * For DMA_TO_DEVICE:
 * 	- The source is the host buffer (given by the DMA configuration, after masking).
 * 	- The destination is computed as:
 * 		device_mem_virt + (dma->config.txdesc.dst - PCIEMU_HW_DMA_AREA_START)
 * 		where device_mem_virt is the kernel virtual address mapping of BAR1.
 * For DMA_FROM_DEVICE:
 * 	- The source is computed similarly (from device memory).
 * 	- The destination is a host DMA buffer address.
 *
 * After the transfer, an interrupt is raised to notify the driver.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
static void pciemu_dma_execute(PCIEMUDevice *dev)
{
	DMAEngine *dma = &dev->dma;
	/* check if the DMA command is valid */
	if (dma->config.cmd != PCIEMU_HW_DMA_DIRECTION_TO_DEVICE &&
	    dma->config.cmd != PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE)
		return;

	/* get the base pointer to the dedicated device memory (BAR1) */
	void *base_ptr = memory_region_get_ram_ptr(&dev->dev_mem);
	if (!base_ptr) {
		qemu_log_mask(LOG_GUEST_ERROR, "failed to get base pointer for device memory\n");
		return;
	}

	if (dma->config.cmd == PCIEMU_HW_DMA_DIRECTION_TO_DEVICE) {
		/* --- DMA: Host -> Device (TO_DEVICE) ---
		 *
		 * Verify that the destination register value is within device memory boundaries.
		 * pciemu_dma_inside_device_boundaries() is a helper that checks whether the
		 * given offset falls within the valid region of device internal memory.
		 */
		if (!pciemu_dma_inside_device_boundaries(dma->config.txdesc.dst)) {
			qemu_log_mask(LOG_GUEST_ERROR, "dst register out of bounds\n");
			return;
		}

		/* get the source DMA address from the configuration (masking as needed) */
		dma_addr_t src = pciemu_dma_addr_mask(dev, dma->config.txdesc.src);

		/* Compute the destination pointer in the device's dedicated memory.
		 * The driver programs a destination (dev_dst) that is a physical address
		 * within the device's internal memory. Here, we assume PCIEMU_HW_DMA_AREA_START is 0.
		 * So, the offset inside BAR1 is simply:
		 * 	(dma->config.txdesc.dst - PCIEMU_HW_DMA_AREA_START)
		 * We then add that offset to the kernel mapping of BAR1 (dev->device_mem_virt)
		 * to get the virtual address where the data should be copied.
		 */
		dma_addr_t dst_offset = dma->config.txdesc.dst - PCIEMU_HW_DMA_AREA_START;
		void *dst_ptr = base_ptr + dst_offset;

		/* Emulate the DMA transfer:
		 * pci_dma_read() is a helper that, in our simulation, copies data
		 * from the host memory (pointed to by src) into the device memory at dst_ptr.
		 */
		int err = pci_dma_read(&dev->pci_dev, src, dst_ptr, dma->config.txdesc.len);
		if (err) {
			qemu_log_mask(LOG_GUEST_ERROR, "pci_dma_read error=%d\n", err);
		}
	} else {
		/* --- DMA: Device -> Host (FROM_DEVICE) --- */

		/* verify that the source register value is within device memory boundaries */
		if (!pciemu_dma_inside_device_boundaries(dma->config.txdesc.src)) {
			qemu_log_mask(LOG_GUEST_ERROR, "src register out of bounds\n");
			return;
		}

		/* Calculate the source offset in the device memory.
		 * For FROM_DEVICE, the DMA configuration's src field represents an offset
		 * within the device internal memory. We subtract the base and then add it to
		 * the BAR1 virtual mapping.
		 */
		dma_addr_t src_offset = dma->config.txdesc.src - PCIEMU_HW_DMA_AREA_START;
		void *src_ptr = base_ptr + src_offset;

		/* get the destination (host buffer) address from the configuration */
		dma_addr_t dst = pciemu_dma_addr_mask(dev, dma->config.txdesc.dst);

		/* emulate the DMA transfer by copying data from device memory to host memory */
		int err = pci_dma_write(&dev->pci_dev, dst, src_ptr, dma->config.txdesc.len);
		if (err) {
			qemu_log_mask(LOG_GUEST_ERROR, "pci_dma_write error=%d\n", err);
		}
	}

	/* after the DMA operation completes, raise an interrupt to signal completion */
	pciemu_irq_raise(dev, PCIEMU_HW_IRQ_DMA_ENDED_VECTOR);
}

/* -----------------------------------------------------------------------------
 *  Public
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_dma_config_txdesc_src: Configure the source register
 *
 * The source register inside the transfer descriptor (txdesc)
 * describes source of the DMA operation. It can be :
 *  - the bus address pointing to RAM (or other) when direction is "to device"
 *  - the offset inside the DMA memory area when direction is "from device"
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_src(PCIEMUDevice *dev, dma_addr_t src)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.src = src;
}

/**
 * pciemu_dma_config_txdesc_dst: Configure the destination register
 *
 * The destination register inside the transfer descriptor (txdesc)
 * describes destination of the DMA operation. It can be :
 *  - the offset inside the DMA memory area when direction is "to device"
 *  - the bus address pointing to RAM (or other) when direction is "from device"
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_dst(PCIEMUDevice *dev, dma_addr_t dst)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.dst = dst;
}

/**
 * pciemu_dma_config_txdesc_len: Configure the length register
 *
 * The length register inside the transfer descriptor (txdesc) describes
 * the size of the DMA operation in bytes.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_len(PCIEMUDevice *dev, dma_size_t size)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.len = size;
}

/**
 * pciemu_dma_config_cmd: Configure the command register
 *
 * The command register can take the following values (pciemu_hw.h);
 *   - PCIEMU_HW_DMA_DIRECTION_TO_DEVICE - DMA to device memory (dma->buff)
 *   - PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE - DMA from device memory (dma->buff)
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_cmd(PCIEMUDevice *dev, dma_cmd_t cmd)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.cmd = cmd;
}

/**
 * pciemu_dma_doorbell_ring: Reception of a doorbell
 *
 * When the host (or other device) writes to the doorbell register
 * it is signaling to the DMA engine to start executing the DMA.
 * At this point, it is assumed that the host has already (and properly)
 * configured all necessary DMA engine registers.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_doorbell_ring(PCIEMUDevice *dev)
{
    /* atomic access of dma.status may not be neeeded as the MMIO access
     * will be normally serialized.
     * Though not really necessary, it can show that we need to think of
     * atomic accessing regions, especially if the device is a bit more
     * complex.
     */
    DMAStatus status = qatomic_cmpxchg(&dev->dma.status, DMA_STATUS_IDLE,
                                       DMA_STATUS_EXECUTING);
    if (status == DMA_STATUS_EXECUTING)
        return;
    pciemu_dma_execute(dev);
    qatomic_set(&dev->dma.status, DMA_STATUS_IDLE);
}

/**
 * pciemu_dma_reset: DMA reset
 *
 * Resets the DMA block for the instantiated PCIEMUDevice object.
 * This can be considered a hard reset as we do not wait for the
 * current operation to finish.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_reset(PCIEMUDevice *dev)
{
    DMAEngine *dma = &dev->dma;
    dma->status = DMA_STATUS_IDLE;
    dma->config.txdesc.src = 0;
    dma->config.txdesc.dst = 0;
    dma->config.txdesc.len = 0;
    dma->config.cmd = 0;

    /* clear the internal buffer */
    memset(dma->buff, 0, PCIEMU_HW_DMA_AREA_SIZE);
}

/**
 * pciemu_dma_init: DMA initialization
 *
 * Initializes the DMA block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
void pciemu_dma_init(PCIEMUDevice *dev, Error **errp)
{
    /* Basically reset the DMA engine */
    pciemu_dma_reset(dev);

    /* and set the DMA mask, which does not change */
    dev->dma.config.mask = DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY);
}


/**
 * pciemu_dma_fini: DMA finalization
 *
 * Finalizes the DMA block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being finalized
 */
void pciemu_dma_fini(PCIEMUDevice *dev)
{
    pciemu_dma_reset(dev);
    dev->dma.status = DMA_STATUS_OFF;
}
