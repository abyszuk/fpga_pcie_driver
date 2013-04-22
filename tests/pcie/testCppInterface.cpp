/*******************************************************************
 * This is a test program for the C++ interface of the 
 * pciDriver library.
 * 
 * $Revision: 1.5 $
 * $Date: 2007-02-09 17:03:09 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2006/11/17 18:56:12  marcus
 * Added optional dump of contents of the BARs.
 *
 * Revision 1.3  2006/10/30 19:38:20  marcus
 * Added test to check buffer contents.
 *
 * Revision 1.2  2006/10/16 16:56:28  marcus
 * Added nice comment at the start.
 *
 *******************************************************************/

#include "lib/pciDriver.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace pciDriver;
using namespace std;

#define MAX_KBUF (8*1024*1024)
//#define MAX_KBUF (8*1024)
#define MAX_UBUF (64*1024*1024)

void testDevice( int i );
void testPCIconfig(pciDriver::PciDevice *dev);
void testPCImmap(pciDriver::PciDevice *dev);
void testKernelMemory(pciDriver::PciDevice *dev);
void testUserMemory(pciDriver::PciDevice *dev);

int main() 
{
	int i;
	
	for(i=0;i<4;i++) {
		testDevice( i );
	}

	return 0;
}

void testDevice( int i ) {
	pciDriver::PciDevice *device;
		
	try {
		cout << "Trying device " << i << " ... ";
		device = new pciDriver::PciDevice( i );
		cout << "found" << endl;
	} catch (Exception& e) {
		cout << "failed: " << e.toString() << endl;
		return;
	}

	testPCIconfig(device);
	testPCImmap(device);
	testKernelMemory(device);
	testUserMemory(device);

/*
		testMmapMode(handle);
		testMmapArea(handle);
		
		testPCIinfo(handle);
		testPCIconfig(handle);
		
		testPCImmap(handle);
		
		testKbuf(handle);
		testKmmap(handle);		
		testUmap(handle);

		printf("Press any key...\n");
		getchar();
*/

}

void testPCIconfig(pciDriver::PciDevice *dev)
{
	int i,j;
	unsigned int val;
	
	dev->open();
	
	// Config in byte mode
	cout << " Reading Config Area in Byte mode:" << endl;

	for(i=0;i<32;i++) {
		cout << "  " << setfill('0') << setw(3) << i*8 << ": ";
		for(j=0;j<8;j++) {
			val = dev->readConfigByte( (i*8)+j );
			cout << hex << setw(2) << val << " ";
		}
		cout << endl;
	}
	cout << dec;

	cout << " Reading Config Area in Word mode:" << endl;

	for(i=0;i<32;i++) {
		cout << "  " << setfill('0') << setw(3) << i*8 << ": ";
		for(j=0;j<8;j+=2) {
			val = dev->readConfigWord( (i*8)+j );
			cout << hex << setw(4) << val << " ";
		}
		cout << endl;
	}
	cout << dec;

	cout << " Reading Config Area in DWord mode:" << endl;

	for(i=0;i<32;i++) {
		cout << "  " << setfill('0') << setw(3) << i*8 << ": ";
		for(j=0;j<8;j+=4) {
			val = dev->readConfigDWord( (i*8)+j );
			cout << hex << setw(8) << val << " ";
		}
		cout << endl;
	}
	cout << dec;

	
	dev->close();
}

void testPCImmap(pciDriver::PciDevice *dev)
{
	int i;
	void *bar;
	unsigned int size;
	unsigned int *plx;
	
	dev->open();
	
	for(i=0;i<6;i++) {
		cout << "Mapping BAR " << i << " ... ";
		try {
			bar = dev->mapBAR(i);
			cout << "mapped ";
			dev->unmapBAR(i,bar);
			cout << "unmapped ";
			size = dev->getBARsize(i);
			cout << size << endl;
		} catch (Exception& e) {
			cout << "failed" << endl;
		}
	}

#if 0
	bar = dev->mapBAR(0);
	plx = static_cast<unsigned int *>(bar);
	for(i=0;i<(512/4);i++)
		cout << "BAR0[" << i*4 << "]: " << hex << plx[i] << endl;
	dev->unmapBAR(0,bar);
	getchar();
#endif
	dev->close();
}

void testKernelMemory(pciDriver::PciDevice *dev)
{
	KernelMemory *km;
	unsigned int size;
	unsigned int *buf;
	int i, err;
	
	dev->open();
	
	for(size=1024;size<=MAX_KBUF;size*=2) {
		try {
			cout << size << ": ";
			km = &(dev->allocKernelMemory(size));
			cout << "created ( ";
			cout << setw(8) << hex << km->getPhysicalAddress() << " - " << km->getSize() << " ) ";
			cout << dec;
			
			buf = static_cast<unsigned int *>(km->getBuffer());

			km->sync( KernelMemory::BIDIRECTIONAL );
#if 0			
			cout << endl;
			for(i=0; i< (size >> 2); i++) {
				cout << hex << setw(8) << buf[i] << " ";
			}
			cout << endl;
#endif
					
			for(i=0; i< (size >> 2); i++) {
				buf[i] = ~i;
			}

			err = 0;
			for(i=0; i< (size >> 2); i++) {
				if (buf[i] != ~i) err++;
			}
			if (err != 0)
				cout << "BufTest-failed(" << err << ") ";
			else
				cout << "BufTest-passed ";

			km->sync( KernelMemory::BIDIRECTIONAL );

#if 0			
			cout << endl;
			for(i=0; i< (size >> 2); i++) {
				cout << hex << setw(8) << buf[i] << " ";
			}
			cout << endl;
#endif					
			
//			getchar();
			
			delete km;
			cout << "deleted" << endl;
		} catch (Exception& e) {
			cout << "failed: " << e.toString() << endl;
		}	
	}
	
	dev->close();
}

void testUserMemory(pciDriver::PciDevice *dev)
{
	UserMemory *um;
	void *mem;
	unsigned int size,i;
	
	dev->open();
	
	for(size=1024;size<=MAX_UBUF;size*=2) {
		try {
			cout << size << ": ";

			posix_memalign( (void**)&mem, 16, size );

			um = &(dev->mapUserMemory(mem,size));			

			cout << "mapped ( ";
			cout << um->getSGcount() << " entries ) ";
//			cout << setw(8) << hex << um-> << " - " << km->getSize() << " ) ";
//			cout << dec;

			delete um;
			cout << "unmapped" << endl;
			
			free( mem );
			
		} catch (Exception& e) {
			cout << "failed: " << e.toString() << endl;
		}	
	}
	
	// SG example
	try {
		size = 1*1024*1024;	// 1MB
		posix_memalign( (void**)&mem, 16, size );

		cout << "  Allocating " << size << " bytes" << endl;

		um = &(dev->mapUserMemory(mem,size));
		
		cout << "   SG entries: " << um->getSGcount() << endl;
		
		for(i=0;i<um->getSGcount();i++) {
			cout << i << ": " << hex << setw(8) << um->getSGentryAddress(i) << " - " << um->getSGentrySize(i) << dec << endl;
		}

		delete um;
		free( mem );
		
	} catch (Exception& e) {
		cout << "failed: " << e.toString() << endl;
	}	
	
	dev->close();
}
