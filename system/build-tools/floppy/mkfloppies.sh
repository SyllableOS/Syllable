#!/bin/sh
#
# Builds the floppy images with the ramdisk images

if [ ! -e temp ]; then
	mkdir temp
fi;

cd temp

mv ../objs/syllable1.img .

cp ../images/blank.img ./syllable2.img
cp ../images/blank.img ./syllable3.img

mkdir disk

mount -t fatfs syllable1.img disk
cp ../objs/bimage1.bin.gz disk/
unmount disk

mount -t fatfs syllable2.img disk
cp ../objs/bimage2.bin.gz disk/
unmount disk

mount -t fatfs syllable3.img disk
cp ../objs/bimage3.bin.gz disk/
unmount disk

rmdir disk

mv syllable1.img ../objs/
mv syllable2.img ../objs/
mv syllable3.img ../objs/

cd ..
rmdir temp

exit 0

