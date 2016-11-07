/**
 *
 * @file base.c
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief Contains the main code which connects all the different parts and does
 * basic driver tasks like initialization.
 *
 * This is a full rewrite of the pciDriver.
 * New default is to support kernel 2.6, using kernel 2.6 APIs.
 *
 */

/*
 * Change History:
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2008-05-30 11:38:15  marcus
 * Added patches for kernel 2.6.24
 *
 * Revision 1.12  2008-01-24 14:21:36  marcus
 * Added a CLEAR_INTERRUPT_QUEUE ioctl.
 * Added a sysfs attribute to show the outstanding IRQ queues.
 *
 * Revision 1.11  2008-01-24 12:53:11  marcus
 * Corrected wait_event condition in waiti_ioctl. Improved the loop too.
 *
 * Revision 1.10  2008-01-14 10:39:39  marcus
 * Set some messages as debug instead of normal.
 *
 * Revision 1.9  2008-01-11 10:18:28  marcus
 * Modified interrupt mechanism. Added atomic functions and queues, to address race conditions. Removed unused interrupt code.
 *
 * Revision 1.8  2007-07-17 13:15:55  marcus
 * Removed Tasklets.
 * Using newest map for the ABB interrupts.
 *
 * Revision 1.7  2007-07-06 15:56:04  marcus
 * Change default status for OLD_REGISTERS to not defined.
 *
 * Revision 1.6  2007-07-05 15:29:59  marcus
 * Corrected issue with the bar mapping for interrupt handling.
 * Added support up to kernel 2.6.20
 *
 * Revision 1.5  2007-05-29 07:50:18  marcus
 * Split code into 2 files. May get merged in the future again....
 *
 * Revision 1.4  2007/03/01 17:47:34  marcus
 * Fixed bug when the kernel memory was less than one page, it was not locked properly, recalling an old mapping issue in this case.
 *
 * Revision 1.3  2007/03/01 17:01:22  marcus
 * comment fix (again).
 *
 * Revision 1.2  2007/03/01 17:00:25  marcus
 * Changed some comment in the log.
 *
 * Revision 1.1  2007/03/01 16:57:43  marcus
 * Divided driver file to ease the interrupt hooks for the user of the driver.
 * Modified Makefile accordingly.
 *
 * From pciDriver.c:
 * Revision 1.11  2006/12/11 16:15:43  marcus
 * Fixed kernel buffer mmapping, and driver crash when application crashes.
 * Buffer memory is now marked reserved during allocation, and mmaped with
 * remap_xx_range.
 *
 * Revision 1.10  2006/11/21 09:50:49  marcus
 * Added PROGRAPE4 vendor/device IDs.
 *
 * Revision 1.9  2006/11/17 18:47:36  marcus
 * Removed MERGE_SGENTRIES flag, now it is selected at runtime with 'type'.
 * Removed noncached in non-prefetchable areas, to allow the use of MTRRs.
 *
 * Revision 1.8  2006/11/17 16:41:21  marcus
 * Added slot number to the PCI info IOctl.
 *
 * Revision 1.7  2006/11/13 12:30:34  marcus
 * Added a IOctl call, to confiure the interrupt response. (testing pending).
 * Basic interrupts are now supported, using a Tasklet and Completions.
 *
 * Revision 1.6  2006/11/08 21:30:02  marcus
 * Added changes after compile tests in kernel 2.6.16
 *
 * Revision 1.5  2006/10/31 07:57:38  marcus
 * Improved the pfn calculation in nopage(), to deal with some possible border
 * conditions. It was really no issue, because they are normally page-aligned
 * anyway, but to be on the safe side.
 *
 * Revision 1.4  2006/10/30 19:37:40  marcus
 * Solved bug on kernel memory not mapping properly.
 *
 * Revision 1.3  2006/10/18 11:19:20  marcus
 * Added kernel 2.6.8 support based on comments from Joern Adamczewski (GSI).
 *
 * Revision 1.2  2006/10/18 11:04:15  marcus
 * Bus Master is only activated when we detect a specific board.
 *
 * Revision 1.1  2006/10/10 14:46:51  marcus
 * Initial commit of the new pciDriver for kernel 2.6
 *
 * Revision 1.9  2006/10/05 11:30:46  marcus
 * Prerelease. Added bus and devfn to pciInfo for compatibility.
 *
 * Revision 1.8  2006/09/25 16:51:07  marcus
 * Added PCI config IOctls, and implemented basic mmap functions.
 *
 * Revision 1.7  2006/09/20 11:12:41  marcus
 * Added Merge SG entries
 *
 * Revision 1.6  2006/09/19 17:22:18  marcus
 * backup commit.
 *
 * Revision 1.5  2006/09/18 17:13:11  marcus
 * backup commit.
 *
 * Revision 1.4  2006/09/15 15:44:41  marcus
 * backup commit.
 *
 * Revision 1.3  2006/08/15 11:40:02  marcus
 * backup commit.
 *
 * Revision 1.2  2006/08/12 18:28:42  marcus
 * Sync with the laptop
 *
 * Revision 1.1  2006/08/11 15:30:46  marcus
 * Sync with the laptop
 *
 */

