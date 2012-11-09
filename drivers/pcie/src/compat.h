/**
 *
 * @file compat.h
 * @author Michael Stapelberg
 * @date 2009-04-05
 * @brief Contains compatibility definitions for the different linux kernel versions to avoid
 * putting ifdefs all over the driver code.
 *
 */
#ifndef _COMPAT_H
#define _COMPAT_H

/* dev_name is the wrapper one needs to use to access what was formerly called
 * bus_id in struct device. However, before 2.6.26, direct access was necessary,
 * so we provide our own version. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
static inline const char *dev_name(struct device *dev) {
	return dev->bus_id;
}
#endif

/* SetPageLocked disappeared in v2.6.27 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	#define compat_lock_page SetPageLocked
	#define compat_unlock_page ClearPageLocked
#else
	/* in v2.6.28, __set_page_locked and __clear_page_locked was introduced */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
		#define compat_lock_page __set_page_locked
		#define compat_unlock_page __clear_page_locked
	#else
		/* However, in v2.6.27 itself, neither of them is there, so
		 * we need to use our own function fiddling with bits inside
		 * the page struct :-\ */
		static inline void compat_lock_page(struct page *page) {
			__set_bit(PG_locked, &page->flags);
		}

		static inline void compat_unlock_page(struct page *page) {
			__clear_bit(PG_locked, &page->flags);
		}
	#endif
#endif

/* Before 2.6.13, simple_class was the standard interface. Nowadays, it's just called class */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)

	#define class_compat class_simple

	/* These functions are redirected to their old corresponding functions */
	#define class_create(module, name) class_simple_create(module, name)
	#define class_destroy(type) class_simple_destroy(type)
	#define class_device_destroy(unused, devno) class_simple_device_remove(devno)
	#define class_device_create(type, unused, devno, devpointer, nameformat, minor, unused) \
		class_simple_device_add(type, devno, devpointer, nameformat, minor)
	#define class_set_devdata(classdev, privdata) classdev->class_data = privdata
	#define DEVICE_ATTR_COMPAT
	#define sysfs_attr_def_name(name) class_device_attr_##name
	#define sysfs_attr_def_pointer privdata->class_dev
	#define SYSFS_GET_FUNCTION(name) ssize_t name(struct class_device *cls, char *buf)
	#define SYSFS_SET_FUNCTION(name) ssize_t name(struct class_device *cls, const char *buf, size_t count)
	#define SYSFS_GET_PRIVDATA (pcidriver_privdata_t*)cls->class_data

#else

/* In 2.6.26, device.h was changed quite significantly. Luckily, it only affected
   type/function names, for the most part. */
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	#define class_device_attribute device_attribute
	#define CLASS_DEVICE_ATTR DEVICE_ATTR
	#define class_device device
	#define class_data dev
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	#define class_device_create(type, parent, devno, devpointer, nameformat, minor, privdata) \
		device_create(type, parent, devno, privdata, nameformat, minor)
#else
	#define class_device_create(type, parent, devno, devpointer, nameformat, minor, unused) \
		device_create(type, parent, devno, nameformat, minor)
#endif
	#define class_device_create_file device_create_file
	#define class_device_remove_file device_remove_file
	#define class_device_destroy device_destroy
	#define DEVICE_ATTR_COMPAT struct device_attribute *attr,
	#define class_set_devdata dev_set_drvdata

	#define sysfs_attr_def_name(name) dev_attr_##name
	#define sysfs_attr_def_pointer privdata->class_dev
	#define SYSFS_GET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, char *buf)
	#define SYSFS_SET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
	#define SYSFS_GET_PRIVDATA dev_get_drvdata(dev)

//#endif

#define class_compat class

#endif

/* The arguments of IRQ handlers have been changed in 2.6.19. It's very likely that
   int irq will disappear somewhen in the future (current is 2.6.29), too. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	#define IRQ_HANDLER_FUNC(name) irqreturn_t name(int irq, void *dev_id)
#else
	#define IRQ_HANDLER_FUNC(name) irqreturn_t name(int irq, void *dev_id, struct pt_regs *regs)
#endif

/* atomic_inc_return appeared in 2.6.9, at least in CERN scientific linux, provide
   compatibility wrapper for older kernels */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
static int atomic_inc_return(atomic_t *variable) {
	atomic_inc(variable);
	return atomic_read(variable);
}
#endif

/* sg_set_page is available starting at 2.6.24 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)

#define sg_set_page(sg, set_page, set_length, set_offset) do { \
	(sg)->page = set_page; \
	(sg)->length = set_length; \
	(sg)->offset = set_offset; \
} while (0)

#endif

/* Before 2.6.20, disable was not an atomic counter, so this check was needed */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#define pci_disable_device(pdev) do { \
	if (pdev->is_enabled) \
		pci_disable_device(pdev); \
} while (0)
#endif

/* Before 2.6.24, scatter/gather lists did not need to be initialized */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	#define sg_init_table(sg, nr_pages)
#endif

/* SA_SHIRQ was renamed to IRQF_SHARED in 2.6.24 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	#define request_irq(irq, irq_handler, modname, privdata) request_irq(irq, irq_handler, IRQF_SHARED, modname, privdata)
#else
	#define request_irq(irq, irq_handler, modname, privdata) request_irq(irq, irq_handler, SA_SHIRQ, modname, privdata)
#endif

/* In 2.6.13, io_remap_page_range was removed in favor for io_remap_pfn_range which works on
   more platforms and allows more memory space */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
#define io_remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	io_remap_pfn_range(vmap, vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vm_page_prot)
#else
#define io_remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	io_remap_page_range(vmap, vm_start, bar_addr, bar_length, vm_page_prot)
#endif

/* In 2.6.10, remap_pfn_range was introduced, see io_remap_pfn_range_compat */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
#define remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	remap_pfn_range(vmap, vm_start, (bar_addr >> PAGE_SHIFT), bar_length, vm_page_prot)

#define remap_pfn_range_cpua_compat(vmap, vm_start, cpua, size, vm_page_prot) \
	remap_pfn_range(vmap, vm_start, page_to_pfn(virt_to_page((void*)cpua)), size, vm_page_prot)

#else
#define remap_pfn_range_compat(vmap, vm_start, bar_addr, bar_length, vm_page_prot) \
	remap_page_range(vmap, vm_start, bar_addr, bar_length, vm_page_prot)

#define remap_pfn_range_cpua_compat(vmap, vm_start, cpua, size, vm_page_prot) \
	remap_page_range(vmap, vm_start, virt_to_phys((void*)cpua), size, vm_page_prot)
#endif

/**
 * Go over the pages of the kmem buffer, and mark them as reserved.
 * This is needed, otherwise mmaping the kernel memory to user space
 * will fail silently (mmaping /dev/null) when using remap_xx_range.
 */
static inline void set_pages_reserved_compat(unsigned long cpua, unsigned long size)
{
	/* Starting in 2.6.15, the PG_RESERVED bit was removed.
	   See also http://lwn.net/Articles/161204/ */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	struct page *page, *last_page;

	page = virt_to_page(cpua);
	last_page = virt_to_page(cpua + size - 1);

	for (; page <= last_page; page++)
               SetPageReserved(page);
#endif
}

#endif
