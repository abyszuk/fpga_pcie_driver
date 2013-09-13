###############################################################
#
# Change History:
#
# Revision 2.0 2013/03/28 abyszuk
# Moved to OHWR repo
#
# Revision 1.4  2006/11/17 18:56:44  marcus
# Added testCompatInterface.
#
# Revision 1.3  2006/10/30 19:40:32  marcus
# Added testPCIeBoard.
#
# Revision 1.2  2006/10/16 16:57:17  marcus
# Added testCinterface.
#
# Revision 1.1  2006/10/13 17:18:35  marcus
# Implemented and tested most of C++ interface.
#
###############################################################

# Makefile for the pciDriver test programs
CC = gcc
CXX = g++

CFLAGS += -O2 -g
CXXFLAGS += -O2 -g

INCDIR = ../../include/pcie
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += -lpcidriver -lpthread

BINDIR = bin

CFLAGS += $(addprefix -I, $(INCDIR))
CXXFLAGS += $(addprefix -I,i $(INCDIR))

BINARIES = testCppInterface \
	testDMA \
	testPciDriver \
	testCinterface \
	benchmarkDevice

###############################################################
# Target definitions

.PHONY: all dirs clean

all: dirs $(BINARIES)

dirs:
	$(Q)mkdir -p $(BINDIR)

# Relate all exec names to it exec in the bin dir
$(BINARIES) : % : $(BINDIR)/% ;

# Target for each exec from the object file
$(BINDIR)/%: %.cpp
	@echo -e "LD \t$@"
	$(Q)$(CXX) $(LDINC) $(LDFLAGS) $(CXXFLAGS) -o $@ $<

$(BINDIR)/testCinterface: testCinterface.c
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $<

$(BINDIR)/testPciDriver: testPciDriver.c
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $<

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(BINDIR)/testCinterface
	-$(Q)rm -f $(BINDIR)/testPciDriver