#include <linux/version.h>

/* Check macros and kernel version first */
#ifndef KERNEL_VERSION
#error "No KERNEL_VERSION macro! Stopping."
#endif

#ifndef LINUX_VERSION_CODE
#error "No LINUX_VERSION_CODE macro! Stopping."
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
#error "This driver has been tested only for Kernel 2.6.8 or above."
#endif

/* Required includes */
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/miscdevice.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,16,0)
	#include <asm/scatterlist.h>
#else
	#include <linux/scatterlist.h>
#endif

#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

/* Configuration for the driver (what should be compiled in, module name, etc...) */
#include "config.h"

/* Compatibility functions/definitions (provides functions which are not available on older kernels) */
#include "compat.h"

/* External interface for the driver */
#include "pciDriver.h"

/* Internal definitions for all parts (prototypes, data, macros) */
#include "common.h"

/* Internal definitions for the base part */
#include "base.h"

/* Internal definitions of the IRQ handling part */
#include "int.h"

/* Internal definitions for kernel memory */
#include "kmem.h"

/* Internal definitions for user space memory */
#include "umem.h"

#include "ioctl.h"

/*************************************************************************/
/* Module device table associated with this driver */
MODULE_DEVICE_TABLE(pci, pcidriver_ids);

/* Module init and exit points */
module_init(pcidriver_init);
module_exit(pcidriver_exit);

/* Module info */
MODULE_AUTHOR("Adrian Byszuk");
MODULE_DESCRIPTION("BPM PCIe board driver");
MODULE_LICENSE("GPL v2");

/**
 *
 * Called when loading the driver
 *
 */
static int __init pcidriver_init(void)
{
	int err;

	/* Register PCI driver. This function returns the number of devices on some
	 * systems, therefore check for errors as < 0. */
	if ((err = pci_register_driver(&pcidriver_driver)) < 0) {
		mod_info("Couldn't register PCI driver. Module not loaded.\n");
		goto init_pcireg_fail;
	}

	mod_info("Module loaded\n");

	return 0;

init_pcireg_fail:
	return err;
}

/**
 *
 * Called when unloading the driver
 *
 */
static void __exit pcidriver_exit(void)
{

	pci_unregister_driver(&pcidriver_driver);

	mod_info("Module unloaded\n");
}

/*************************************************************************/
/* Driver functions */

/**
 *
 * This struct defines the PCI entry points.
 * Will be registered at module init.
 *
 */
static struct pci_driver pcidriver_driver = {
	.name = MODNAME,
	.id_table = pcidriver_ids,
	.probe = pcidriver_probe,
	.remove = pcidriver_remove,
};

/* Misc device */
static int fpga_create_misc_device(pcidriver_privdata_t *priv)
{
	priv->mdev.minor = MISC_DYNAMIC_MINOR;
	priv->mdev.fops = &pcidriver_fops;
	priv->mdev.name = priv->name;
	return misc_register(&priv->mdev);
}

static void fpga_destroy_misc_device(pcidriver_privdata_t *priv)
{
	misc_deregister(&priv->mdev);
}

/**
 *
 * This function is called when installing the driver for a device
 * @param pdev Pointer to the PCI device
 *
 */
