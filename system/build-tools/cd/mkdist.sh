#!/bin/sh
#
# Root script to build a bootable CD ISO image from an installation tarball

if [ "$1" = "" ]; then
	echo "You must supply the path to the base zip file"
	exit 1
fi;

if [ ! -e "$1" ]; then
	echo "$1 does not exist.  Stopping"
	exit 1
fi;

if [ "$2" = "" ]; then
	echo "You must supply the version number (E.g. 0.4.4)"
	exit 1
fi;

# The cd_root directory is the "root" directory which contains the file system for the CD
if [ -e cd_root ]; then
	echo "cd_root directory already exists.  Removing"
	rm -rf cd_root
fi;

ZIPFILE=$1
VER=$2
# The root directory which we extract the installation tarball too
ROOT=root

if [ -e $ROOT ]; then
	rm -rf $ROOT
fi;

if [ ! -e objs ]; then
	mkdir objs
fi;

echo "Unpacking $ZIPFILE"

mkdir $ROOT
cd $ROOT
unzip -d . $ZIPFILE
cd ..

echo "Building CD boot floppy image"
./mkboot.sh $ROOT

echo "Building CD image"
./mkbase.sh $ROOT $ZIPFILE $VER

echo "Done!"
exit 0

