/*
 * pcie_accel_emu_module.c: Driver module implementation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "driver/pcie_accel_emu_module.h"

/* Module Information */
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
MODULE_DESCRIPTION("Kernel module to drive the virtual pciemu device for custom AI acceleration");
MODULE_AUTHOR("Fleming Patel");

static struct class *pcie_accel_emu_class;
static struct pci_device_id pcie_accel_emu_id_tbl[] = {
	{ PCI_DEVICE(PCIEMU_HW_VENDOR_ID, PCIEMU_HW_DEVICE_ID) },
	{},
};

/* Open File */
static int pcie_accel_emu_open(struct inode *inode, struct file *fp)
{
	unsigned int bar0 = iminor(inode);
	struct pcie_accel_emu_dev *pemu_dev = container_of(inode->i_cdev, struct pcie_accel_emu_dev, cdev);
	dev_info(&pemu_dev->pdev->dev, "%s: requested BAR: %u, device minor: %d", __func__, bar0,
		 MINOR(inode->i_rdev));
	/* only BAR 0 operations are exposed */
	if (bar0 != PCIEMU_HW_BAR0)
		return -ENXIO;
	if (pemu_dev->bars[bar0].len == 0)
		return -EIO;
	fp->private_data = pemu_dev;

	return 0;
}

/* Mmap Function */
static int pcie_accel_emu_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct pcie_accel_emu_dev *pemu_dev = fp->private_data;
	dev_info(&pemu_dev->pdev->dev, "%s: mmap called (vma_start=0x%lx, vma_end=0x%lx)", __func__,
		 vma->vm_start, vma->vm_end);

	/* buffer_handle passed as offset */
	uint64_t handle = vma->vm_pgoff;
	struct pcie_accel_emu_buffer *buffer;

	// TODO implement

	return 0;
}

/* File Operations Structure */
static const struct file_operations pcie_accel_emu_fops = {
	.owner = THIS_MODULE,
	.open = pcie_accel_emu_open,
	.unlocked_ioctl = pcie_accel_emu_ioctl,
	.mmap = pcie_accel_emu_mmap,
};

static void pcie_accel_emu_dev_clean(struct pcie_accel_emu_dev *pemu_dev)
{
	dev_info(&pemu_dev->pdev->dev, "%s: cleaning dev resources", __func__);

	/* reset BARs and unmap */
	for (int i = 0; i < PCIEMU_HW_BAR_CNT; i++) {
		pemu_dev->bars[i].start = 0;
		pemu_dev->bars[i].end = 0;
		pemu_dev->bars[i].len = 0;
		if (pemu_dev->bars[i].mmio)
			pci_iounmap(pemu_dev->pdev, pemu_dev->bars[i].mmio);
	}

	/* clean up model_list */
	struct pcie_accel_emu_model *model, *next_model;
	mutex_lock(&pemu_dev->model_list_lock);
	list_for_each_entry_safe(model, next_model, &pemu_dev->model_list, list) {
		list_del(&model->list);
		kfree(model);
	}
	mutex_unlock(&pemu_dev->model_list_lock);

	/* clean up buffer_list */
	deregister_all_buffers(pemu_dev);

	/* clean up gen_pool */
	if (pemu_dev->device_mem_pool) {
		gen_pool_destroy(pemu_dev->device_mem_pool);
		pemu_dev->device_mem_pool = NULL;
	}
}

