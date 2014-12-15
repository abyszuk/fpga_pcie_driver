# Set your cross compile prefix with CROSS_COMPILE variable
CROSS_COMPILE ?=

CMDSEP = ;

CC =            $(CROSS_COMPILE)gcc
AR =            $(CROSS_COMPILE)ar
LD =            $(CROSS_COMPILE)ld
OBJDUMP =       $(CROSS_COMPILE)objdump
OBJCOPY =       $(CROSS_COMPILE)objcopy
SIZE =          $(CROSS_COMPILE)size
MAKE =          make

DRIVER_DIR = drivers/pcie
LIB_DIR = lib/pcie

all: kernel_driver lib_driver

.PHONY: kernel_driver lib_driver install uninstall clean

kernel_driver:
	$(MAKE) -C $(DRIVER_DIR) all
#	$(MAKE) -C $(DRIVER_DIR) install

lib_driver:
	$(MAKE) -C $(LIB_DIR) all
#	$(MAKE) -C $(LIB_DIR) install

clean:
	$(MAKE) -C $(DRIVER_DIR) clean
	$(MAKE) -C $(LIB_DIR) clean

install:
	$(MAKE) -C $(DRIVER_DIR) install
	$(MAKE) -C $(LIB_DIR) install

uninstall:
	$(MAKE) -C $(DRIVER_DIR) uninstall
	$(MAKE) -C $(LIB_DIR) uninstall
