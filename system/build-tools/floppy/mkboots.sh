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

cp ../$BASE/atheos/system/kernel.so .
gzip kernel.so

cp kernel.so.gz disk/atheos/system/
cp ../$BASE/atheos/system/drivers/fs/ramfs disk/atheos/system/drivers/fs/
cp ../$BASE/atheos/system/drivers/dev/disk/bios disk/atheos/system/drivers/dev/disk/

unmount disk
mv boot-floppy.img ../objs/syllable1.img

echo "Building CD boot image"

cp ../images/boot-cd.img .
mount -t fatfs boot-cd.img disk

mv kernel.so.gz disk/atheos/system/
cp ../$BASE/atheos/system/drivers/fs/iso9660 disk/atheos/system/drivers/fs/
cp ../$BASE/atheos/system/drivers/dev/disk/ata disk/atheos/system/drivers/dev/disk/
cp ../$BASE/atheos/system/drivers/bus/pci disk/atheos/system/drivers/bus/
touch disk/atheos/system/config/kernel.cfg

unmount disk
mv boot-cd.img ../objs/boot.img

echo "Cleaning up"
rmdir disk
cd ..
rmdir temp

echo "Done"

exit 0