static int pcie_accel_emu_dev_init(struct pcie_accel_emu_dev *pemu_dev, struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "%s: initializing dev resources", __func__);
	pemu_dev->pdev = pdev;

	/* initialize bars based on loop indices */
	for (int i = 0; i < PCIEMU_HW_BAR_CNT; i++) {
		unsigned int bar = bar_ids[i];
		pemu_dev->bars[i].start = pci_resource_start(pdev, bar);
		pemu_dev->bars[i].end = pci_resource_end(pdev, bar);
		pemu_dev->bars[i].len = pci_resource_len(pdev, bar);
		dev_dbg(&pdev->dev, "%s: BAR%d start=%pa, end=%pa, len=%pa", __func__, bar,
			&pemu_dev->bars[i].start, &pemu_dev->bars[i].end, &pemu_dev->bars[i].len);

		pemu_dev->bars[i].mmio = pci_iomap(pdev, bar, pemu_dev->bars[i].len);
		if (!pemu_dev->bars[i].mmio) {
			dev_err(&pdev->dev, "%s: cannot map BAR%d", __func__, bar);
			pcie_accel_emu_dev_clean(pemu_dev);
			return -ENOMEM;
		}
		dev_dbg(&pdev->dev, "%s: mapped BAR%d MMIO at %p", __func__, bar, pemu_dev->bars[i].mmio);
	}

	/* initialize ioctl_lock */
	mutex_init(&pemu_dev->ioctl_lock);

	/* initialize completion fields */
	init_completion(&pemu_dev->dma_done);
	init_completion(&pemu_dev->model_ctrl_done);

	/* initialize model related fields */
	atomic_set(&pemu_dev->model_id_counter, 0);
	INIT_LIST_HEAD(&pemu_dev->model_list);
	mutex_init(&pemu_dev->model_list_lock);

	/* initialize buffer related fields */
	atomic64_set(&pemu_dev->buffer_id_counter, 0);
	INIT_LIST_HEAD(&pemu_dev->buffer_list);
	mutex_init(&pemu_dev->buffer_list_lock);

	/* create a gen_pool over the dedicated device memory (BAR1) */
	pemu_dev->device_mem_pool = gen_pool_create(PAGE_SHIFT, -1); // PAGE_SHIFT granularity
	if (!pemu_dev->device_mem_pool) {
		dev_err(&pdev->dev, "%s: failed to create gen_pool", __func__);
		pcie_accel_emu_dev_clean(pemu_dev);
		return -ENOMEM;
	}
	dev_dbg(&pdev->dev, "%s: created DMA gen_pool at %p", __func__, pemu_dev->device_mem_pool);

	/* add device memory region (BAR1) to gen_pool (device_mem_pool) */
	if (gen_pool_add_virt(pemu_dev->device_mem_pool, (unsigned long)pemu_dev->bars[BAR_IDX_1].mmio,
			      pemu_dev->bars[BAR_IDX_1].start, pemu_dev->bars[BAR_IDX_1].len, -1)) {
		dev_err(&pdev->dev, "%s: failed to add memory to gen_pool", __func__);
		pcie_accel_emu_dev_clean(pemu_dev);
		return -ENOMEM;
	}
	dev_dbg(&pdev->dev, "%s: added device memory region to gen_pool", __func__);

	pci_set_drvdata(pdev, pemu_dev);
	return 0;
}

static struct pcie_accel_emu_dev *pcie_accel_emu_alloc_dev(void)
{
	return kmalloc(sizeof(struct pcie_accel_emu_dev), GFP_KERNEL);
}

/* Probe Function */
static int pcie_accel_emu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	dev_info(&pdev->dev, "%s: probe started for Vendor ID=0x%04x, Device ID=0x%04x", __func__, id->vendor,
		 id->device);

	int err;
	int mem_bars;
	struct pcie_accel_emu_dev *pemu_dev;
	dev_t dev_num;
	struct device *dev;

	/* allocate pcie_accel_emu_dev structure */
	pemu_dev = pcie_accel_emu_alloc_dev();
	if (pemu_dev == NULL) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "%s: pcie_accel_emu_alloc_dev failed", __func__);
		goto err_pcie_accel_emu_alloc;
	}

	/* enable PCI device */
	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "%s: pci_enable_device failed", __func__);
		goto err_pci_enable;
	}

	/* set DMA mask */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY));
	if (err) {
		dev_err(&pdev->dev, "%s: dma_set_mask_and_coherent failed", __func__);
		goto err_dma_set_mask;
	}

	/* enable bus mastering (set bus master bit of PCI_COMMAND register) */
	pci_set_master(pdev);

	/* select and request BARs if available */
	mem_bars = pci_select_bars(pdev, IORESOURCE_MEM);
	/* check if BAR0 is available (for registers) */
	if (!(mem_bars & (1 << PCIEMU_HW_BAR0))) {
		dev_err(&pdev->dev, "%s: BAR0 not available", __func__);
		err = -ENXIO;
		goto err_select_region;
	}

	/* check if BAR1 is available (for dedicated device memory) */
	if (!(mem_bars & (1 << PCIEMU_HW_BAR1))) {
		dev_err(&pdev->dev, "%s: BAR1 not available", __func__);
		err = -ENXIO;
		goto err_select_region;
	}

	err = pci_request_selected_regions(pdev, mem_bars, "pcie_accel_emu_device_bars");
	if (err) {
		dev_err(&pdev->dev, "%s: pci_request_selected_regions failed", __func__);
		goto err_req_region;
	}

	/* initialize pcie_accel_emu_dev */
	err = pcie_accel_emu_dev_init(pemu_dev, pdev);
	if (err) {
		dev_err(&pdev->dev, "%s: pcie_accel_emu_dev_init failed", __func__);
		goto err_dev_init;
	}

	/* get device number (base_minor = bar0 and count = nbr of bars)*/
	err = alloc_chrdev_region(&dev_num, PCIEMU_HW_BAR0, PCIEMU_HW_BAR_CNT, "pcie_accel_emu");
	if (err) {
		dev_err(&pdev->dev, "%s: alloc_chrdev_region failed", __func__);
		goto err_alloc_chrdev;
	}

	/* save minor and major */
	pemu_dev->minor = MINOR(dev_num);
	pemu_dev->major = MAJOR(dev_num);

	/* add cdev with file operations */
	cdev_init(&pemu_dev->cdev, &pcie_accel_emu_fops);
	pemu_dev->cdev.owner = THIS_MODULE;
	/* add major/minor to cdev */
	err = cdev_add(&pemu_dev->cdev, MKDEV(pemu_dev->major, pemu_dev->minor), PCIEMU_HW_BAR_CNT);
	if (err) {
		dev_err(&pdev->dev, "%s: cdev_add failed", __func__);
		goto err_cdev_add;
	}

	/* create /dev/ node via udev */
	dev = device_create(pcie_accel_emu_class, &pdev->dev, MKDEV(pemu_dev->major, pemu_dev->minor),
			    pemu_dev, "d%xb%xd%xf%x_bar%u", pci_domain_nr(pdev->bus), pdev->bus->number,
			    PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn), PCIEMU_HW_BAR0);
	if (IS_ERR(dev)) {
		err = PTR_ERR(dev);
		dev_err(&pdev->dev, "%s: device_create failed", __func__);
		goto err_device_create;
	}

	/* enable IRQs */
	err = pcie_accel_emu_irq_enable(pemu_dev);
	if (err) {
		dev_err(&pdev->dev, "%s: pcie_accel_emu_irq_enable failed", __func__);
		goto err_irq_enable;
	}

	dev_info(&pdev->dev, "%s: probe completed successfully", __func__);
	return 0;

