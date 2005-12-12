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
# Do not mess with the following line unless you know exactly what you are doing.  Getting the
# options wrong will mean the CD will be unbootable (And likely unreadable)
mkisofs -iso-level 3 --allow-leading-dots -R -V "$NAME" -b boot/boot.img -o $VER.iso $ROOT

echo "Done!"
exit 0

