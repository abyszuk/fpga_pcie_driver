#!/bin/bash

#add relevant overrides to make use of buildroot toolchain
BUILDROOT_DIR="/home/adrian/praca/buildroot/buildroot"

KERNELDIR="$BUILDROOT_DIR/output/build/linux-3.6.8"
INSTALLDIR="$BUILDROOT_DIR/output/target"

export ARCH=x86_64
export CROSS_COMPILE=x86_64-linux-
export PATH="$PATH:$BUILDROOT_DIR/output/host/usr/bin"

EXTRA_CFLAGS="-D VERBOSE"

cd src && make KERNELDIR=$KERNELDIR

cd ..
echo "Copying to $INSTALLDIR"
cp src/pciDriver.ko $INSTALLDIR/opt/
cp etc/udev/rules.d/60-udev_fpga.rules $INSTALLDIR/etc/udev/rules.d/
echo "DONE"
