#!/bin/sh
#
# Builds the ISO CD image from the CD root directory
# The first argument must be the path to the root directory containing the CD files
# The second argument is the version number E.g. 0.4.4
 
if [ ! -e "$1" ]; then
	echo "You must supply the path to the root directory (E.g. root/)"
	exit 1
fi;

if [ "$2" = "" ]; then
	echo "You must supply the version number (E.g. 0.4.4)"
	exit 1
fi;

ROOT=$1
VER=$2
NAME="Syllable $VER"

echo "Building ISO image"
mkisofs -iso-level 3 -L -R -V "$NAME" -b boot/boot.img -o $VER.iso $ROOT

echo "Compressing ISO image"
gzip $VER.iso

echo "Done!"
exit 0

