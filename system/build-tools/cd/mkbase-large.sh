#!/bin/sh
#
# Builds a "large" CD image, which includes all of the available packages, source & Extras.  This
# script should only be run after the "Basic" ISO image has been created, and the cd_root/ directory
# must exist.

if [ "$1" = "" ]; then
	echo "You must supply the version number.  Stopping"
	exit 1
fi;

VER=$1
CDROOT=cd_root

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
cp -dpr $DOCS $CDROOT/Packages/base/

echo "Copying upgrade tarball from $UPGRADE"
cp -dpr $UPGRADE $CDROOT/Packages/base/

mkdir $CDROOT/Source
echo "Copying CVS tarball from $CVS"
cp -dpr $CVS $CDROOT/Source/

echo "Copying packages from $PACKAGES"
mkdir $CDROOT/Packages/optional
cp -dpr $PACKAGES/* $CDROOT/Packages/optional/

echo "Copying bonus files from $BONUS"
mkdir $CDROOT/Bonus
cp -dpr $BONUS/* $CDROOT/Bonus/

echo "Building ISO image"
./mkiso.sh $CDROOT $VER

echo "Moving ISO image"
mv $VER.iso.gz objs/$VER-large.iso.gz

echo "Done!"
exit 0

