#!/bin/sh

if [ "$1" = "" ]; then
	echo "You must supply the version number.  Stopping"
	exit 1
fi;

VER=$1

ROOT=root

UPGRADE=""

echo -n "Enter the path to the upgrade tarball:"
read UPGRADE

if [ ! -e "$UPGRADE" ]; then
	echo "$UPGRADE does not exist.  Stoping."
	exit 1;
fi;

DOCS=""

echo -n "Enter the path to the documentation tarball:"
read DOCS

if [ ! -e "$DOCS" ]; then
	echo "$DOCS does not exist.  Stoping."
	exit 1;
fi;

CVS=""

echo -n "Enter the path to the CVS tarball:"
read CVS

if [ ! -e "$CVS" ]; then
	echo "$CVS does not exist.  Stoping."
	exit 1;
fi;

PACKAGES=""

echo -n "Enter the path to the Packages directory:"
read PACKAGES

if [ ! -e "$PACKAGES" ]; then
	echo "$PACKAGES does not exist.  Stoping."
	exit 1;
fi;

BONUS=""

echo -n "Enter the path to the Bonus directory"
read BONUS

if [ ! -e "$BONUS" ]; then
	echo "$BONUS does not exist.  Stoping."
	exit 1;
fi;

echo "Copying documentation tarball from $DOCS"
cp -dpr $DOCS $ROOT/Packages/base/

echo "Copying upgrade tarball from $UPGRADE"
cp -dpr $UPGRADE $ROOT/Packages/base/

mkdir $ROOT/Source
echo "Copying CVS tarball from $CVS"
cp -dpr $CVS $ROOT/Source/

echo "Copying packages from $PACKAGES"
mkdir $ROOT/Packages/optional
cp -dpr $PACKAGES/* $ROOT/Packages/optional/

echo "Copying bonus files from $BONUS"
mkdir $ROOT/Bonus
cp -dpr $BONUS/* $ROOT/Bonus/

echo "Building ISO image"
./mkiso.sh $ROOT $VER

echo "Moving ISO image"
mv $VER.iso.gz objs/$VER-large.iso.gz

echo "Done!"
exit 0

