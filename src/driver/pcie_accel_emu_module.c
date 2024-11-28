/**
 * pcie_accel_emu.c
 * Driver module implementation
 * Author: Fleming Patel
 *
 */

#include "driver/pcie_accel_emu_module.h"


/* Module Information */
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
MODULE_DESCRIPTION("Kernel module to drive the virtual pciemu device for custom AI acceleration");
MODULE_AUTHOR("Fleming Patel");


static struct class *pcie_accel_emu_class;
static struct pci_device_id pcie_accel_emu_id_tbl[] = {
        {PCI_DEVICE(PCIEMU_HW_VENDOR_ID, PCIEMU_HW_DEVICE_ID)},
        {},
};

/* Open File */
static int pcie_accel_emu_open(struct inode *inode, struct file *fp)
{
    unsigned int bar = iminor(inode);
    struct pcie_accel_emu_dev *pemu_dev = container_of(inode->i_cdev, struct pcie_accel_emu_dev, cdev);
    /* only BAR 0 operations */
    if (bar != PCIEMU_HW_BAR0)
        return -ENXIO;
    if (pemu_dev->bar.len == 0)
        return -EIO;
    fp->private_data = pemu_dev;

    dev_info(&(pemu_dev->pdev->dev), "pcie_accel_emu_open - success");
    return 0;
}

/* Mmap Function */
static int pcie_accel_emu_mmap(struct file *fp, struct vm_area_struct *vma)
{
    return 0;
}

/* File Operations Structure */
static const struct file_operations pcie_accel_emu_fops = {
        .owner = THIS_MODULE,
        .open = pcie_accel_emu_open,
        .mmap = pcie_accel_emu_mmap,
        .unlocked_ioctl = pcie_accel_emu_ioctl,
};

static void pcie_accel_emu_dev_clean(struct pcie_accel_emu_dev *pemu_dev)
{
    pemu_dev->bar.start = 0;
    pemu_dev->bar.end = 0;
    pemu_dev->bar.len = 0;
    if (pemu_dev->bar.mmio)
        pci_iounmap(pemu_dev->pdev, pemu_dev->bar.mmio);
}

