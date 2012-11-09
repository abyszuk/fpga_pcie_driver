#ifndef _PCIDRIVER_SYSFS_H
#define _PCIDRIVER_SYSFS_H
int pcidriver_sysfs_initialize_kmem(pcidriver_privdata_t *privdata, int id, struct class_device_attribute *sysfs_attr);
int pcidriver_sysfs_initialize_umem(pcidriver_privdata_t *privdata, int id, struct class_device_attribute *sysfs_attr);
void pcidriver_sysfs_remove(pcidriver_privdata_t *privdata, struct class_device_attribute *sysfs_attr);

#ifdef ENABLE_IRQ
SYSFS_GET_FUNCTION(pcidriver_show_irq_count);
SYSFS_GET_FUNCTION(pcidriver_show_irq_queues);
#endif

/* prototypes for sysfs operations */
SYSFS_GET_FUNCTION(pcidriver_show_mmap_mode);
SYSFS_SET_FUNCTION(pcidriver_store_mmap_mode);
SYSFS_GET_FUNCTION(pcidriver_show_mmap_area);
SYSFS_SET_FUNCTION(pcidriver_store_mmap_area);
SYSFS_GET_FUNCTION(pcidriver_show_kmem_count);
SYSFS_GET_FUNCTION(pcidriver_show_kbuffers);
SYSFS_SET_FUNCTION(pcidriver_store_kmem_alloc);
SYSFS_SET_FUNCTION(pcidriver_store_kmem_free);
SYSFS_GET_FUNCTION(pcidriver_show_umappings);
SYSFS_SET_FUNCTION(pcidriver_store_umem_unmap);
#endif
