# Makefile for the pciDriver library

CC = gcc
CXX = g++

CFLAGS += -O2 -g -fPIC
CXXFLAGS += -O2 -g -fPIC

# Source files in this directory
SRC = $(wildcard *.cpp)
SRCC = $(wildcard *.c)

# Corresponding object file
OBJ = $(SRC:.cpp=.o)
OBJ += $(SRCC:.c=.o)

LIB ?= libpcidriver.so
LIBSTATIC ?= libpcidriver.a

INCDIR =../../include/pcie
INCDIR +=../../include/pcie/lib
INC = $(INCDIR)/%.h

LDINC += -L $(realpath .)
CFLAGS += $(addprefix -I, $(INCDIR))
CXXFLAGS += $(addprefix -I,i $(INCDIR))

###############################################################
# Target definitions

.PHONY: all clean

all: $(LIB) $(LIBSTATIC)

# Target for dynamic the library
$(LIB): $(OBJ)
	@echo "## Creating dynamic library ##"
	@echo -e "LD [.so] \t$@"
	$(Q)$(CXX) -shared $(LDINC) $(LDFLAGS) $(CXXFLAGS) -o $@ $^

# Target for static the library
$(LIBSTATIC): $(OBJ)
	@echo "## Creating static library ##"
	@echo -e "LD [.a] \t$@"
	$(Q)ar -r $(LIBSTATIC) $^
	$(Q)ar -s $(LIBSTATIC)

%.o: %.cpp $(INC)
	@echo -e "CC \t$<"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c $(INC)
	@echo -e "CC \t$<"
	$(CC) $(CFLAGS) -c -o $@ $<

install:
	@echo "INSTALL libpcidriver.so"
	-$(Q)install -m 755 $(LIB) /usr/lib/
	@echo "INSTALL libpcidriver.a"
	-$(Q)install -m 755 $(LIBSTATIC) /usr/lib/
	@echo "INSTALL /usr/include/pciDriver/lib"
	-$(Q)mkdir -p /usr/include/pciDriver/lib
	-$(Q)install -D -m 644 ../../include/pcie/lib/*.h /usr/include/pciDriver/lib

uninstall:
	@echo "UNINSTALL libpcidriver.so"
	-$(Q)rm -f /usr/lib/libpcidriver.so 
	@echo "UNINSTALL libpcidriver.a"
	-$(Q)rm -f /usr/lib/libpcidriver.a
	@echo "UNINSTALL /usr/include/pciDriver/lib"
	-$(Q)rm -rf /usr/include/pciDriver/lib
	-$(Q)rmdir /usr/include/pciDriver

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(LIB) $(LIBSTATIC)
	-$(Q)rm -f $(OBJ)
