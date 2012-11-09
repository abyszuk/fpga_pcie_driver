#ifndef LIBPCIDRIVER_H_
#define LIBPCIDRIVER_H_

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2008-01-11 10:07:53  marcus
 * Removed unused int code.
 * Modified int wait call to include the intSource to wait for. (experimental for cocurrent interrupts support).
 *
 * Revision 1.5  2007/02/09 16:59:52  marcus
 * Added interrupt descriptor and related function.
 *
 * Revision 1.4  2006/11/13 16:44:51  marcus
 * Added direction values as define's.
 *
 * Revision 1.3  2006/10/16 16:57:43  marcus
 * Added nice comment at the start.
 *
 *******************************************************************/

#include <pthread.h>

/* Both APIs are in a single header */

/***********************************************************************************
 *  This is the C API 
 ***********************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* Data types */
typedef struct {
	int handle;					/* PCI device handle */
	int device;					/* Device ID number */
	char name[50];				/* Device Name (node) used */
	pthread_mutex_t mmap_mutex;	/* Mmap mutex used by the device */
} pd_device_t;

/* All Data types are redefined in the C API, even if they match the native driver interface */
typedef struct {
	unsigned long pa;
	unsigned long size;
	void *mem;
	int handle_id;
	pd_device_t *pci_handle;
} pd_kmem_t;

typedef struct {
	unsigned long addr;
	unsigned long size;
} pd_umem_sgentry_t;

typedef struct {
	unsigned long vma;
	unsigned long size;
	int handle_id;
	int nents;
	pd_umem_sgentry_t *sg;
	pd_device_t *pci_handle;
} pd_umem_t;

/* Direction of a Sync operation */
#define PD_DIR_BIDIRECTIONAL	0
#define	PD_DIR_TODEVICE			1
#define PD_DIR_FROMDEVICE		2

int pd_open( int dev, pd_device_t *pci_handle );
int pd_close( pd_device_t *pci_handle );

/* Kernel Memory Functions */
void *pd_allocKernelMemory( pd_device_t *pci_handle, unsigned int size, pd_kmem_t *kmem_handle );
int pd_freeKernelMemory( pd_kmem_t *kmem_handle );

/* User Memory Functions */
int pd_mapUserMemory( pd_device_t *pci_handle, void *mem, unsigned int size, pd_umem_t *umem_handle );
int pd_unmapUserMemory( pd_umem_t *umem_handle );

/* Sync Functions */
int pd_syncKernelMemory( pd_kmem_t *kmem_handle, int dir );
int pd_syncUserMemory( pd_umem_t *umem_handle, int dir );

/* Interrupt Function */
int pd_waitForInterrupt(pd_device_t *pci_handle , unsigned int int_id );
int pd_clearInterruptQueue(pd_device_t *pci_handle , unsigned int int_id );

/* PCI Functions */
int pd_getID( pd_device_t *pci_handle );
int pd_getBARsize( pd_device_t *pci_handle, unsigned int bar );
void *pd_mapBAR( pd_device_t *pci_handle, unsigned int bar );
int pd_unmapBAR( pd_device_t *pci_handle, unsigned int bar, void *ptr );

unsigned char pd_readConfigByte( pd_device_t *pci_handle, unsigned int addr );
unsigned short pd_readConfigWord( pd_device_t *pci_handle, unsigned int addr );
unsigned int pd_readConfigDWord( pd_device_t *pci_handle, unsigned int addr );

int pd_writeConfigByte( pd_device_t *pci_handle, unsigned int addr, unsigned char val );
int pd_writeConfigWord( pd_device_t *pci_handle, unsigned int addr, unsigned short val );
int pd_writeConfigDWord( pd_device_t *pci_handle, unsigned int addr, unsigned int val );

#ifdef __cplusplus
}
#endif

/***********************************************************************************
 *  This is the C++ API 
 ***********************************************************************************/
#ifdef __cplusplus

#include "Exception.h"
#include "PciDevice.h"
#include "KernelMemory.h"
#include "UserMemory.h"

#include "pciDriver_compat.h"

#endif

#endif /*LIBPCIDRIVER_H_*/
