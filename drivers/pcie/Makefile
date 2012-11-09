
# Main Makefile for the pciDriver

#helpful in case of buildroot crosscompiling
ROOTDIR := 
CURDIR := $(shell pwd)

export SRCDIR := $(CURDIR)/src

default:
	$(Q)$(MAKE) -C $(SRCDIR)

install:
	$(Q)$(MAKE) -C $(SRCDIR) install
	@echo "INSTALL 60-udev_fpga.rules"
	-$(Q)install -m 644 etc/udev/rules.d/60-udev_fpga.rules $(ROOTDIR)/etc/udev/rules.d/

uninstall: 
	$(Q)$(MAKE) -C $(SRCDIR) uninstall
	@echo "UNINSTALL 60-udev_fpga.rules"
	-$(Q)rm -f $(ROOTDIR)/etc/udev/rules.d/60-udev_fpga.rules

clean:
	@echo "Cleaning..."
	-$(Q)$(MAKE) -C $(SRCDIR) clean
