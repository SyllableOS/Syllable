#!/bin/sh
#
# Builds a minimal base system for booting from a CD
# The first argument must be the directory containing the distribution files (E..g root/)
# The second argument must be the distribution tarball itself
# The third argument must be the version number (E.g. 0.4.4)

if [ "$1" = "" ]; then
	echo "You must supply the directory containing the distribution files.  Stopping"
	exit 1
fi;

if [ ! -e "$1" ]; then
	echo "$1 does not exist.  Stopping"
	exit 1
fi;

if [ "$2" = "" ]; then
	echo "You must supply the path to the distribution tarball.  Stopping"
	exit 1
fi;

if [ ! -e "$2" ]; then
	echo "$2 does not exist.  Stopping"
	exit 1
fi;

if [ "$3" = "" ]; then
	echo "You must supply the version number.  Stopping"
	exit 1
fi;

BASE=$1
TARBALL=$2
VER=$3

ROOT=root

if [ -e $ROOT ]; then
	rm -rf $ROOT
fi;

mkdir $ROOT

echo "Building base directory tree"

mkdir $ROOT/boot

mkdir $ROOT/atheos
mkdir $ROOT/atheos/etc

mkdir $ROOT/atheos/home
mkdir $ROOT/atheos/home/root

mkdir root/atheos/sys

mkdir $ROOT/atheos/sys/bin
mkdir $ROOT/atheos/sys/config
mkdir $ROOT/atheos/sys/fonts
mkdir $ROOT/atheos/sys/keymaps
mkdir $ROOT/atheos/sys/libs

mkdir $ROOT/atheos/sys/drivers

mkdir $ROOT/atheos/sys/drivers/appserver
mkdir $ROOT/atheos/sys/drivers/appserver/input
mkdir $ROOT/atheos/sys/drivers/appserver/video

mkdir $ROOT/atheos/sys/drivers/dev
mkdir $ROOT/atheos/sys/drivers/dev/disk
mkdir $ROOT/atheos/sys/drivers/dev/misc

mkdir $ROOT/atheos/sys/drivers/fs
mkdir $ROOT/atheos/sys/drivers/bus

mkdir $ROOT/atheos/usr
mkdir $ROOT/atheos/usr/bin
mkdir $ROOT/atheos/usr/share
mkdir $ROOT/atheos/usr/share/terminfo
mkdir $ROOT/atheos/usr/share/terminfo/x

mkdir $ROOT/Install/

mkdir $ROOT/Packages/
mkdir $ROOT/Packages/base
mkdir $ROOT/Packages/net

echo "Copying files from $BASE"

cp -p $BASE/atheos/etc/profile $ROOT/atheos/etc/
cp -p $BASE/atheos/etc/passwd $ROOT/atheos/etc/
cp -p $BASE/atheos/etc/termcap $ROOT/atheos/etc/

cp -p $BASE/atheos/home/root/.profile $ROOT/atheos/home/root/

cp -p $BASE/atheos/sys/drivers/appserver/input/* $ROOT/atheos/sys/drivers/appserver/input/
cp -p $BASE/atheos/sys/drivers/appserver/video/* $ROOT/atheos/sys/drivers/appserver/video/

cp -p $BASE/atheos/sys/drivers/dev/keybd $ROOT/atheos/sys/drivers/dev/

cp -p $BASE/atheos/sys/drivers/dev/disk/bios $ROOT/atheos/sys/drivers/dev/disk/

cp -p $BASE/atheos/sys/drivers/dev/misc/ps2aux $ROOT/atheos/sys/drivers/dev/misc/

cp -p $BASE/atheos/sys/drivers/fs/afs $ROOT/atheos/sys/drivers/fs/
cp -p $BASE/atheos/sys/drivers/fs/fatfs $ROOT/atheos/sys/drivers/fs/

cp -p $BASE/atheos/sys/drivers/bus/pci $ROOT/atheos/sys/drivers/bus/

cp -dpr $BASE/atheos/sys/libs/* $ROOT/atheos/sys/libs/
cp -dpr $BASE/atheos/usr/zlib/lib/* $ROOT/atheos/sys/libs/

cp -p $BASE/atheos/sys/fonts/NimbusSanL-Regu.ttf $ROOT/atheos/sys/fonts/
cp -p $BASE/atheos/sys/fonts/VeraMono.ttf $ROOT/atheos/sys/fonts/

cp -p $BASE/atheos/sys/keymaps/American $ROOT/atheos/sys/keymaps/

cp -p $BASE/atheos/sys/appserver $ROOT/atheos/sys/

cp -p $BASE/atheos/sys/bin/DiskManager $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/aedit $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/aterm $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/dbterm $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/devstat $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/format $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/fsprobe $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/init $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/mount $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/reboot $ROOT/atheos/sys/bin/
cp -p $BASE/atheos/sys/bin/unmount $ROOT/atheos/sys/bin/

cp -p $BASE/atheos/usr/bin/bash $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/cat $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/clear $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/cp $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/cut $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/dd $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/echo $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/find $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/grep $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/gzip $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/hostname $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/less $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/ln $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/ls $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/md5sum $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mkdir $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/mv $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/printf $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/pwd $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/rm $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/rmdir $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/sed $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/sleep $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/sync $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/tar $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/whoami $ROOT/atheos/usr/bin/
cp -p $BASE/atheos/usr/bin/uname $ROOT/atheos/usr/bin/

cp -p $BASE/atheos/usr/share/terminfo/x/xterm $ROOT/atheos/usr/share/terminfo/x/

echo "Creating links"

cd $ROOT/atheos/usr/bin
ln -s gzip gunzip
cd ../../../..

cd $ROOT/atheos/sys/bin
ln -s /usr/bin/bash bash
ln -s /usr/bin/bash sh
ln -s /usr/bin/pwd pwd
cd ../../../..

echo "Copying init files"

cp scripts/sys/basic-init.sh $ROOT/atheos/sys/init.sh
cp -p $BASE/atheos/sys/config/appserver $ROOT/atheos/sys/config/

echo "Copying boot image"
cp objs/boot.img $ROOT/boot/boot.img

echo "Copying $TARBALL base package"
cp $TARBALL $ROOT/Packages/base/

echo "Copying floppy images"
cp objs/syllable* $ROOT/Packages/base/

echo "Copying installation scripts"
cp -dpr scripts/install/* $ROOT/Install/

NET=""

echo -n "Enter the path to the Syllable-Net directory:"
read NET

if [ ! -e "$NET" ]; then
	echo "$NET does not exist.  Stoping."
	exit 1;
fi;

cp -r $NET/* $ROOT/Packages/net/

echo "Building ISO image"
./mkiso.sh $ROOT $VER

echo "Moving ISO image"
mv $VER.iso.gz objs/$VER-basic.iso.gz

echo "Done!"
exit 0


