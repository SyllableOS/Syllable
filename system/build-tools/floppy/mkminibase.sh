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
mkdir $ROOT/disk1/atheos/system
mkdir $ROOT/disk1/atheos/system/bin
mkdir $ROOT/disk1/atheos/system/fonts

DISK=$ROOT/disk1

cp -p $BASE/atheos/system/bin/aedit $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/aterm $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/devstat $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/format $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/fsprobe $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/init $DISK/atheos/system/bin/
cp -p $BASE/atheos/system/bin/mount $DISK/atheos/system/bin/

cd $DISK/atheos/system/bin
ln -s /usr/bin/bash bash
ln -s /usr/bin/bash sh
ln -s /usr/bin/pwd pwd
cd ../../../../../

cp -p $BASE/atheos/system/appserver $DISK/atheos/system/
strip --strip-all $DISK/atheos/system/appserver

cp -p $BASE/atheos/system/fonts/NimbusSanL-Regu.ttf $DISK/atheos/system/fonts/
cp -p $BASE/atheos/system/fonts/SyllableConsole-Regu.ttf $DISK/atheos/system/fonts/

# Disk 2

mkdir $ROOT/disk2
mkdir $ROOT/disk2/atheos
mkdir $ROOT/disk2/atheos/system
mkdir $ROOT/disk2/atheos/system/bin
mkdir $ROOT/disk2/atheos/system/libs
mkdir $ROOT/disk2/atheos/system/keymaps
mkdir $ROOT/disk2/atheos/system/config
mkdir $ROOT/disk2/atheos/system/drivers
mkdir $ROOT/disk2/atheos/system/drivers/appserver
mkdir $ROOT/disk2/atheos/system/drivers/appserver/video
mkdir $ROOT/disk2/atheos/system/drivers/dev
mkdir $ROOT/disk2/atheos/system/drivers/dev/disk
mkdir $ROOT/disk2/atheos/system/drivers/dev/misc
mkdir $ROOT/disk2/atheos/etc
mkdir $ROOT/disk2/atheos/home
mkdir $ROOT/disk2/atheos/home/root

DISK=$ROOT/disk2

cp -p $BASE/atheos/system/bin/DiskManager $DISK/atheos/system/bin/
strip --strip-all $DISK/atheos/system/bin/DiskManager

cp -dpr $BASE/atheos/system/libs/libc.so.1 $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libc-2.1.2.so $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libm.so.1 $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libm-2.1.2.so $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libgcc.so.1 $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libstdc++-2.so.3 $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/system/libs/libstdc++-3-2-2.10.0.so $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/usr/zlib/lib/libz.so $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/usr/zlib/lib/libz.so.1 $DISK/atheos/system/libs/
cp -dpr $BASE/atheos/usr/zlib/lib/libz.so.1.1.4 $DISK/atheos/system/libs/

for l in $(ls $DISK/atheos/system/libs/);do
	strip --strip-all $DISK/atheos/system/libs/$l;
done;

cp -p $BASE/atheos/system/keymaps/American $DISK/atheos/system/keymaps/

cp -p $BASE/atheos/system/config/appserver $DISK/atheos/system/config/

cp -p $BASE/atheos/system/drivers/appserver/video/sis3xx $DISK/atheos/system/drivers/appserver/video/
cp -p $BASE/atheos/system/drivers/appserver/video/trident $DISK/atheos/system/drivers/appserver/video/

cp -p $BASE/atheos/system/drivers/dev/keybd $DISK/atheos/system/drivers/dev/

cp -p $BASE/atheos/system/drivers/dev/disk/bios $DISK/atheos/system/drivers/dev/disk/

cp -p $BASE/atheos/system/drivers/dev/misc/ps2aux $DISK/atheos/system/drivers/dev/misc/

cp -p $BASE/atheos/etc/profile $DISK/atheos/etc/
cp -p $BASE/atheos/etc/passwd $DISK/atheos/etc/

cp -p $BASE/atheos/home/root/.profile $DISK/atheos/home/root/

# Disk 3

mkdir $ROOT/disk3
mkdir $ROOT/disk3/atheos
mkdir $ROOT/disk3/atheos/system
mkdir $ROOT/disk3/atheos/system/libs
mkdir $ROOT/disk3/atheos/system/drivers
mkdir $ROOT/disk3/atheos/system/drivers/appserver
mkdir $ROOT/disk3/atheos/system/drivers/appserver/input
mkdir $ROOT/disk3/atheos/system/drivers/appserver/video
mkdir $ROOT/disk3/atheos/system/drivers/fs
mkdir $ROOT/disk3/atheos/usr
mkdir $ROOT/disk3/atheos/usr/bin

DISK=$ROOT/disk3

cp -dpr $BASE/atheos/system/libs/libatheos.so.3 $DISK/atheos/system/libs/
strip --strip-all $DISK/atheos/system/libs/libatheos.so.3

cp -p $BASE/atheos/system/drivers/appserver/input/* $DISK/atheos/system/drivers/appserver/input/

cp -p $BASE/atheos/system/drivers/appserver/video/mach64 $DISK/atheos/system/drivers/appserver/video/
cp -p $BASE/atheos/system/drivers/appserver/video/nvidia $DISK/atheos/system/drivers/appserver/video/
cp -p $BASE/atheos/system/drivers/appserver/video/s3_virge $DISK/atheos/system/drivers/appserver/video/

cp -p $BASE/atheos/system/drivers/fs/afs $DISK/atheos/system/drivers/fs/
cp -p $BASE/atheos/system/drivers/fs/fatfs $DISK/atheos/system/drivers/fs/

cp -p $BASE/atheos/usr/bin/bash $DISKatheos/usr/bin/
cp -p $BASE/atheos/usr/bin/cp $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/gzip $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/hostname $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/ls $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mkdir $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mv $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/pwd $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/rm $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/sync $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/tar $DISK/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/whoami $DISK/atheos/usr/bin/

cd $DISK/atheos/usr/bin
ln -s gzip gunzip
cd ../../../../..

echo "Copying init files"

cp scripts/system/floppy-init.sh $DISK/atheos/system/init.sh

echo "Building ramdisk image 1"
bin/makeramdisk $ROOT/disk1 bimage1
gzip -9 bimage1
rm -rf $ROOT/disk1
mv bimage1.gz objs/bimage1.bin.gz

echo "Building ramdisk image 2"
bin/makeramdisk $ROOT/disk2 bimage2
gzip -9 bimage2
rm -rf $ROOT/disk2
mv bimage2.gz objs/bimage2.bin.gz

echo "Building ramdisk image 3"
bin/makeramdisk $ROOT/disk3 bimage3
gzip -9 bimage3
rm -rf $ROOT/disk3
mv bimage3.gz objs/bimage3.bin.gz

echo "Building floppies"
./mkfloppies.sh

echo "Cleaning up"
rm objs/bimage1.bin.gz
rm objs/bimage2.bin.gz
rm objs/bimage3.bin.gz

rm -rf $ROOT

echo "Done"
exit 0

