#!/bin/sh
#
# Builds a minimal base system for booting from a CD
# The first argument must be the directory containing the distribution files (E.g. root/)
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

ROOT=$1
TARBALL=$2
VER=$3

CDROOT=cd_root

if [ -e $CDROOT ]; then
	rm -rf CD$ROOT
fi;

mkdir $CDROOT

# Build the filesystem directory tree and then copy the files into it.
# If you need to add any files to the boot CD, this is the place to do it.

echo "Building base directory tree"

mkdir $CDROOT/boot

mkdir $CDROOT/atheos
mkdir $CDROOT/atheos/etc

mkdir $CDROOT/atheos/home
mkdir $CDROOT/atheos/home/root

mkdir $CDROOT/atheos/sys

mkdir $CDROOT/atheos/sys/bin
mkdir $CDROOT/atheos/sys/config
mkdir $CDROOT/atheos/sys/fonts
mkdir $CDROOT/atheos/sys/keymaps
mkdir $CDROOT/atheos/sys/libs

mkdir $CDROOT/atheos/sys/drivers

mkdir $CDROOT/atheos/sys/drivers/appserver
mkdir $CDROOT/atheos/sys/drivers/appserver/input
mkdir $CDROOT/atheos/sys/drivers/appserver/video

mkdir $CDROOT/atheos/sys/drivers/dev
mkdir $CDROOT/atheos/sys/drivers/dev/disk
mkdir $CDROOT/atheos/sys/drivers/dev/misc
mkdir $CDROOT/atheos/sys/drivers/dev/graphics
mkdir $CDROOT/atheos/sys/drivers/dev/hcd
mkdir $CDROOT/atheos/sys/drivers/dev/input

mkdir $CDROOT/atheos/sys/drivers/fs
mkdir $CDROOT/atheos/sys/drivers/bus

mkdir $CDROOT/atheos/usr
mkdir $CDROOT/atheos/usr/bin
mkdir $CDROOT/atheos/usr/share
mkdir $CDROOT/atheos/usr/share/terminfo
mkdir $CDROOT/atheos/usr/share/terminfo/x

mkdir $CDROOT/Install/

mkdir $CDROOT/Packages/
mkdir $CDROOT/Packages/base
mkdir $CDROOT/Packages/net

echo "Copying files from $ROOT"

cp -p $ROOT/atheos/etc/profile $CDROOT/atheos/etc/
cp -p $ROOT/atheos/etc/passwd $CDROOT/atheos/etc/
cp -p $ROOT/atheos/etc/termcap $CDROOT/atheos/etc/

cp -p $ROOT/atheos/home/root/.profile $CDROOT/atheos/home/root/