static int pcidriver_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int err;
    long slot_number;
	pcidriver_privdata_t *privdata;

	dev_info(&pdev->dev, " probe for device %04x:%04x\n",
		pdev->bus->number, pdev->devfn);

	/* At the moment there is no difference between these boards here, other than
	 * printing a different message in the log.
	 *
	 * However, there is some difference in the interrupt handling functions.
	 */
	if ( (id->vendor == PCIE_XILINX_VENDOR_ID) &&
		(id->device == PCIE_KC705_DEV_ID))
	{
		mod_info( "Found KC705 at %s\n", dev_name(&pdev->dev));
	}
	else if ((id->vendor == PCIE_XILINX_VENDOR_ID) &&
		(id->device == PCIE_ML605_DEVICE_ID))
	{
	/* It is a PCI-E Xilinx ML605 evaluation board */
		mod_info("Found ML605 board at %s\n", dev_name(&pdev->dev));
	}
	else if ((id->vendor == PCIE_XILINX_VENDOR_ID) &&
		(id->device == PCIE_AMC_DEV_ID))
	{
	/* It is a PCI-E Creotech uTCA AMC board */
		mod_info("Found uTCA AMC board at %s\n", dev_name(&pdev->dev));
	}
	else
	{
		/* It is something else */
		mod_info( "Found unknown board (%x:%x) at %s\n", id->vendor, id->device, dev_name(&pdev->dev));
	}

	/* Enable the device */
	if ((err = pci_enable_device(pdev)) != 0) {
		mod_info("Couldn't enable device\n");
		goto probe_pcien_fail;
	}

	/* Set bus master */
	pci_set_master(pdev);

	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err) {
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	}
	if (err) {
		dev_err(&pdev->dev, "No suitable DMA available");
		goto probe_disable_device;
	}

	/* Allocate and initialize the private data for this device */
	if ((privdata = kcalloc(1, sizeof(*privdata), GFP_KERNEL)) == NULL) {
		err = -ENOMEM;
		goto probe_nomem;
	}

	INIT_LIST_HEAD(&(privdata->kmem_list));
	spin_lock_init(&(privdata->kmemlist_lock));
	atomic_set(&privdata->kmem_count, 0);

	INIT_LIST_HEAD(&(privdata->umem_list));
	spin_lock_init(&(privdata->umemlist_lock));
	atomic_set(&privdata->umem_count, 0);

	pci_set_drvdata(pdev, privdata);
	privdata->pdev = pdev;

#ifdef ENABLE_PHYSICAL_SLOT_NUMBER
    /* Convert to decimal. Is this the right place to get the slot number? */
	err = kstrtol(pdev->slot->kobj.name, 10, &slot_number);
	if (err) {
		dev_err(&privdata->pdev->dev, "Error converting slot number to decimal\n");
		goto failed_conv_slot_number;
	}

	dev_info(&pdev->dev, "Creating device name "NODENAME"-%ld\n",
		slot_number);
	snprintf(privdata->name, PCIE_NAME_LEN, NODENAME"-%ld",
		slot_number);
#else
	slot_number = privdata->pdev->bus->number << 8 | privdata->pdev->devfn;
	dev_info(&pdev->dev, "Creating device name "NODENAME"-%04x\n",
		slot_number);
	snprintf(privdata->name, PCIE_NAME_LEN, NODENAME"-%04x",
		slot_number);
#endif

	/* Setup mmaped BARs into kernel space */
	if ((err = pcidriver_probe_irq(privdata)) != 0)
		goto probe_irq_probe_fail;

	/* Populate sysfs attributes for the class device */
	/* TODO: correct errorhandling. ewww. must remove the files in reversed order :-( */
	#define sysfs_attr(name) do { \
			if (class_device_create_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)) != 0) \
				goto probe_device_create_fail; \
			} while (0)
	#ifdef ENABLE_IRQ
	sysfs_attr(irq_count);
	sysfs_attr(irq_queues);
	#endif

	sysfs_attr(mmap_mode);
	sysfs_attr(mmap_area);
	sysfs_attr(kmem_count);
	sysfs_attr(kmem_alloc);
	sysfs_attr(kmem_free);
	sysfs_attr(kbuffers);
	sysfs_attr(umappings);
	sysfs_attr(umem_unmap);
	#undef sysfs_attr

	init_completion(&privdata->user_comp);

	/* The user refcount starts with one to inidicate an active device */
	atomic_set(&privdata->user_refcount, 1);

	mod_info_dbg("pcidriver_probe: user_refcount = %d\n", atomic_read(&privdata->user_refcount));

	/* Device register */
	err = fpga_create_misc_device(privdata);
	if (err) {
		dev_err(&privdata->pdev->dev, "Error creating misc device\n");
		goto failed_misc;
	}

	return 0;

