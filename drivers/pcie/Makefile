
# Main Makefile for the pciDriver

#helpful in case of buildroot crosscompiling
CURDIR := $(shell pwd)
export SRCDIR := $(CURDIR)/src

PREFIX ?=

all:
	$(Q)$(MAKE) -C $(SRCDIR)

install: core_install etc_install

core_install:
	$(Q)$(MAKE) -C $(SRCDIR) install

etc_install:
	-$(Q)mkdir -p $(PREFIX)/etc/udev/rules.d/
	@echo "INSTALL 60-udev_fpga.rules"
	-$(Q)install -m 644 etc/udev/rules.d/60-udev_fpga.rules $(PREFIX)/etc/udev/rules.d/

uninstall: core_uninstall etc_uninstall

core_uninstall:
	$(Q)$(MAKE) -C $(SRCDIR) uninstall

etc_uninstall:
	@echo "UNINSTALL 60-udev_fpga.rules"
	-$(Q)rm -f $(PREFIX)/etc/udev/rules.d/60-udev_fpga.rules
	-$(Q)rmdir $(PREFIX)/etc/udev/rules.d/

clean:
	@echo "Cleaning..."
	-$(Q)$(MAKE) -C $(SRCDIR) clean
