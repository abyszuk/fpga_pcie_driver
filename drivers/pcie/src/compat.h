/**
 *
 * @file compat.h
 * @author Michael Stapelberg(prev), Adrian Byszuk(current)
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


/* In 2.6.26, device.h was changed quite significantly. Luckily, it only affected
   type/function names, for the most part. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	#define device_create_compat(type, parent, devno, devpointer, nameformat, minor, privdata) \
		device_create(type, parent, devno, privdata, nameformat, minor)
#else
	#define device_create_compat(type, parent, devno, devpointer, nameformat, minor, unused) \
		device_create(type, parent, devno, nameformat, minor)
#endif

	#define sysfs_attr_def_name(name) dev_attr_##name
	#define SYSFS_GET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, char *buf)
	#define SYSFS_SET_FUNCTION(name) ssize_t name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)


#endif //_COMPAT_H
