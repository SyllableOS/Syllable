#!/bin/sh
#
# Creates bootable disk images for both the floppy and CD installations
# First argument must be the directory containing the distributiong files (Usually root/ if we're called from mkdist.sh)

if [ "$1" = "" ]; then
	echo "You must supply the path to the distribution files (E.g. root/)"
	exit 1
fi;

if [ ! -e $1 ]; then
	echo "$1 does not exist.  Stopping"
	exit 1
fi;

BASE=$1

echo "Building floppy boot image"

if [ ! -e temp ]; then
	mkdir temp
fi;

cd temp
if [ ! -e disk ]; then
	mkdir disk
fi;

cp ../images/boot-floppy.img .
mount -t fatfs boot-floppy.img disk

cp ../$BASE/atheos/sys/kernel.so .
gzip kernel.so

cp kernel.so.gz disk/atheos/sys/
cp ../$BASE/atheos/sys/drivers/fs/ramfs disk/atheos/sys/drivers/fs/
cp ../$BASE/atheos/sys/drivers/dev/disk/bios disk/atheos/sys/drivers/dev/disk/

unmount disk
mv boot-floppy.img ../objs/syllable1.img

echo "Building CD boot image"

cp ../images/boot-cd.img .
mount -t fatfs boot-cd.img disk

mv kernel.so.gz disk/atheos/sys/
cp ../$BASE/atheos/sys/drivers/fs/iso9660 disk/atheos/sys/drivers/fs/
cp ../$BASE/atheos/sys/drivers/dev/disk/ata disk/atheos/sys/drivers/dev/disk/
cp ../$BASE/atheos/sys/drivers/bus/pci disk/atheos/sys/drivers/bus/
touch disk/atheos/sys/config/kernel.cfg

unmount disk
mv boot-cd.img ../objs/boot.img

echo "Cleaning up"
rmdir disk
cd ..
rmdir temp

echo "Done"

exit 0

