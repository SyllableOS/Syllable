#!/bin/sh
#
# Build the root tree-structure to install into

ROOT_DIR=$1

echo Building system tree directory structure in $ROOT_DIR

# Check to see if we've built the tree already

if [ -e $ROOT_DIR ];then
	echo $ROOT_DIR already exists, assuming it is complete \& exiting now.
	exit 0
fi

# Build the tree & all sub-directories
mkdir $ROOT_DIR

# boot dir structure
echo Building $ROOT_DIR/boot

mkdir $ROOT_DIR/boot
mkdir $ROOT_DIR/boot/grub

# atheos dir structure
echo Building $ROOT_DIR/atheos

mkdir $ROOT_DIR/atheos

mkdir $ROOT_DIR/atheos/Applications
mkdir $ROOT_DIR/atheos/Documentation
mkdir $ROOT_DIR/atheos/autolnk
mkdir $ROOT_DIR/atheos/etc

mkdir $ROOT_DIR/atheos/home
mkdir $ROOT_DIR/atheos/home/root
mkdir $ROOT_DIR/atheos/home/guest

mkdir $ROOT_DIR/atheos/sys
mkdir $ROOT_DIR/atheos/sys/bin

mkdir $ROOT_DIR/atheos/sys/config
mkdir $ROOT_DIR/atheos/sys/config/drivers
mkdir $ROOT_DIR/atheos/sys/config/drivers/dev
mkdir $ROOT_DIR/atheos/sys/config/default_user
mkdir $ROOT_DIR/atheos/sys/config/default_user/config

mkdir $ROOT_DIR/atheos/sys/drivers
mkdir $ROOT_DIR/atheos/sys/drivers/appserver
mkdir $ROOT_DIR/atheos/sys/drivers/appserver/decorators
mkdir $ROOT_DIR/atheos/sys/drivers/appserver/input
mkdir $ROOT_DIR/atheos/sys/drivers/appserver/video

mkdir $ROOT_DIR/atheos/sys/drivers/dev
mkdir $ROOT_DIR/atheos/sys/drivers/dev/disk
mkdir $ROOT_DIR/atheos/sys/drivers/dev/misc
mkdir $ROOT_DIR/atheos/sys/drivers/dev/net
mkdir $ROOT_DIR/atheos/sys/drivers/dev/net/eth
mkdir $ROOT_DIR/atheos/sys/drivers/dev/video

mkdir $ROOT_DIR/atheos/sys/drivers/fs

mkdir $ROOT_DIR/atheos/sys/drivers/net
mkdir $ROOT_DIR/atheos/sys/drivers/net/if

mkdir $ROOT_DIR/atheos/sys/fonts
mkdir $ROOT_DIR/atheos/sys/icons
mkdir $ROOT_DIR/atheos/sys/include
mkdir $ROOT_DIR/atheos/sys/keymaps
mkdir $ROOT_DIR/atheos/sys/libs
mkdir $ROOT_DIR/atheos/sys/translators

mkdir $ROOT_DIR/atheos/tmp

mkdir $ROOT_DIR/atheos/usr
mkdir $ROOT_DIR/atheos/usr/bin
mkdir $ROOT_DIR/atheos/usr/etc
mkdir $ROOT_DIR/atheos/usr/include
mkdir $ROOT_DIR/atheos/usr/lib
mkdir $ROOT_DIR/atheos/usr/local
mkdir $ROOT_DIR/atheos/usr/man
mkdir $ROOT_DIR/atheos/usr/sbin
mkdir $ROOT_DIR/atheos/usr/share

mkdir $ROOT_DIR/atheos/usr/glibc2
mkdir $ROOT_DIR/atheos/usr/groff

mkdir $ROOT_DIR/atheos/var
mkdir $ROOT_DIR/atheos/var/log

# Phew!
echo Done building distribution tree in $ROOT_DIR
exit 0

