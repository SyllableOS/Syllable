#!/bin/sh

if [ "$1" = "" ]; then
	echo "You must supply the path to the base tarball"
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

if [ -e root ]; then
	echo "Root directory already exists.  Removing"
	rm -rf root
fi;

TARBALL=$1
VER=$2
BASE=base

if [ -e $BASE ]; then
	rm -rf $BASE
fi;

if [ ! -e bin ];then
	mkdir bin
fi;

if [ ! -e objs ];then
	mkdir objs
fi;

echo "Building ramdisk tools"

make -C src

echo "Unpacking $TARBALL"

mkdir $BASE
cd $BASE
tar -xzpf $TARBALL
cd ..

echo "Building boot images"
./mkboots.sh $BASE

echo "Building floppy images"
./mkminibase.sh $BASE

exit

echo "Building CD image"
./mkbase.sh $BASE $TARBALL $VER

echo "Cleaning up"
rm -rf $BASE

echo "Done!"
exit 0

