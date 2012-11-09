#ifndef PCIDRIVER_COMPAT_H_
#define PCIDRIVER_COMPAT_H_

/***********************************************************************************
 *  C++ API compatible with older pciDriver interfaces
 *  Do not use for new designs!
 *
 * $Revision: 1.3 $
 * $Date: 2006-11-17 18:53:23 $
 * 
 ***********************************************************************************/

/***********************************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2006/11/13 12:29:49  marcus
 * Backup commit. Compatibility section still requires testing.
 *
 * Revision 1.1  2006/10/10 14:46:49  marcus
 * Initial commit of the new pciDriver for kernel 2.6
 *
 ***********************************************************************************/

// Some needed forward references
namespace pciDriver {
	class KernelMemory;
	class UserMemory;
	class PciDevice;
}

#ifndef KMEM_H
#define KMEM_H

/** This class is the main interface to allocate kernel memory
 *  with the driver. It performes ioctl calls to allocate and
 *  free the memory and mapps it into the user space.
 */ 

class KMem {
 public:
  /** The standard constructor.
   *  Initialises data.
   */
  KMem();

  /** The standard destructor.
   *  Ensure data is properly released.
   */
  ~KMem();

  /** Constructor which initialises with allocated kernel memory.
   *    @param handle  The device driver handle
   *    @param order   The memory size in powers of 2
   */
  KMem(int handle, int order);

  /** Allocate kernel memory.
   *    @param  handle  The device driver handle
   *    @param  order   The memory size in powers of 2
   *    @return Integer 1 on success, 0 else.
   */
  int Alloc(int handle, int order);

  /** Free the kernel memory
   *    @return Integer 1 on success, 0 else.
   */
  int Free(void);
  
  /** Get the physical address of the allocated buffer
   *    @return The physical address of the buffer
   */
  unsigned long GetPhysicalAddress(void);

  /** Get the buffer address
   *    @return The virtual address of the buffer
   */
  unsigned int *GetBuffer(void);

  void Sync(void);

 protected:

  pciDriver::KernelMemory *km;
};

#endif	/* KMEM_H */

#ifndef MEMORYPAGELIST_H
#define MEMORYPAGELIST_H

class MemoryPageList {

 public:

  /// The standard constructor
  MemoryPageList();

  /// The standard destructor
  ~MemoryPageList();

  /** A constructor which builds up the pagelist for a 
   *  given buffer.
   *      @param handle The device driver handle
   *      @param buffer Pointer which contains the virtual address of the buffer
   *      @param size   The buffer size in byte
   */
  MemoryPageList(int handle, unsigned int *buffer, unsigned int size);

  /** Lock AND build up the list of buffer pages. 
   *      @param handle The device driver handle
   *      @param buffer Pointer which contains the virtual address of the buffer
   *      @param size   The buffer size in byte
   *      @return True on success, else false
   */
  bool LockBuffer(int handle, unsigned int *buffer, unsigned int size);

  /** Unlock the buffer
   *      @return True on success, else false
   */
  bool UnlockBuffer(void);
  
  /** Check if this page list is accociated to a buffer.
   *      @return True if this object contains a page list, else false
   */
  bool IsUsed(void);

  /** Get the size of the pagelist. 
   *      @return The number of pages, 0 if no pagelist exists
   */
  unsigned int GetNumberOfPages(void);

  /** returns the physical addresses of the pages.
          @param index The number of the buffer page
	  @return The physical address or 0 if the pagelist is not in use
  */
  unsigned int GetPhysicalAddress(unsigned int index);

  /// Access to the physical addresses of the pages.
  unsigned int operator[] (unsigned int index);

  /// Get the offset in the first page
  unsigned int GetFirstPageOffset(void);

  void Sync(void);

 protected:

  pciDriver::UserMemory *um;
  int pagesize;
  int pageshift;
  
};

#endif /* MEMORYPAGELIST_H */


#ifndef PCIDEVICE_H
#define PCIDEVICE_H

// --------x----------------------------x-----------------------------------x--

/** Class PciDevice
 *  This class represents a PCI device. It can open and close the device 
 *  driver and get the user memory mapped PCI areas.
 */

class PciDevice {

 public:

  /** Standard constructor
      Initialises data.*/
  PciDevice();

  /** Standard destructor
      Initialises data.*/
  ~PciDevice();
  
  /** Open the driver to accesss a device. Initialise data.
   *  @param deviceNr  The logical number of the device to open.
   *  @return Integer 1 on success.
   */
  int Open(unsigned int deviceNr);

  /** Close the device driver. */
  int Close(void);

  /** Get access to a PCI memory bar which is mapped in user memory.
      @return The virtual pointer to the PCI memory bar.
   */
  volatile unsigned int *GetBarAccess(unsigned int barNr);

  /** Checks if the driver is open.
   *  @return True if the driver is open else false.
   */
  bool IsOpen(void); 

  unsigned char  ReadConfigByte(unsigned int address);
  unsigned short ReadConfigWord(unsigned int address);
  unsigned int   ReadConfigDWord(unsigned int address);

  void WriteConfigByte(unsigned int address, unsigned char val);
  void WriteConfigWord(unsigned int address, unsigned short val);
  void WriteConfigDWord(unsigned int address, unsigned int val);

  unsigned int GetBus(void);
  unsigned int GetSlot(void);

  unsigned short GetVendorId(void);
  unsigned short GetDeviceId(void);

  /** Performs a cast operation to an int to get easy access to the
   * device handle.
   */
  operator int();

  static int GetNumberOfDevices(void);

 protected:

  pciDriver::PciDevice *dev;
  int dev_number;
  void *bar[6];

};
#endif /* PCIDEVICE_H */

#endif /*PCIDRIVER_COMPAT_H_*/
