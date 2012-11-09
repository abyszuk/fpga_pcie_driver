/**
 *
 * @file KernelMemory.cpp
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief KernelMemory class implementation.
 *
 */

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2007-02-09 17:02:38  marcus
 * Modified Exception handling, made simpler and more standard.
 *
 * Revision 1.2  2006/10/30 19:39:50  marcus
 * Renamed variable to avoid confusions.
 *
 * Revision 1.1  2006/10/13 17:18:32  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "KernelMemory.h"
#include "Exception.h"
#include "driver/pciDriver.h"

#include <sys/ioctl.h>
#include <sys/mman.h>

using namespace pciDriver;

/**
 *
 * Constructor of a KernelMemory object, allocates kernel memory of the specified
 * size and mmaps it.
 *
 * @param size How much memory to allocate
 *
 */
KernelMemory::KernelMemory(PciDevice& dev, unsigned int size)
{
	void *m_ptr;
	kmem_handle_t kh;
	int dev_handle;
	
	dev_handle = dev.getHandle();

	this->device = &dev;
	this->size = size;
	
	/* Allocate */
	kh.size = size;
	if (ioctl(dev_handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh) != 0)
		throw Exception(Exception::ALLOC_FAILED);

	handle_id = kh.handle_id;
	pa = kh.pa;

	/* Mmap */
	/* This is not fully safe, as a separate process can still open the device independently.
	 * That will use a separate mutex and the race condition can arise.
	 * Posible fix: Do not allow the driver for mutliple openings of a device */
	device->mmap_lock();
		
	if (ioctl(dev_handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_KMEM) != 0)
		goto pd_allockm_err;
	
	m_ptr = mmap( 0, size, PROT_WRITE | PROT_READ, MAP_SHARED, dev_handle, 0 );
	if ((m_ptr == MAP_FAILED) || (m_ptr == NULL))
		goto pd_allockm_err;

	this->mem = m_ptr;

	device->mmap_unlock();

	/* Success, Object created successfully */
	return;

	/* On error, unlock, deallocate buffer and throw an exception */
pd_allockm_err:
	device->mmap_unlock();
	ioctl(dev_handle, PCIDRIVER_IOC_KMEM_FREE, &kh);
	throw Exception(Exception::ALLOC_FAILED);
}

/**
 *
 * Destructor of KernelMemory, unmaps the memory and frees it.
 *
 */
KernelMemory::~KernelMemory()
{
	kmem_handle_t kh;
	
	/* Unmap */
	munmap(this->mem, this->size);
	
	/* Free buffer */
	kh.handle_id = handle_id;
	kh.size = size;
	kh.pa = pa;
	if (ioctl(device->getHandle(), PCIDRIVER_IOC_KMEM_FREE, &kh) != 0)
		throw Exception(Exception::INTERNAL_ERROR);
}

/**
 *
 * Syncs the kernel memory to the device.
 *
 */
void KernelMemory::sync(sync_dir dir)
{
	kmem_sync_t ks;

	ks.handle.handle_id = handle_id;
	ks.handle.pa = pa;
	ks.handle.size = size;

	/* We assume (C++ API) dir === (Driver API) dir */	
	ks.dir = dir;

	if (ioctl(device->getHandle(), PCIDRIVER_IOC_KMEM_SYNC, &ks) != 0)
		throw Exception(Exception::INTERNAL_ERROR);
}