err_irq_enable:
	device_destroy(pcie_accel_emu_class, MKDEV(pemu_dev->major, pemu_dev->minor));
err_device_create:
	cdev_del(&pemu_dev->cdev);
err_cdev_add:
	unregister_chrdev_region(MKDEV(pemu_dev->major, pemu_dev->minor), PCIEMU_HW_BAR_CNT);
err_alloc_chrdev:
	pcie_accel_emu_dev_clean(pemu_dev);
err_dev_init:
	pci_release_selected_regions(pdev, mem_bars);
err_req_region:
err_select_region:
	pci_clear_master(pdev);
err_dma_set_mask:
	pci_disable_device(pdev);
err_pci_enable:
	kfree(pemu_dev);
err_pcie_accel_emu_alloc:
	dev_err(&pdev->dev, "%s: failed with error=%d", __func__, err);
	return err;
}

/* Remove Function */
static void pcie_accel_emu_remove(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "%s: removing device", __func__);

	struct pcie_accel_emu_dev *pemu_dev = pci_get_drvdata(pdev);

	/* clean up irq vectors */
	pcie_accel_emu_irq_disable(pemu_dev);

	/* destroy device node */
	device_destroy(pcie_accel_emu_class, MKDEV(pemu_dev->major, pemu_dev->minor));

	/* delete cdev */
	cdev_del(&pemu_dev->cdev);

	/* unregister character device region */
	unregister_chrdev_region(MKDEV(pemu_dev->major, pemu_dev->minor), PCIEMU_HW_BAR_CNT);

	/* clean up pcie_accel_emu_dev */
	pcie_accel_emu_dev_clean(pemu_dev);

	/* release PCI regions */
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));

	/* clear master */
	pci_clear_master(pdev);

	/* free pcie_accel_emu_dev structure */
	kfree(pemu_dev);
}

static struct pci_driver pcie_accel_emu_driver = {
	.name = "pcie_accel_emu",
	.id_table = pcie_accel_emu_id_tbl,
	.probe = pcie_accel_emu_probe,
	.remove = pcie_accel_emu_remove,
};

static char *pcie_accel_emu_devnode(const struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return kasprintf(GFP_KERNEL, "pcie_accel_emu/%s", dev_name(dev));
}

/* Module Initialization */
static int __init pcie_accel_emu_module_init(void)
{
	int err;
	pcie_accel_emu_class = class_create("pcie_accel_emu");
	if (IS_ERR(pcie_accel_emu_class)) {
		printk(KERN_ERR "%s: class_create failed\n", __func__);
		return PTR_ERR(pcie_accel_emu_class);
	}

	pcie_accel_emu_class->devnode = pcie_accel_emu_devnode;
	err = pci_register_driver(&pcie_accel_emu_driver);
	if (err) {
		printk(KERN_ERR "%s: pci_register_driver failed with error=%d\n", __func__, err);
		class_destroy(pcie_accel_emu_class);
		return err;
	}

	printk(KERN_ALERT "%s: success\n", __func__);
	return 0;
}

/* Module Exit */
static void __exit pcie_accel_emu_module_exit(void)
{
	pci_unregister_driver(&pcie_accel_emu_driver);
	printk(KERN_INFO "%s: unregistered PCI driver\n", __func__);

	class_destroy(pcie_accel_emu_class);
	printk(KERN_INFO "%s: destroyed device class\n", __func__);

	printk(KERN_ALERT "%s: success\n", __func__);
}

module_init(pcie_accel_emu_module_init);
module_exit(pcie_accel_emu_module_exit);
