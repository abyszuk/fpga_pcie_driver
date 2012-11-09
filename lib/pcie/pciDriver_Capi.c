/*******************************************************************
 * Change History:
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2008-01-24 14:21:35  marcus
 * Added a CLEAR_INTERRUPT_QUEUE ioctl.
 * Added a sysfs attribute to show the outstanding IRQ queues.
 *
 * Revision 1.7  2008-01-11 10:16:53  marcus
 * Removed unused interrupt code.
 * Added intSource to the WaitFor Interrupt call.
 *
 * Revision 1.6  2007/02/09 17:02:05  marcus
 * Added interrupt descriptor function.
 *
 * Revision 1.5  2006/12/07 18:43:24  marcus
 * Fixed offset when mapping/unmapping a non-aligned BAR.
 *
 * Revision 1.4  2006/11/17 19:00:41  marcus
 * Added default type to the SGlist.
 *
 * Revision 1.3  2006/10/16 16:54:59  marcus
 * Changed malloc to posix_memalign.
 * Fixed memory leak in unmapUserMemory.
 *
 * Revision 1.2  2006/10/13 17:18:33  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "pciDriver.h"
#include "driver/pciDriver.h"

// two helper functions
int pd_getpagesize() {
	return getpagesize();
}

int pd_getpagemask() {
	int pagesize,pagemask,temp;

	pagesize = pd_getpagesize();

	// set pagemask
	for( pagemask=0, temp = pagesize; temp != 1; ) {
		temp = (temp >> 1);
		pagemask = (pagemask << 1)+1;
	}

	return pagemask;
}

int pd_open( int dev, pd_device_t *pci_handle )
{
	int ret;

	/* check for null pointer */
	if (pci_handle == NULL)
		return -1;

	pci_handle->device = dev;
	sprintf( pci_handle->name, "/dev/fpga%d", dev );

	ret = open( pci_handle->name, O_RDWR );
	if (ret < 0)
		return -1;

	pci_handle->handle = ret;

	pthread_mutex_init( &pci_handle->mmap_mutex, NULL );

	return 0;
}

int pd_close(pd_device_t *pci_handle)
{
	pthread_mutex_destroy( &pci_handle->mmap_mutex );

	return close( pci_handle->handle );
}

/* Kernel Memory Functions */

void *pd_allocKernelMemory( pd_device_t *pci_handle, unsigned int size, pd_kmem_t *kmem_handle )
{
	int ret;
	void *mem;
	kmem_handle_t kh;

	/* Check for null pointer */
	if (kmem_handle == NULL)
		return NULL;

	/* Allocate */
	kh.size = size;
	ret = ioctl(pci_handle->handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh );
	if (ret != 0)
		return NULL;

	kmem_handle->handle_id = kh.handle_id;
	kmem_handle->pa = kh.pa;
	kmem_handle->size = size;
	kmem_handle->pci_handle = pci_handle;

	/* Mmap */
	/* This is not fully safe, as a separate process can still open the device independently.
	 * That will use a separate mutex and the race condition can arise.
	 * Posible fix: Do not allow the driver for mutliple openings of a device */
	pthread_mutex_lock( &pci_handle->mmap_mutex );

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_KMEM );
	if (ret != 0)
		goto pd_allockm_err;

	mem = mmap( 0, size, PROT_WRITE | PROT_READ, MAP_SHARED, pci_handle->handle, 0 );
	if ((mem == MAP_FAILED) || (mem == NULL))
		goto pd_allockm_err;

	kmem_handle->mem = mem;

	pthread_mutex_unlock( &pci_handle->mmap_mutex );

	/* Success, return the mmaped address */
	return mem;

	/* On error, unlock and deallocate buffer */
pd_allockm_err:
		pthread_mutex_unlock( &pci_handle->mmap_mutex );
		ioctl(pci_handle->handle, PCIDRIVER_IOC_KMEM_FREE, &kh );
		return NULL;
}

