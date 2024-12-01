/*
 * pcie_accel_emu_irq.c: Driver IRQ related implementation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "driver/pcie_accel_emu_module.h"


/* IRQ Handler */
static irqreturn_t pcie_accel_emu_irq_handler(int irq, void *data)
{
    struct pcie_accel_emu_dev *pemu_dev = data;
    int vector = -1;

    dev_info(&pemu_dev->pdev->dev, "%s: irq = %d dev = %d", __func__, irq, pemu_dev->major);

    /* identify which vector triggered the interrupt */
    for (int i = 0; i < PCIEMU_HW_IRQ_CNT; i++)
    {
        if (irq == pemu_dev->irq.irq_nums[i])
        {
            vector = i;
            break;
        }
    }

    if (vector == -1)
    {
        dev_warn(&pemu_dev->pdev->dev, "%s: unhandled irq = %d dev = %d", __func__, irq, pemu_dev->major);
        return IRQ_NONE;
    }

    /*
     * Handle the interrupt based on the vector
     * We don't do any post-process, here for debug
     */
    switch (vector)
    {
        case PCIEMU_HW_IRQ_MODEL_LOADED_VECTOR:
            dev_info(&pemu_dev->pdev->dev, "irq_handler - model load interrupt");
            /* complete the operation */
            complete(&pemu_dev->model_ctrl_done);
            break;

        case PCIEMU_HW_IRQ_MODEL_UNLOADED_VECTOR:
            dev_info(&pemu_dev->pdev->dev, "irq_handler - model unload interrupt");
            /* complete the operation */
            complete(&pemu_dev->model_ctrl_done);
            break;

        case PCIEMU_HW_IRQ_MODEL_INFERENCE_DONE_VECTOR:
            dev_info(&pemu_dev->pdev->dev, "irq_handler - inference done interrupt");
            /* complete the operation */
            complete(&pemu_dev->model_ctrl_done);
            break;

        default:
            // it won't hit this
            return IRQ_NONE;
    }

    /* acknowledge the interrupt by writing the vector number to the ACK address */
    iowrite32(vector, pemu_dev->irq.mmio_ack_irq);
    return IRQ_HANDLED;
}

static int pcie_accel_emu_irq_enable_msi(struct pcie_accel_emu_dev *pemu_dev)
{
    int msi_vecs_req = PCIEMU_HW_IRQ_CNT;
    int msi_vecs;
    int err;

    dev_info(&pemu_dev->pdev->dev, "%s: requesting %d MSI vectors", __func__, msi_vecs_req);

    msi_vecs = pci_alloc_irq_vectors(pemu_dev->pdev, msi_vecs_req, msi_vecs_req, PCI_IRQ_MSI);
    if (msi_vecs < 0)
    {
        dev_err(&pemu_dev->pdev->dev, "%s: failed, vectors %d", __func__, msi_vecs);
        return -ENOSPC;
    }

    if (msi_vecs != msi_vecs_req)
    {
        pci_free_irq_vectors(pemu_dev->pdev);
        dev_err(&pemu_dev->pdev->dev, "%s: allocated %d MSI (out of %d requested)", __func__, msi_vecs, msi_vecs_req);
        return -ENOSPC;
    }

    /* map the single MMIO ACK address */
    pemu_dev->irq.mmio_ack_irq = pemu_dev->bar.mmio + PCIEMU_HW_BAR0_IRQ_0_LOWER;

    /* request IRQs for all vectors */
    for (int i = 0; i < PCIEMU_HW_IRQ_CNT; i++)
    {
        pemu_dev->irq.irq_nums[i] = pci_irq_vector(pemu_dev->pdev, i);
        if (pemu_dev->irq.irq_nums[i] < 0)
        {
            pci_free_irq_vectors(pemu_dev->pdev);
            dev_err(&pemu_dev->pdev->dev, "%s: vector %d out of range", __func__, i);
            return -EINVAL;
        }

        err = request_irq(pemu_dev->irq.irq_nums[i], pcie_accel_emu_irq_handler, 0, "pcie_accel_emu_irq_handler",
                          pemu_dev);
        if (err)
        {
            dev_err(&pemu_dev->pdev->dev, "%s: failed to request irq %d (%d)", __func__, pemu_dev->irq.irq_nums[i],
                    err);
            pci_free_irq_vectors(pemu_dev->pdev);
            return err;
        }
    }
    return 0;
}

/* Enable MSI IRQ */
int pcie_accel_emu_irq_enable(struct pcie_accel_emu_dev *pemu_dev)
{
    return pcie_accel_emu_irq_enable_msi(pemu_dev);
}

/* Disable MSI IRQ */
void pcie_accel_emu_irq_disable(struct pcie_accel_emu_dev *pemu_dev)
{
    /* free each allocated IRQ */
    for (int i = 0; i < PCIEMU_HW_IRQ_CNT; i++)
    {
        if (pemu_dev->irq.irq_nums[i] > 0)
        {
            free_irq(pemu_dev->irq.irq_nums[i], pemu_dev);
            dev_dbg(&pemu_dev->pdev->dev, "%s: freed IRQ %d", __func__, pemu_dev->irq.irq_nums[i]);
        }
    }

    /* free the MSI vectors */
    pci_free_irq_vectors(pemu_dev->pdev);
    dev_dbg(&pemu_dev->pdev->dev, "%s: freed MSI vectors", __func__);
}
