#!/bin/sh
#
# Creates bootable disk images for the CD
# First argument must be the directory containing the distributiong files
# (Which is root/ if we're called from mkdist.sh)

if [ "$1" = "" ]; then
	echo "You must supply the path to the distribution files (E.g. root/)"
	exit 1
fi;

if [ ! -e $1 ]; then
	echo "$1 does not exist.  Stopping"
	exit 1
fi;

ROOT=$1

echo "Building CD boot image"

if [ ! -e temp ]; then
	mkdir temp
fi;

cd temp
if [ ! -e disk ]; then
	mkdir disk
fi;

cp ../../images/boot-cd.img .
mount -t fatfs boot-cd.img disk

cp ../$ROOT/atheos/system/kernel.so .
gzip kernel.so
mv kernel.so.gz disk/atheos/system/

cp ../$ROOT/atheos/system/drivers/fs/iso9660 disk/atheos/system/drivers/fs/
cp ../$ROOT/atheos/system/drivers/bus/pci disk/atheos/system/drivers/bus/
cp ../$ROOT/atheos/system/drivers/bus/ata disk/atheos/system/drivers/bus/
cp ../$ROOT/atheos/system/drivers/dev/hcd/ata_pci disk/atheos/system/drivers/dev/hcd/
touch disk/atheos/system/config/kernel.cfg

unmount disk
mv boot-cd.img ../objs/boot.img

echo "Cleaning up"
rmdir disk
cd ..
rmdir temp

echo "Done"

exit 0