int pd_freeKernelMemory( pd_kmem_t *kmem_handle )
{
	int ret;
	kmem_handle_t kh;

	/* Check for null pointer */
	if (kmem_handle == NULL)
		return -1;

	/* Unmap */
	munmap( kmem_handle->mem, kmem_handle->size );

	/* Free buffer */
	kh.handle_id = kmem_handle->handle_id;
	kh.size = kmem_handle->size;
	kh.pa = kmem_handle->pa;
	ret = ioctl(kmem_handle->pci_handle->handle, PCIDRIVER_IOC_KMEM_FREE, &kh );

	/* I can just return ret, but this is clearer */
	if (ret != 0)
		return ret;

	return 0;
}

/* User Memory Functions */
int pd_mapUserMemory( pd_device_t *pci_handle, void *mem, unsigned int size, pd_umem_t *umem_handle )
{
	int ret;
	umem_handle_t uh;
	umem_sglist_t sgl;

	/* Check for null pointers */
	if (pci_handle == NULL)
		return -1;
	if (umem_handle == NULL)
		return -1;

	uh.vma = (unsigned long)mem;
	uh.size = size;

	/* Lock and Map the memory to their pages */
	ret = ioctl(pci_handle->handle, PCIDRIVER_IOC_UMEM_SGMAP, &uh );
	if (ret != 0)
		return -1;

	umem_handle->pci_handle = pci_handle;
	umem_handle->handle_id = uh.handle_id;

	/* Obtain the scatter/gather list for this memory */
	sgl.handle_id = uh.handle_id;
	sgl.type = PCIDRIVER_SG_MERGED;
	sgl.nents = (size / getpagesize()) + 1;
	posix_memalign( (void**)&(sgl.sg), 16, sgl.nents*sizeof(umem_sgentry_t) );

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_UMEM_SGGET, &sgl );
	if (ret != 0) {
		ioctl( pci_handle->handle, PCIDRIVER_IOC_UMEM_SGUNMAP, &uh );
		free(sgl.sg);
		return -1;
	}

	/* We can do this because (C API) pd_umem_sgentry_t === (Driver API) umem_sgentry_t */
	umem_handle->nents = sgl.nents;
	umem_handle->sg = (pd_umem_sgentry_t*)sgl.sg;

	/* On Success, return 0 */
	return 0;
}

int pd_unmapUserMemory( pd_umem_t *umem_handle )
{
	int ret;
	umem_handle_t uh;

	/* Check for null pointer */
	if (umem_handle == NULL)
		return -1;

	uh.handle_id = umem_handle->handle_id;
	uh.vma = umem_handle->vma;
	uh.size = umem_handle->size;

	ret = ioctl( umem_handle->pci_handle->handle, PCIDRIVER_IOC_UMEM_SGUNMAP, &uh );

	free( umem_handle->sg );

	return ret;
}

/* Sync Functions */
int pd_syncKernelMemory( pd_kmem_t *kmem_handle, int dir )
{
	int ret;
	kmem_sync_t ks;

	/* Check for null pointer */
	if (kmem_handle == NULL)
		return -1;

	ks.handle.handle_id = kmem_handle->handle_id;
	ks.handle.pa = kmem_handle->pa;
	ks.handle.size = kmem_handle->size;

	/* We assume (C API) dir === (Driver API) dir */
	ks.dir = dir;

	ret = ioctl(kmem_handle->pci_handle->handle, PCIDRIVER_IOC_KMEM_SYNC, &ks );
	if (ret != 0)
		return -1;

	/* Success */
	return 0;
}

int pd_syncUserMemory( pd_umem_t *umem_handle, int dir )
{
	int ret;
	umem_handle_t uh;

	/* Check for null pointer */
	if (umem_handle == NULL)
		return -1;

	/* We assume (C API) dir === (Driver API) dir */
	uh.handle_id = umem_handle->handle_id;
	uh.vma = umem_handle->vma;
	uh.size = umem_handle->size;
	uh.dir = dir;

	ret = ioctl(umem_handle->pci_handle->handle, PCIDRIVER_IOC_UMEM_SYNC, &uh );
	if (ret != 0)
		return -1;

	/* Success */
	return 0;
}

