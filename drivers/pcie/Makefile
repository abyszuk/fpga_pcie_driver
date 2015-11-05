
# Main Makefile for the pciDriver

#helpful in case of buildroot crosscompiling
CURDIR := $(shell pwd)
export SRCDIR := $(CURDIR)/src

PREFIX ?=

all:
	$(Q)$(MAKE) -C $(SRCDIR)

install:
	$(Q)$(MAKE) -C $(SRCDIR) install
	-$(Q)mkdir -p $(PREFIX)/etc/udev/rules.d/
	@echo "INSTALL 60-udev_fpga.rules"
	-$(Q)install -m 644 etc/udev/rules.d/60-udev_fpga.rules $(PREFIX)/etc/udev/rules.d/
	@echo "INSTALL eat-dashes.sh"
	-$(Q)install -m 755 usr/local/bin/eat-dashes.sh $(PREFIX)/usr/local/bin/

uninstall:
	$(Q)$(MAKE) -C $(SRCDIR) uninstall
	@echo "UNINSTALL eat-dashes.sh"
	-$(Q)rm -f $(PREFIX)/bin/eat-dashes.sh
	@echo "UNINSTALL 60-udev_fpga.rules"
	-$(Q)rm -f $(PREFIX)/etc/udev/rules.d/60-udev_fpga.rules
	-$(Q)rmdir $(PREFIX)/etc/udev/rules.d/

clean:
	@echo "Cleaning..."
	-$(Q)$(MAKE) -C $(SRCDIR) clean
