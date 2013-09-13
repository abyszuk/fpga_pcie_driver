#include "lib/pciDriver.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>


class BDA {
public:
	uint32_t pa_h;
	uint32_t pa_l;
	uint32_t ha_h;
	uint32_t ha_l;
	uint32_t length;
	uint32_t control;
	uint32_t next_bda_h;
	uint32_t next_bda_l;
	uint32_t status;

	void write(volatile uint32_t *base) {
		base[0] = pa_h;
		base[1] = pa_l;
		base[2] = ha_h;
		base[3] = ha_l;
		base[4] = next_bda_h;
		base[5] = next_bda_l;
		base[6] = length;
		base[7] = control;		// control is written at the end, starts DMA
	}

	void reset(volatile uint32_t *base) {
		base[7] = 0x0200000A;
	}

	inline void wait_finish(volatile uint32_t *base) {
		//check for END bit
		do {
			status = base[8];
		} while(!(status & 0x1));
	}
};


void testDevice(int i);
void testDirectIO(pciDriver::PciDevice *dev, size_t total_size);
void testDMA(pciDriver::PciDevice *dev, size_t total_size);
void testDMAKernelMemory(uint32_t *bar0, uint32_t *bar2,
		pciDriver::KernelMemory *km, const size_t buf_size,
		const size_t test_len);


int main()
{
	testDevice(0);

	return 0;
}

void testDevice(int i)
{
	pciDriver::PciDevice *dev;
	//Total transfer data count for each test
	const unsigned int dma_total_size = std::numeric_limits<unsigned int>::max();
	const unsigned int dio_total_size = pow(10,9);

	try {
		std::cout << "Trying device " << i << " ... ";
		dev = new pciDriver::PciDevice(i);
		std::cout << "found" << std::endl;

		// Open device
		dev->open();

		testDirectIO(dev, dio_total_size);
		testDMA(dev, dma_total_size);

		// Close device
		dev->close();

		delete dev;

	} catch (pciDriver::Exception& e) {
		std::cout << "failed: " << e.toString() << std::endl;
		return;
	}

}

void testDirectIO(pciDriver::PciDevice *dev,
		size_t total_size)
{
	uint32_t *bar2;
	unsigned int bar2size;
	time_t time_start;
	time_t time_end;
	double time_diff;
	size_t bytes_sent;

	try {
		std::cout << "\n### Starting DirectIO test ###" << std::endl;
		// Map BARs
	    bar2 = static_cast<uint32_t *>(dev->mapBAR(2));

		// Get BAR sizes
		bar2size = dev->getBARsize(2);
		unsigned int bar2len = bar2size/sizeof(uint32_t);
		uint32_t *buf = new uint32_t[bar2len];
		const unsigned int size2mbyte = total_size/pow(2, 20);
		std::cout << "Total transfer size: " << size2mbyte << " MBytes" << std::endl;

		//start measurement
		std::cout << "[Write test]" << std::endl;
		time(&time_start);
		for(bytes_sent = 0; bytes_sent < total_size; bytes_sent += bar2size) {
			memcpy(bar2, buf, bar2size);
		}
		time(&time_end);

		time_diff = difftime(time_end, time_start);;
		std::cout << "Write time: " << std::fixed << std::setprecision(2) <<
			time_diff << " seconds" << std::endl;
		std::cout << "Write speed: " << std::fixed <<
			(bytes_sent/time_diff)/pow(2,20) << " [MB/s]" << std::endl;

		std::cout << "[Read test]" << std::endl;
		time(&time_start);
		for(bytes_sent = 0; bytes_sent < total_size; bytes_sent += bar2size) {
			memcpy(buf, bar2, bar2size);
		}
		time(&time_end);

		time_diff = difftime(time_end, time_start);;
		std::cout << "Read time: " << std::fixed << std::setprecision(2) <<
			time_diff << " seconds" << std::endl;
		std::cout << "Read speed: " << std::fixed <<
			(bytes_sent/time_diff)/pow(2,20) << " [MB/s]" << std::endl;

		delete[] buf;
		dev->unmapBAR(2,bar2);

	} catch(pciDriver::Exception& e) {
		std::cout << "Exception: " << e.toString() << std::endl;
	}
}

