/*******************************************************************
 * This is a test program for the C interface of the 
 * pciDriver library.
 * 
 * $Revision: 1.2 $
 * $Date: 2008-05-30 11:38:15 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2006-10-16 16:56:56  marcus
 * Initial version. Tests the C interface of the library.
 *
 *******************************************************************/

#include "lib/pciDriver.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_KBUF (8*1024*1024)
#define MAX_UBUF (64*1024*1024)

void testDevice( int i );
void testPCIconfig(pd_device_t *dev);
void testPCImmap(pd_device_t *dev);
void testKernelMemory(pd_device_t *dev);
void testUserMemory(pd_device_t *dev);

int main() 
{
	int i;
	
	for(i=0;i<4;i++) {
		testDevice( i );
	}

	return 0;
}

void testDevice( int i )
{
	pd_device_t dev;
	int ret;
	
	printf("Trying device %d ...", i);
	ret = pd_open( i, &dev );

	if (ret != 0) {
		printf("failed\n");
		return;
	}

	printf("ok\n");	

	testPCIconfig(&dev);
	testPCImmap(&dev);
	testKernelMemory(&dev);
	testUserMemory(&dev);
	
	pd_close( &dev );
}

void testPCIconfig(pd_device_t *dev)
{
	int i,j,ret;
	
	printf(" Testing PCI config ... \n");
	printf("  Reading PCI config area in byte mode ... \n");
	for(i=0;i<32;i++) {
		printf("   %03d: ", i*8 );
		for(j=0;j<8;j++) {
			ret = pd_readConfigByte( dev, (i*8)+j );
			printf("%02x ",ret);
		}
		printf("\n");
	}

	printf("  Reading PCI config area in word mode ... \n");
	for(i=0;i<32;i++) {
		printf("   %03d: ", i*8 );
		for(j=0;j<8;j+=2) {
			ret = pd_readConfigWord( dev, (i*8)+j );
			printf("%04x ",ret);
		}
		printf("\n");
	}

	printf("  Reading PCI config area in double-word mode ... \n");
	for(i=0;i<32;i++) {
		printf("   %03d: ", i*8 );
		for(j=0;j<8;j+=4) {
			ret = pd_readConfigDWord( dev, (i*8)+j );
			printf("%08x ",ret);
		}
		printf("\n");
	}
}

void testPCImmap(pd_device_t *dev)
{
	int i,ret;
	void *bar;
	unsigned int size;
	
	for(i=0;i<6;i++) {
		printf("Mapping BAR %d ...",i);
		bar = pd_mapBAR( dev, i );
		if (bar == NULL) {
			printf("failed\n");
			continue;
		}
		printf("mapped ");

		ret=pd_unmapBAR( dev,i,bar );
		if (ret < 0) {
			printf("failed\n");
			continue;
		}
		printf("unmapped ");
		size = pd_getBARsize( dev,i );
		printf( "%d\n", size );
	}
}

void testKernelMemory(pd_device_t *dev)
{
	pd_kmem_t km;
	void *buf;
	unsigned int size,ret;
	
	for(size=1024;size<=MAX_KBUF;size*=2) {
		printf("%d: ", size );
		
		buf = pd_allocKernelMemory( dev, size, &km );
		if (buf == NULL) {
			printf("failed\n");
			break;
		}
		printf("created ( %08x - %08x ) ", km.pa, km.size ); 

		ret = pd_freeKernelMemory( &km );
		if (ret < 0) {
			printf("failed\n");
			break;
		}
		printf("deleted\n" );
	}
}

void testUserMemory(pd_device_t *dev)
{
	pd_umem_t um;
	void *mem;
	unsigned int size,i,ret;
	
	for(size=1024;size<=MAX_UBUF;size*=2) {
		printf("%d: ", size );

		posix_memalign( (void**)&mem, 16, size );
		if (mem == NULL) {
			printf("failed\n");
			return;
		}

		ret = pd_mapUserMemory( dev, mem, size, &um );
		if (ret < 0) {
			printf("failed\n");
			break;
		}
		printf("mapped ( %d entries ) ", um.nents );

		ret = pd_unmapUserMemory( &um );
		if (ret < 0) {
			printf("failed\n");
			break;
		}
		printf("unmapped\n" );

		free( mem );
	}
	
	// SG example
	size = 1*1024*1024;	// 1MB
	posix_memalign( (void**)&mem, 16, size );

	printf("  Allocating %d bytes\n", size);

	ret = pd_mapUserMemory( dev, mem, size, &um );
	if (ret < 0) {
		printf("failed\n");
		return;
	}
		
	printf("   SG entries: %d\n", um.nents );
		
	for(i=0;i<um.nents;i++) {
		printf("%d: %08x - %08x\n", i, um.sg[i].addr, um.sg[i].size);
	}

	ret = pd_unmapUserMemory( &um );
	if (ret < 0) {
		printf("failed\n");
		return;
	}

	free( mem );
}

