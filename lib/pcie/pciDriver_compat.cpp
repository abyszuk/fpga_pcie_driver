/*
 *
 * @file pciDriver_compat.cpp
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief Implements the old pciDriver API, to be transparent to applications.
 *
 */

#include "pciDriver.h"
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>

#define MAX_DEVICES 8

//*******************************************************
// Internal to this compatibility section

namespace pciDriver_compat {

//*******************************************************
// PciDriverEnumerator Class
// Used internally by the compatibility classes to
// identify the PciDevice based on a handle

	class PciDeviceEnumerator {
	protected:
		static PciDeviceEnumerator *instance;
		pciDriver::PciDevice *devices[MAX_DEVICES];
	public:
		static PciDeviceEnumerator *getInstance() {
			if (PciDeviceEnumerator::instance == NULL) {
				PciDeviceEnumerator::instance = new PciDeviceEnumerator();
			}
			return PciDeviceEnumerator::instance;
		}
		
		pciDriver::PciDevice *getDevice(unsigned int index) {
			if (index < MAX_DEVICES) {
				return devices[index];
			}
			return NULL;
		}

		int registerDevice(unsigned int index,pciDriver::PciDevice *dev) {
			if ((index < MAX_DEVICES) && (dev != NULL)) {
				devices[index] = dev;
				return true;
			}
			return false;
		}

		bool unregisterDevice(unsigned int index) {
			if (index < MAX_DEVICES) {
				devices[index] = NULL;
				return true;
			}
			return false;
		}
		
		int static countDevices() {
			int i,cnt;
			struct stat tmp_stat;
			char name[20];
			
			cnt=0;
			for(i=0;i<MAX_DEVICES;i++) {
				sprintf( name, "/dev/fpga%d", i );
				if ( stat( name, &tmp_stat ) == 0 )
					cnt++;
			}
			return cnt;
		}
	};
	
	// static variable
	PciDeviceEnumerator *PciDeviceEnumerator::instance;
}

//*******************************************************
// Kmem Class

KMem::KMem() {
	km = NULL;
}

KMem::~KMem() {
	if (km != NULL)
		delete km;
	km = NULL;
}

KMem::KMem(int handle, int order) {
	int ret;
	ret = this->Alloc(handle,order);
	if (ret == 0)
		throw "error allocating!\n";
}

int KMem::Alloc(int handle, int order) {
	unsigned int size;
	pciDriver::PciDevice *dev;
	unsigned int pagesize = getpagesize();

	// get the pcidevice from the handle somehow
	dev = pciDriver_compat::PciDeviceEnumerator::getInstance()->getDevice( handle );
	
	size = (1 << order)*pagesize;
	km = &(dev->allocKernelMemory(size));

	if (km != NULL)
		return 1;
	else
		return 0;
}

int KMem::Free(void) {
	if (this->km != NULL) {
		delete km;
		km = NULL;
	}
	return 1;
}

unsigned long KMem::GetPhysicalAddress(void) {
	if (km == NULL)
		return 0;
		
	return km->getPhysicalAddress();
}

unsigned int *KMem::GetBuffer(void) {
	if (km == NULL)
		return NULL;

	return static_cast<unsigned int *>(km->getBuffer());
}

void KMem::Sync(void) {
        km->sync(pciDriver::KernelMemory::BIDIRECTIONAL);
}

//*******************************************************
// MemoryPageList Class

MemoryPageList::MemoryPageList() {
	um = NULL;
	pagesize = getpagesize();
	unsigned int temp;

	for( pageshift=0, temp = pagesize; temp != 1; pageshift++ )
		temp = (temp >> 1);

}

MemoryPageList::~MemoryPageList() {
	if (um != NULL)
		delete um;
	um = NULL;
}

MemoryPageList::MemoryPageList(int handle, unsigned int *buffer, unsigned int size) {

	pagesize = getpagesize();
	unsigned int temp;

	for( pageshift=0, temp = pagesize; temp != 1; pageshift++ )
		temp = (temp >> 1);

	this->LockBuffer(handle,buffer,size);
}

bool MemoryPageList::LockBuffer(int handle, unsigned int *buffer, unsigned int size) {

	pciDriver::PciDevice *dev;

	// get the pcidevice from the handle somehow
	dev = pciDriver_compat::PciDeviceEnumerator::getInstance()->getDevice( handle );
	
	// here SGlist MUST NOT BE MERGED. The uelib assumes single pages.
	um = &(dev->mapUserMemory(buffer,size,false));

	return (um != NULL);	
}

bool MemoryPageList::UnlockBuffer(void) {
	if (um != NULL) {
		delete um;
		um = NULL;
	}
	return true;
}

bool MemoryPageList::IsUsed(void) {
	return (um != NULL);
}

