#!/bin/sh
#
# Builds a minimal base system for booting from a floppy disk
# The first argument must be the directory containing the distribution files (E..g root/)

if [ "$1" = "" ]; then
	echo "You must supply the directory containing the distribution files.  Stopping"
	exit 1
fi;

if [ ! -e "$1" ]; then
	echo "$1 does not exist.  Stopping"
	exit 1
fi;

BASE=$1
ROOT=miniroot

if [ -e "$ROOT" ]; then
	rm -rf $ROOT
fi;

echo "Copying files from $BASE"

mkdir $ROOT

# Disk 1

mkdir $ROOT/disk1
mkdir $ROOT/disk1/atheos
mkdir $ROOT/disk1/atheos/sys
mkdir $ROOT/disk1/atheos/sys/bin
mkdir $ROOT/disk1/atheos/sys/config
mkdir $ROOT/disk1/atheos/sys/fonts
mkdir $ROOT/disk1/atheos/sys/keymaps
mkdir $ROOT/disk1/atheos/etc
mkdir $ROOT/disk1/atheos/home
mkdir $ROOT/disk1/atheos/home/root

DISK=$ROOT/disk1

cp -p $BASE/atheos/sys/bin/DiskManager $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/aedit $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/aterm $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/dbterm $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/devstat $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/format $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/fsprobe $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/init $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/mount $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/reboot $DISK/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/unmount $DISK/atheos/sys/bin/

cp -p $BASE/atheos/sys/appserver $DISK/atheos/sys/
strip --strip-all $DISK/atheos/sys/appserver

cd $DISK/atheos/sys/bin
ln -s /usr/bin/bash bash
ln -s /usr/bin/bash sh
ln -s /usr/bin/pwd pwd
cd ../../../../../

cp -p $BASE/atheos/sys/config/appserver $DISK/atheos/sys/config/

cp -p $BASE/atheos/sys/fonts/NimbusSanL-Regu.ttf $DISK/atheos/sys/fonts/
cp -p $BASE/atheos/sys/fonts/SyllableConsole-Regu.ttf $DISK/atheos/sys/fonts/

cp -p $BASE/atheos/sys/keymaps/American $DISK/atheos/sys/keymaps/

cp -p $BASE/atheos/etc/profile $DISK/atheos/etc/
cp -p $BASE/atheos/etc/passwd $DISK/atheos/etc/

cp -p $BASE/atheos/home/root/.profile $DISK/atheos/home/root/

# Disk 2

mkdir $ROOT/disk2
mkdir $ROOT/disk2/atheos
mkdir $ROOT/disk2/atheos/sys
mkdir $ROOT/disk2/atheos/sys/libs

DISK=$ROOT/disk2

cp -dpr $BASE/atheos/sys/libs/libc.so.1 $DISK/atheos/sys/libs/
cp -dpr $BASE/atheos/sys/libs/libm-2.1.2.so $DISK/atheos/sys/libs/
cp -dpr $BASE/atheos/sys/libs/libc-2.1.2.so $DISK/atheos/sys/libs/
cp -dpr $BASE/atheos/sys/libs/libgcc.so.1 $DISK/atheos/sys/libs/
cp -dpr $BASE/atheos/sys/libs/libstdc++-2.so.3 $DISK/atheos/sys/libs/
#cp -dpr $BASE/atheos/sys/libs/libnss_compat-2.1.2.so $DISK/atheos/sys/libs/

cp -dpr $BASE/atheos/sys/libs/libatheos.so.3 $DISK/atheos/sys/libs/
strip --strip-all $DISK/atheos/sys/libs/libatheos.so.3

# Disk 3

mkdir $ROOT/disk3
mkdir $ROOT/disk3/atheos
mkdir $ROOT/disk3/atheos/sys
#mkdir $ROOT/disk3/atheos/sys/libs
mkdir $ROOT/disk3/atheos/sys/drivers
mkdir $ROOT/disk3/atheos/sys/drivers/appserver
mkdir $ROOT/disk3/atheos/sys/drivers/appserver/input
mkdir $ROOT/disk3/atheos/sys/drivers/appserver/video
mkdir $ROOT/disk3/atheos/sys/drivers/dev
mkdir $ROOT/disk3/atheos/sys/drivers/dev/disk
mkdir $ROOT/disk3/atheos/sys/drivers/dev/misc
mkdir $ROOT/disk3/atheos/sys/drivers/fs
mkdir $ROOT/disk3/atheos/usr
mkdir $ROOT/disk3/atheos/usr/bin

DISK=$ROOT/disk3

cp -p $BASE/atheos/sys/drivers/appserver/input/* $DISK/atheos/sys/drivers/appserver/input/

cp -p $BASE/atheos/sys/drivers/appserver/video/* $DISK/atheos/sys/drivers/appserver/video/

cp -p $BASE/atheos/sys/drivers/dev/keybd $DISK/atheos/sys/drivers/dev/

cp -p $BASE/atheos/sys/drivers/dev/disk/bios $DISK/atheos/sys/drivers/dev/disk/

cp -p $BASE/atheos/sys/drivers/dev/misc/ps2aux $DISK/atheos/sys/drivers/dev/misc/

cp -p $BASE/atheos/sys/drivers/fs/afs $DISK/atheos/sys/drivers/fs/
cp -p $BASE/atheos/sys/drivers/fs/fatfs $DISK/atheos/sys/drivers/fs/

cp -p $BASE/atheos/usr/bin/bash $DISKatheos/usr/bin/
cp -p $BASE/atheos/usr/bin/cp $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/gzip $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/hostname $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/less $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/ln $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/ls $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mkdir $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mv $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/pwd $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/rm $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/rmdir $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/sync $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/tar $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/whoami $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/uname $DISK/atheos/usr/bin/

cd $DISK/atheos/usr/bin
ln -s gzip gunzip
cd ../../../../..

echo "Copying init files"

cp scripts/sys/floppy-init.sh $DISK/atheos/sys/init.sh

echo "Building ramdisk image 1"
bin/makeramdisk $ROOT/disk1 bimage1
gzip -9 bimage1
rm -rf $ROOT/disk1
mv bimage1.gz objs/

echo "Building ramdisk image 2"
bin/makeramdisk $ROOT/disk2 bimage2
gzip -9 bimage2
rm -rf $ROOT/disk2
mv bimage2.gz objs/

echo "Building ramdisk image 3"
bin/makeramdisk $ROOT/disk3 bimage3
gzip -9 bimage3
rm -rf $ROOT/disk3
mv bimage3.gz objs/

echo "Building floppies"
./mkfloppies.sh

echo "Done"
exit 0