/* Interrupt Function */
int pd_waitForInterrupt(pd_device_t *pci_handle, unsigned int int_id )
{
	int ret;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_WAITI, int_id );
	if (ret != 0)
		return -1;

	return 0;
}

int pd_clearInterruptQueue(pd_device_t *pci_handle, unsigned int int_id )
{
	int ret;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_CLEAR_IOQ, int_id );
	if (ret != 0)
		return -1;

	return 0;
}

/* PCI Functions */
int pd_getID( pd_device_t *pci_handle )
{
	int ret;
	pci_board_info info;
	unsigned int id;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		return -1;

	id = (info.vendor_id << 16) || (info.device_id);

	return id;
}

int pd_getBARsize( pd_device_t *pci_handle, unsigned int bar )
{
	int ret;
	pci_board_info info;
	unsigned int id;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	if (bar > 5)
		return -1;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		return -1;

	return info.bar_length[ bar ];
}

void *pd_mapBAR( pd_device_t *pci_handle, unsigned int bar )
{
	int ret;
	void *mem;
	pci_board_info info;
	unsigned int offset;
	unsigned char* ptr;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return NULL;

	if (bar > 5)
		return NULL;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		return NULL;

	/* Mmap */
	/* This is not fully safe, as a separate process can still open the device independently.
	 * That will use a separate mutex and the race condition can arise.
	 * Posible fix: Do not allow the driver for mutliple openings of a device */
	pthread_mutex_lock( &pci_handle->mmap_mutex );

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
	if (ret != 0)
		return NULL;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0+bar );
	if (ret != 0)
		return NULL;

	mem = mmap( 0, info.bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, pci_handle->handle, 0 );

	pthread_mutex_unlock( &pci_handle->mmap_mutex );

	if ((mem == MAP_FAILED) || (mem == NULL))
		return NULL;

	offset = info.bar_start[bar] & pd_getpagemask();

	// adjust pointer
	if (offset != 0) {
		ptr = (unsigned char *)(mem);
		ptr += offset;
		mem = (void *)(ptr);
	}

	return mem;
}

int pd_unmapBAR( pd_device_t *pci_handle, unsigned int bar, void *ptr )
{
	int ret;
	pci_board_info info;
	unsigned int offset;
	unsigned long tmp;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	if (bar > 5)
		return -1;

	ret = ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		return -1;

	offset = info.bar_start[bar] & pd_getpagemask();

	// adjust pointer
	if (offset != 0) {
		tmp = (unsigned long)(ptr);
		tmp -= offset;
		ptr = (void *)(tmp);
	}

	munmap( ptr, info.bar_length[bar] );

	/* Success */
	return 0;
}

unsigned char pd_readConfigByte( pd_device_t *pci_handle, unsigned int addr )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_BYTE;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );

	return cmd.val.byte;
}

unsigned short pd_readConfigWord( pd_device_t *pci_handle, unsigned int addr )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_WORD;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );

	return cmd.val.word;
}

unsigned int pd_readConfigDWord( pd_device_t *pci_handle, unsigned int addr )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_DWORD;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );

	return cmd.val.dword;
}


int pd_writeConfigByte( pd_device_t *pci_handle, unsigned int addr, unsigned char val )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_BYTE;
	cmd.val.byte = val;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );

	return 0;
}

int pd_writeConfigWord( pd_device_t *pci_handle, unsigned int addr, unsigned short val )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_WORD;
	cmd.val.word = val;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );

	return 0;
}

int pd_writeConfigDWord( pd_device_t *pci_handle, unsigned int addr, unsigned int val )
{
	pci_cfg_cmd cmd;

	/* Check for null pointer */
	if (pci_handle == NULL)
		return -1;

	cmd.addr = addr;
	cmd.size = PCIDRIVER_PCI_CFG_SZ_DWORD;
	cmd.val.dword = val;
	ioctl( pci_handle->handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );

	return 0;
}
