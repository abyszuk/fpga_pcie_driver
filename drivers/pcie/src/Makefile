
obj-m := pciDriver.o
pciDriver-objs := base.o int.o umem.o kmem.o sysfs.o ioctl.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
INSTALLDIR ?= /lib/modules/$(shell uname -r)/extra
INSTALLHDRDIR ?= /usr/include/pciDriver/driver
PWD := $(shell pwd)

default: dolink
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

dolink:
	@ln -sf ../../../include/pcie/driver/pciDriver.h

install:
	@mkdir -p $(INSTALLDIR)
	@echo "INSTALL $(INSTALLDIR)/pciDriver.ko"
	@install -m 755 pciDriver.ko $(INSTALLDIR)
	@echo "INSTALL $(INSTALLHDRDIR)/pciDriver.h"
	@mkdir -p $(INSTALLHDRDIR)
	@install -m 644 pciDriver.h $(INSTALLHDRDIR)

uninstall:
	@echo "UNINSTALL $(INSTALLDIR)/pciDriver.ko"
	@rm -f $(INSTALLDIR)/pciDriver.ko
	@echo "UNINSTALL $(INSTALLHDRDIR)/pciDriver.h"
	@rm -rf /usr/include/pciDriver/driver

clean:
	rm -f *.o *.ko *.mod.c .*.cmd Module.symvers modules.order
	rm -rf .tmp_versions
