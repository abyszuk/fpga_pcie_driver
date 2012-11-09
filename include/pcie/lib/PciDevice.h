#ifndef PD_PCIDEVICE_H_
#define PD_PCIDEVICE_H_

/********************************************************************
 * 
 * October 10th, 2006
 * Guillermo Marcus - Universitaet Mannheim
 * 
 * $Revision: 1.5 $
 * $Date: 2008-01-24 14:21:36 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2008-01-11 10:14:21  marcus
 * Added intSource to the interrupt wait function.
 *
 * Revision 1.3  2006/11/17 18:51:08  marcus
 * Support both types of SG-lists at runtime.
 *
 * Revision 1.2  2006/11/17 16:51:05  marcus
 * Added page info values, and functions to get the bus and slot of a device,
 * required for compatibility with the uelib.
 *
 * Revision 1.1  2006/10/13 17:18:34  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include <pthread.h>

namespace pciDriver {

// Forward references
class KernelMemory;
class UserMemory;
	
class PciDevice {
private:
	unsigned int pagesize;
	unsigned int pageshift;
	unsigned int pagemask;
	
protected:
	int handle;
	int device;
	char name[50];
	pthread_mutex_t mmap_mutex;
public:
	PciDevice(int number);
	~PciDevice();
	
	void open();
	void close();

	int getHandle();
	unsigned short getBus();
	unsigned short getSlot();

	KernelMemory& allocKernelMemory( unsigned int size );
	UserMemory& mapUserMemory( void *mem, unsigned int size, bool merged );
	inline UserMemory& mapUserMemory( void *mem, unsigned int size ) 
		{ return mapUserMemory(mem,size,true); }

	inline void mmap_lock() { pthread_mutex_lock( &mmap_mutex ); }
	inline void mmap_unlock() { pthread_mutex_unlock( &mmap_mutex ); }
	
	void waitForInterrupt(unsigned int int_id);
	void clearInterruptQueue(unsigned int int_id);
	
	unsigned int getBARsize(unsigned int bar);
	void *mapBAR(unsigned int bar);
	void unmapBAR(unsigned int bar, void *ptr);
	
	unsigned char readConfigByte(unsigned int addr);
	unsigned short readConfigWord(unsigned int addr);
	unsigned int readConfigDWord(unsigned int addr);
	
	void writeConfigByte(unsigned int addr, unsigned char val);
	void writeConfigWord(unsigned int addr, unsigned short val);
	void writeConfigDWord(unsigned int addr, unsigned int val);
};
	
}

#endif /*PD_PCIDEVICE_H_*/