void testDMA(pciDriver::PciDevice *dev,
		size_t total_size)
{
	pciDriver::KernelMemory *km;
	uint32_t *bar0, *bar2;
	//buffer sizes for DMA transactions
	const size_t base_size = pow(2, 10); //1KByte
	const size_t top_size = pow(2, 22);; //4MBytes

	try {
		// Map BARs
	    bar0 = static_cast<uint32_t *>(dev->mapBAR(0));
	    bar2 = static_cast<uint32_t *>(dev->mapBAR(2));
		std::cout << "\n### Starting DMA test ###" << std::endl;
		const unsigned int size2mbyte = total_size/pow(2, 20);
		std::cout << "Total transfer size: " << size2mbyte << " MBytes" << std::endl;

		for (size_t buffer_size = base_size; buffer_size <= top_size; buffer_size <<= 1) {
			const unsigned int dma_length = buffer_size/pow(2, 10);
			std::cout << "## DMA length: " << dma_length << " KB" << std::endl;
			// Create buffers
			km = &dev->allocKernelMemory(buffer_size);

			// Test DDR SDRAM memory
			testDMAKernelMemory(bar0, bar2, km, buffer_size, total_size);

			// Delete buffer descriptors
			delete km;
		}

		// Unmap BARs
		dev->unmapBAR(0,bar0);
		dev->unmapBAR(2,bar2);

	} catch(pciDriver::Exception& e) {
		std::cout << "Exception: " << e.toString() << std::endl;
	}
}

void testDMAKernelMemory(
		uint32_t *bar0,
		uint32_t *bar2,
		pciDriver::KernelMemory *km,
		const size_t buf_size,
		const size_t test_len)
{
	BDA dma;
	const unsigned int BASE_DMA_UP = (0x2C >> 2);
	uint32_t * const us_engine = bar0 + BASE_DMA_UP;
	const unsigned int BASE_DMA_DOWN = (0x50 >> 2);
	uint32_t * const ds_engine = bar0 + BASE_DMA_DOWN;
	const unsigned int bar_no = 2;
	time_t time_start;
	time_t time_end;
	double time_diff;
	size_t bytes_sent;

	uint32_t *ptr = static_cast<uint32_t *>(km->getBuffer());
	uint64_t ha = km->getPhysicalAddress();
	uint64_t pa = 0x0;

	// Setup a DMA transfer
	dma.pa_h = (pa >> 32);
	dma.pa_l = pa;
	dma.ha_h = (ha >> 32);
	dma.ha_l = ha;
	dma.length = buf_size;
	dma.control = 0x03008000 | (bar_no << 16);
	dma.next_bda_h = 0x0;
	dma.next_bda_l = 0x0;

	std::cout << "[Write test]" << std::endl;
	time(&time_start);
	for(bytes_sent = 0; bytes_sent < test_len; bytes_sent += buf_size) {
		dma.reset(ds_engine);
		dma.write(ds_engine);

		dma.wait_finish(ds_engine);
	}
	time(&time_end);

	time_diff = difftime(time_end, time_start);;
	std::cout << "Write time: " << std::fixed << std::setprecision(2) <<
		time_diff << " seconds" << std::endl;
	std::cout << "Write speed: " << std::fixed << std::setprecision(2) <<
		(bytes_sent/time_diff)/pow(2,20) << " [MB/s]" << std::endl;

	std::cout << "[Read test]" << std::endl;
	time(&time_start);
	for(bytes_sent = 0; bytes_sent < test_len; bytes_sent += buf_size) {
		dma.reset(us_engine);
		dma.write(us_engine);

		dma.wait_finish(us_engine);
	}
	time(&time_end);

	time_diff = difftime(time_end, time_start);;
	std::cout << "Read time: " << std::fixed << std::setprecision(2) <<
		time_diff << " seconds" << std::endl;
	std::cout << "Read speed: " << std::fixed << std::setprecision(2) <<
		(bytes_sent/time_diff)/pow(2,20) << " [MB/s]\n" << std::endl;
}