cp -p $ROOT/atheos/sys/drivers/appserver/input/* $CDROOT/atheos/sys/drivers/appserver/input/
cp -p $ROOT/atheos/sys/drivers/appserver/video/* $CDROOT/atheos/sys/drivers/appserver/video/

cp -p $ROOT/atheos/sys/drivers/dev/keybd $CDROOT/atheos/sys/drivers/dev/

cp -p $ROOT/atheos/sys/drivers/dev/disk/bios $CDROOT/atheos/sys/drivers/dev/disk/

cp -p $ROOT/atheos/sys/drivers/dev/misc/ps2aux $CDROOT/atheos/sys/drivers/dev/misc/
cp -p $ROOT/atheos/sys/drivers/dev/misc/serial $CDROOT/atheos/sys/drivers/dev/misc/

cp -dpr $ROOT/atheos/sys/drivers/dev/graphics/* $CDROOT/atheos/sys/drivers/dev/graphics/

cp -p $ROOT/atheos/sys/drivers/dev/hcd/usb_ehci $CDROOT/atheos/sys/drivers/dev/hcd/
cp -p $ROOT/atheos/sys/drivers/dev/hcd/usb_uhci $CDROOT/atheos/sys/drivers/dev/hcd/
cp -p $ROOT/atheos/sys/drivers/dev/hcd/usb_ohci $CDROOT/atheos/sys/drivers/dev/hcd/

cp -p $ROOT/atheos/sys/drivers/dev/input/usb_mouse $CDROOT/atheos/sys/drivers/dev/input/

cp -p $ROOT/atheos/sys/drivers/fs/afs $CDROOT/atheos/sys/drivers/fs/
cp -p $ROOT/atheos/sys/drivers/fs/fatfs $CDROOT/atheos/sys/drivers/fs/
cp -p $ROOT/atheos/sys/drivers/fs/ext2 $CDROOT/atheos/sys/drivers/fs/

cp -p $ROOT/atheos/sys/drivers/bus/pci $CDROOT/atheos/sys/drivers/bus/
cp -p $ROOT/atheos/sys/drivers/bus/usb $CDROOT/atheos/sys/drivers/bus/

cp -dpr $ROOT/atheos/sys/libs/* $CDROOT/atheos/sys/libs/

cp -p $ROOT/atheos/sys/fonts/Vera.ttf $CDROOT/atheos/sys/fonts/
cp -p $ROOT/atheos/sys/fonts/VeraMono.ttf $CDROOT/atheos/sys/fonts/

cp -p $ROOT/atheos/sys/keymaps/American $CDROOT/atheos/sys/keymaps/

cp -p $ROOT/atheos/sys/appserver $CDROOT/atheos/sys/

cp -p $ROOT/atheos/sys/bin/DiskManager $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/aterm $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/dbterm $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/devstat $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/format $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/fsprobe $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/init $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/mount $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/reboot $CDROOT/atheos/sys/bin/
cp -p $ROOT/atheos/sys/bin/unmount $CDROOT/atheos/sys/bin/

cp -p $ROOT/atheos/Applications/AEdit/AEdit $CDROOT/atheos/sys/bin/aedit

cp -p $ROOT/atheos/usr/coreutils/bin/cat $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/cp $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/cut $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/dd $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/echo $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/hostname $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/ln $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/ls $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/md5sum $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/mkdir $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/mv $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/printf $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/pwd $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/rm $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/rmdir $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/sleep $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/sync $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/whoami $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/coreutils/bin/uname $CDROOT/atheos/usr/bin/

cp -p $ROOT/atheos/usr/findutils/bin/find $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/gzip/bin/gzip $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/sed/bin/sed $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/tar/bin/tar $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/unzip/bin/unzip $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/ncurses/bin/clear $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/less/bin/less $CDROOT/atheos/usr/bin/

cp -p $ROOT/atheos/usr/bash/bin/bash $CDROOT/atheos/usr/bin/
cp -p $ROOT/atheos/usr/bin/grep $CDROOT/atheos/usr/bin/

cp -p $ROOT/atheos/usr/share/terminfo/x/xterm $CDROOT/atheos/usr/share/terminfo/x/

echo "Creating links"

cd $CDROOT/atheos/usr/bin
ln -s gzip gunzip
cd ../../../..

cd $CDROOT/atheos/sys/bin
ln -s /usr/bin/bash bash
ln -s /usr/bin/bash sh
ln -s /usr/bin/pwd pwd
cd ../../../..

echo "Copying init files"

cp ../scripts/sys/basic-init.sh $CDROOT/atheos/sys/init.sh
cp -p $ROOT/atheos/sys/config/appserver $CDROOT/atheos/sys/config/

# Ruby 1.8.1 is required for the installation scripts.
RUBY=""

echo -n "Enter the location of the Ruby 1.8.1 package:"
read RUBY

if [ ! -e "$RUBY" ]; then
	echo "$RUBY does not exist.  Stoping."
	exit 1;
fi;

tar -xzpf $RUBY -C $CDROOT/atheos/usr/
cd $CDROOT/atheos/usr/bin
ln -s /usr/ruby/bin/ruby ruby
cd ../../../..

echo "Copying boot image"
cp objs/boot.img $CDROOT/boot/boot.img

echo "Copying $TARBALL base package"
cp $TARBALL $CDROOT/Packages/base/

echo "Copying installation scripts"
cp -dpr ../scripts/install/* $CDROOT/Install/

# Run the documentation through sed to set the correct version number
sed -e "s/VER/$VER/" $CDROOT/Install/doc/welcome-template.txt > $CDROOT/Install/doc/welcome.txt
rm $CDROOT/Install/doc/welcome-template.txt

# Copy Net files
NET=""

echo -n "Enter the path to the Syllable-Net directory:"
read NET

if [ ! -e "$NET" ]; then
	echo "$NET does not exist.  Stoping."
	exit 1;
fi;

cp -r $NET/* $CDROOT/Packages/net/

echo "Building ISO image"
./mkiso.sh $CDROOT $VER

echo "Moving ISO image"
mv $VER.iso.gz objs/$VER-basic.iso.gz

echo "Done!"
exit 0