static int pcie_accel_emu_dev_init(struct pcie_accel_emu_dev *pemu_dev, struct pci_dev *pdev)
{
    const unsigned int bar = PCIEMU_HW_BAR0;
    pemu_dev->pdev = pdev;

    /* Initialize struct with BAR 0 info */
    pemu_dev->bar.start = pci_resource_start(pdev, bar);
    pemu_dev->bar.end = pci_resource_end(pdev, bar);
    pemu_dev->bar.len = pci_resource_len(pdev, bar);
    pemu_dev->bar.mmio = pci_iomap(pdev, bar, pemu_dev->bar.len);
    if (!pemu_dev->bar.mmio)
    {
        dev_err(&(pdev->dev), "cannot map BAR %u\n", bar);
        pcie_accel_emu_dev_clean(pemu_dev);
        return -ENOMEM;
    }
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
    int err;
    int mem_bars;
    struct pcie_accel_emu_dev *pemu_dev;
    dev_t dev_num;
    struct device *dev;

    /* allocate pcie_accel_emu_dev structure */
    pemu_dev = pcie_accel_emu_alloc_dev();
    if (pemu_dev == NULL)
    {
        err = -ENOMEM;
        dev_err(&(pdev->dev), "pcie_accel_emu_alloc - failed\n");
        goto err_pcie_accel_emu_alloc;
    }

    /* initialize ioctl_lock */
    mutex_init(&pemu_dev->ioctl_lock);

    /* initialize model related fields */
    init_completion(&pemu_dev->model_ctrl_done);
    atomic_set(&pemu_dev->model_id_counter, 0);
    INIT_LIST_HEAD(&pemu_dev->model_list);
    mutex_init(&pemu_dev->model_list_lock);

    /* initialize buffer related fields */
    atomic64_set(&pemu_dev->buffer_id_counter, 0);
    INIT_LIST_HEAD(&pemu_dev->buffer_list);
    mutex_init(&pemu_dev->buffer_list_lock);

    /* enable the PCI device */
    err = pci_enable_device(pdev);
    if (err)
    {
        dev_err(&pdev->dev, "pci_enable_device - failed\n");
        goto err_pci_enable;
    }

    /* set DMA mask */
    err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY));
    if (err)
    {
        dev_err(&pdev->dev, "dma_set_mask_and_coherent - failed\n");
        goto err_dma_set_mask;
    }

    /* enable bus mastering */
    pci_set_master(pdev);

    /* select and request BARs */
    mem_bars = pci_select_bars(pdev, IORESOURCE_MEM);
    if (!(mem_bars & (1 << PCIEMU_HW_BAR0)))
    {
        dev_err(&pdev->dev, "BAR0 not available\n");
        err = -ENXIO;
        goto err_select_region;
    }

    err = pci_request_selected_regions(pdev, mem_bars, "pcie_accel_emu_device_bars");
    if (err)
    {
        dev_err(&pdev->dev, "pci_request_selected_regions - failed\n");
        goto err_req_region;
    }

    /* initialize pcie_accel_emu_dev */
    err = pcie_accel_emu_dev_init(pemu_dev, pdev);
    if (err)
    {
        dev_err(&pdev->dev, "pcie_accel_emu_dev_init - failed\n");
        goto err_dev_init;
    }

    /* get device number range (base_minor = bar0 and count = nbr of bars)*/
    err = alloc_chrdev_region(&dev_num, PCIEMU_HW_BAR0, PCIEMU_HW_BAR_CNT, "pcie_accel_emu");
    if (err)
    {
        dev_err(&(pdev->dev), "alloc_chrdev_region - failed\n");
        goto err_alloc_chrdev;
    }

    /* save minor and major */
    pemu_dev->minor = MINOR(dev_num);
    pemu_dev->major = MAJOR(dev_num);

    /* connect cdev with file operations */
    cdev_init(&pemu_dev->cdev, &pcie_accel_emu_fops);
    pemu_dev->cdev.owner = THIS_MODULE;
    /* add major/min range to cdev */
    err = cdev_add(&pemu_dev->cdev, MKDEV(pemu_dev->major, pemu_dev->minor), PCIEMU_HW_BAR_CNT);
    if (err)
    {
        dev_err(&(pdev->dev), "cdev_add - failed\n");
        goto err_cdev_add;
    }

    /* create /dev/ node via udev */
    dev = device_create(
            pcie_accel_emu_class,
            &pdev->dev,
            MKDEV(pemu_dev->major, pemu_dev->minor),
            pemu_dev,
            "d%xb%xd%xf%x_bar%u",
            pci_domain_nr(pdev->bus),
            pdev->bus->number,
            PCI_SLOT(pdev->devfn),
            PCI_FUNC(pdev->devfn),
            PCIEMU_HW_BAR0);
    if (IS_ERR(dev))
    {
        err = PTR_ERR(dev);
        dev_err(&(pdev->dev), "device_create - failed\n");
        goto err_device_create;
    }

    /* enable IRQs */
    err = pcie_accel_emu_irq_enable(pemu_dev);
    if (err) {
        dev_err(&(pdev->dev), "pcie_accel_emu_irq_enable - failed\n");
        goto err_irq_enable;
    }

    dev_info(&(pdev->dev), "pcie_accel_emu_probe - success\n");
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
    dev_err(&pdev->dev, "pcie_accel_emu_probe - failed with error=%d\n", err);
    return err;
}

/* Remove Function */
static void pcie_accel_emu_remove(struct pci_dev *pdev)
{
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

    pr_info("pcie_accel_emu_remove - success\n");
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
    if (IS_ERR(pcie_accel_emu_class))
    {
        pr_err("class_create - failed\n");
        return PTR_ERR(pcie_accel_emu_class);
    }
    pcie_accel_emu_class->devnode = pcie_accel_emu_devnode;

    err = pci_register_driver(&pcie_accel_emu_driver);
    if (err)
    {
        pr_err("pci_register_driver - failed\n");
        class_destroy(pcie_accel_emu_class);
        return err;
    }

    pr_info("pcie_accel_emu_module_init - success\n");
    return 0;
}

/* Module Exit */
static void __exit pcie_accel_emu_module_exit(void)
{
    pci_unregister_driver(&pcie_accel_emu_driver);
    class_destroy(pcie_accel_emu_class);
    pr_debug("pcie_accel_emu_module_exit - success\n");
}

module_init(pcie_accel_emu_module_init);
module_exit(pcie_accel_emu_module_exit);