failed_misc:
probe_device_create_fail:
probe_irq_probe_fail:
	pcidriver_irq_unmap_bars(privdata);
#ifdef ENABLE_PHYSICAL_SLOT_NUMBER
failed_conv_slot_number:
#endif
	pci_set_drvdata(pdev, NULL);
	kfree(privdata);
probe_nomem:
probe_disable_device:
	pci_disable_device(pdev);
probe_pcien_fail:
 	return err;
}

static void wait_for_clients(pcidriver_privdata_t *privdata)
{
	/*
	 * Remove the device init value and complete the device if there is
	 * no clients or wait for active clients to finish.
	 */
	if (atomic_dec_and_test(&privdata->user_refcount))
		complete(&privdata->user_comp);

	mod_info_dbg("wait_for_clients: waiting for completion. user_refcount = %d\n", 
		atomic_read(&privdata->user_refcount));
	wait_for_completion(&privdata->user_comp);
	mod_info_dbg("wait_for_clients: finished waiting for completion\n");
}

/**
 *
 * This function is called when disconnecting a device
 *
 */
static void pcidriver_remove(struct pci_dev *pdev)
{
	pcidriver_privdata_t *privdata;

	/* Get private data from the device */
	privdata = pci_get_drvdata(pdev);

	/* Removing the device from sysfs */
	fpga_destroy_misc_device(privdata);

	/* wait for existing user space clients to finish */
	wait_for_clients(privdata);

	/* Removing sysfs attributes from class device */
#if 0
	#define sysfs_attr(name) do { \
			class_device_remove_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)); \
			} while (0)
	#ifdef ENABLE_IRQ
	sysfs_attr(irq_count);
	sysfs_attr(irq_queues);
	#endif

	sysfs_attr(mmap_mode);
	sysfs_attr(mmap_area);
	sysfs_attr(kmem_count);
	sysfs_attr(kmem_alloc);
	sysfs_attr(kmem_free);
	sysfs_attr(kbuffers);
	sysfs_attr(umappings);
	sysfs_attr(umem_unmap);
	#undef sysfs_attr
#endif

	/* Free all allocated kmem buffers before leaving */
	pcidriver_kmem_free_all( privdata );

#ifdef ENABLE_IRQ
	pcidriver_remove_irq(privdata);
#endif

	/* Unset privdata */
	pci_set_drvdata(pdev, NULL);

	/* Releasing privdata */
	kfree(privdata);

	/* Disabling PCI device */
	pci_disable_device(pdev);

	mod_info("Device at %s removed\n", dev_name(&pdev->dev));
}

/*************************************************************************/
/* File operations */
/*************************************************************************/

/**
 * This struct defines the file operation entry points.
 *
 * @see pcidriver_ioctl
 * @see pcidriver_mmap
 * @see pcidriver_open
 * @see pcidriver_release
 *
 */
static struct file_operations pcidriver_fops = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35)
	.owner = THIS_MODULE,
#endif
	.unlocked_ioctl = pcidriver_ioctl,
	.mmap = pcidriver_mmap,
	.open = pcidriver_open,
	.release = pcidriver_release,
};

/**
 *
 * Called when an application open()s a /dev/fpga*, attaches the private data
 * with the file pointer.
 *
 */
int pcidriver_open(struct inode *inode, struct file *filp)
{
	pcidriver_privdata_t *privdata;
	/* Set the private data area for the file */
	struct miscdevice *mdev_ptr = filp->private_data;
	filp->private_data = container_of(mdev_ptr, pcidriver_privdata_t, mdev);

	privdata = filp->private_data;

	if (!atomic_inc_not_zero(&privdata->user_refcount)) {
		mod_info_dbg("pcidriver_open: user_refcount = %d, should be > 0\n", atomic_read(&privdata->user_refcount));
		return -ENXIO;
	}

	mod_info_dbg("pcidriver_open: user_refcount = %d\n", atomic_read(&privdata->user_refcount));

	return 0;
}

/**
 *
 * Called when the application close()s the file descriptor. Does nothing at
 * the moment.
 *
 */
