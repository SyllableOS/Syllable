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

cp ../$ROOT/atheos/sys/kernel.so .
gzip kernel.so
mv kernel.so.gz disk/atheos/sys/

cp ../$ROOT/atheos/sys/drivers/fs/iso9660 disk/atheos/sys/drivers/fs/
cp ../$ROOT/atheos/sys/drivers/bus/pci disk/atheos/sys/drivers/bus/
cp ../$ROOT/atheos/sys/drivers/bus/ata disk/atheos/sys/drivers/bus/
cp ../$ROOT/atheos/sys/drivers/dev/hcd/ata_pci disk/atheos/sys/drivers/dev/hcd/
touch disk/atheos/sys/config/kernel.cfg

unmount disk
mv boot-cd.img ../objs/boot.img

echo "Cleaning up"
rmdir disk
cd ..
rmdir temp

echo "Done"

exit 0