unsigned int MemoryPageList::GetNumberOfPages(void) {
	/*
	 *  NOTE: This requires the driver to be compile WITHOUT 
	 *        the MERGE_SGENTRIES flag.
	 */
	return um->getSGcount();
}

unsigned int MemoryPageList::GetPhysicalAddress(unsigned int index) {
	/*
	 *  NOTE: This requires the driver to be compile WITHOUT 
	 *        the MERGE_SGENTRIES flag.
	 */

	unsigned int addr = um->getSGentryAddress(index);
	
	if (index == 0) {
		addr = addr >> pageshift;
		addr = addr << pageshift;
	}
	
	return addr;
}

unsigned int MemoryPageList::operator[] (unsigned int index) {
	/*
	 *  NOTE: This requires the driver to be compile WITHOUT 
	 *        the MERGE_SGENTRIES flag.
	 */

	unsigned int addr = um->getSGentryAddress(index);
	
	if (index == 0) {
		addr = addr >> pageshift;
		addr = addr << pageshift;
	}
	
	return addr;
}

unsigned int MemoryPageList::GetFirstPageOffset(void) {
	/*
	 *  NOTE: This requires the driver to be compile WITHOUT 
	 *        the MERGE_SGENTRIES flag.
	 */
	// FIXME: calculate page offset

	unsigned int pg_addr = um->getSGentryAddress(0);
	unsigned int pg_start = pg_addr;
	pg_start = pg_start >> pageshift;
	pg_start = pg_start << pageshift;
	
	return pg_addr-pg_start;
}

void MemoryPageList::Sync(void) {
        um->sync(pciDriver::UserMemory::BIDIRECTIONAL);
}


//*******************************************************
// PciDevice Class

PciDevice::PciDevice() {
	int i;
	dev = NULL;
	for(i=0;i<6;i++)
		bar[i] = NULL;
}

PciDevice::~PciDevice() {
	if (dev != NULL)
		Close();
}

int PciDevice::Open(unsigned int deviceNr) {
	try {
		dev = new pciDriver::PciDevice(deviceNr);
		dev_number = deviceNr;
		
		pciDriver_compat::PciDeviceEnumerator::getInstance()->registerDevice( deviceNr, dev );

		dev->open();
		
	} catch (pciDriver::Exception *e) {
		delete e;
		return -1;
	}
	return 0;
//	return deviceNr;
}

int PciDevice::Close(void) {
	int i;
	
	for(i=0;i<6;i++) {
		if (bar[i] != NULL)
			dev->unmapBAR( i, bar[i] );
		bar[i] = NULL;
	}
	
	if (dev != NULL) {
		
		dev->close();
		delete dev;
		dev = NULL;
		pciDriver_compat::PciDeviceEnumerator::getInstance()->unregisterDevice( dev_number );
		dev_number = -1;
	}
	
	return 0;
}

volatile unsigned int *PciDevice::GetBarAccess(unsigned int barNr) {
	void *ptr;
	
	// map the BAR if neccesary
	if (bar[ barNr ] == NULL) {
		try {
			ptr = dev->mapBAR( barNr );
		} catch (...) {
			return 0;
		}
		if (ptr == NULL)
			return 0;
			
		bar[ barNr ] = ptr;
	}
	return static_cast<unsigned int *>(bar[ barNr ]);
}

bool PciDevice::IsOpen(void) {
	return (dev != NULL);
} 

unsigned char  PciDevice::ReadConfigByte(unsigned int address) {
	return dev->readConfigByte(address);
}

unsigned short PciDevice::ReadConfigWord(unsigned int address) {
	return dev->readConfigWord(address);
}

unsigned int   PciDevice::ReadConfigDWord(unsigned int address) {
	return dev->readConfigDWord(address);
}

void PciDevice::WriteConfigByte(unsigned int address, unsigned char val) {
	dev->writeConfigByte(address,val);
}

void PciDevice::WriteConfigWord(unsigned int address, unsigned short val) {
	dev->writeConfigWord(address,val);
}

void PciDevice::WriteConfigDWord(unsigned int address, unsigned int val) {
	dev->writeConfigDWord(address,val);
}

unsigned int PciDevice::GetBus(void) {
	return dev->getBus();
}

unsigned int PciDevice::GetSlot(void) {
	return dev->getSlot();
}

unsigned short PciDevice::GetVendorId(void) {
	return ReadConfigWord(0);
}

unsigned short PciDevice::GetDeviceId(void) {
	return ReadConfigWord(2);
}

PciDevice::operator int() { return dev_number; }

int PciDevice::GetNumberOfDevices(void) {
	return pciDriver_compat::PciDeviceEnumerator::countDevices();
}