int pcidriver_release(struct inode *inode, struct file *filp)
{
	pcidriver_privdata_t *privdata;

	/* Get the private data area */
	privdata = filp->private_data;

	if (atomic_dec_and_test(&privdata->user_refcount))
		complete(&privdata->user_comp);

	mod_info_dbg("pcidriver_release: user_refcount = %d\n", atomic_read(&privdata->user_refcount));

	return 0;
}

/**
 *
 * This function is the entry point for mmap() and calls either pcidriver_mmap_pci
 * or pcidriver_mmap_kmem
 *
 * @see pcidriver_mmap_pci
 * @see pcidriver_mmap_kmem
 *
 */
int pcidriver_mmap(struct file *filp, struct vm_area_struct *vma)
{
	pcidriver_privdata_t *privdata;
	int ret = 0, bar;

	mod_info_dbg("Entering mmap\n");

	/* Get the private data area */
	privdata = filp->private_data;

	/* Check the current mmap mode */
	switch (privdata->mmap_mode) {
		case PCIDRIVER_MMAP_PCI:
			/* Mmap a PCI region */
			switch (privdata->mmap_area) {
				case PCIDRIVER_BAR0:	bar = 0; break;
				case PCIDRIVER_BAR1:	bar = 1; break;
				case PCIDRIVER_BAR2:	bar = 2; break;
				case PCIDRIVER_BAR3:	bar = 3; break;
				case PCIDRIVER_BAR4:	bar = 4; break;
				case PCIDRIVER_BAR5:	bar = 5; break;
				default:
					mod_info("Attempted to mmap a PCI area with the wrong mmap_area value: %d\n",privdata->mmap_area);
					return -EINVAL;			/* invalid parameter */
					break;
			}
			ret = pcidriver_mmap_pci(privdata, vma, bar);
			break;
		case PCIDRIVER_MMAP_KMEM:
			/* mmap a Kernel buffer */
			ret = pcidriver_mmap_kmem(privdata, vma);
			break;
		default:
			mod_info( "Invalid mmap_mode value (%d)\n",privdata->mmap_mode );
			return -EINVAL;			/* Invalid parameter (mode) */
	}

	return ret;
}

/*************************************************************************/
/* Internal driver functions */
int pcidriver_mmap_pci(pcidriver_privdata_t *privdata, struct vm_area_struct *vmap, int bar)
{
	int ret = 0;
	unsigned long bar_addr;
	unsigned long bar_length, vma_size;
	unsigned long bar_flags;

	mod_info_dbg("Entering mmap_pci\n");

	/* Get info of the BAR to be mapped */
	bar_addr = pci_resource_start(privdata->pdev, bar);
	bar_length = pci_resource_len(privdata->pdev, bar);
	bar_flags = pci_resource_flags(privdata->pdev, bar);

	/* Check sizes */
	vma_size = (vmap->vm_end - vmap->vm_start);
	if ((vma_size != bar_length) &&
	   ((bar_length < PAGE_SIZE) && (vma_size != PAGE_SIZE))) {
		mod_info( "mmap size is not correct! bar: %lu - vma: %lu\n", bar_length, vma_size );
		return -EINVAL;
	}

	if (bar_flags & IORESOURCE_IO) {
		/* Unlikely case, we will mmap a IO region */

		/* IO regions are never cacheable */
#ifdef pgprot_noncached
		vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif

		/* Map the BAR */
		ret = io_remap_pfn_range_compat(
					vmap,
					vmap->vm_start,
					bar_addr,
					bar_length,
					vmap->vm_page_prot);
	} else {
		/* Normal case, mmap a memory region */

		/* Ensure this VMA is non-cached, if it is not flaged as prefetchable.
		 * If it is prefetchable, caching is allowed and will give better performance.
		 * This should be set properly by the BIOS, but we want to be sure. */
		/* adapted from drivers/char/mem.c, mmap function. */
#ifdef pgprot_noncached
/* Setting noncached disables MTRR registers, and we want to use them.
 * So we take this code out. This can lead to caching problems if and only if
 * the System BIOS set something wrong. Check LDDv3, page 425.
 */
//		if (!(bar_flags & IORESOURCE_PREFETCH))
//			vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif

		/* Map the BAR */
		ret = remap_pfn_range_compat(
					vmap,
					vmap->vm_start,
					bar_addr,
					bar_length,
					vmap->vm_page_prot);
	}

	if (ret) {
		mod_info("remap_pfn_range failed\n");
		return -EAGAIN;
	}

	return 0;	/* success */
}
