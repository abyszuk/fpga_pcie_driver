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

kernel_driver_clean:
	$(MAKE) -C $(DRIVER_DIR) clean

kernel_driver_install:
	$(MAKE) -C $(DRIVER_DIR) install

kernel_driver_uninstall:
	$(MAKE) -C $(DRIVER_DIR) uninstall

lib_driver:
	$(MAKE) -C $(LIB_DIR) all

lib_driver_clean:
	$(MAKE) -C $(LIB_DIR) clean

lib_driver_install:
	$(MAKE) -C $(LIB_DIR) install

lib_driver_uninstall:
	$(MAKE) -C $(LIB_DIR) uninstall

clean: kernel_driver_clean lib_driver_clean

install: kernel_driver_install lib_driver_install

uninstall: kernel_driver_uninstall lib_driver_uninstall
