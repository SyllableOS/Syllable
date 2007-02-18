#!/bin/bash

VERSION=0.6.3

BUILD_DIR=$HOME/Build
INSTALLER_DIR=$BUILD_DIR/Installer
SCRIPTS_DIR=$HOME/bin

# If you want to automatically upload the build to
# an FTP server, set the following environment variables
# in your .profile:
#
# FTP_SERVER
# FTP_USER
# FTP_PASSWD

# Clean up from any previous build
if [ -e $INSTALLER_DIR/base-syllable.zip ]
then
  rm $INSTALLER_DIR/base-syllable.zip
fi
if [ -e $INSTALLER_DIR/files ]
then
  rm -rf $INSTALLER_DIR/files
fi

# XXXKV: Need to manually copy the GRUB files
cd $BUILD_DIR/system/stage/image
cp -a usr/grub/lib/grub/i386-pc/* boot/grub/

# XXXKV: Need to manually move the CUPS PPDs
if [ -e $INSTALLER_DIR/ppds ]
then
  rm -rf $INSTALLER_DIR/ppds
fi
mkdir -p $INSTALLER_DIR/ppds
cp -a usr/cups/share/cups/model/* $INSTALLER_DIR/ppds/
for PPD in `find usr/cups/share/cups/model/ -name *.ppd*`
do
  rm $PPD
done

# Generate the printers model list
$SCRIPTS_DIR/printers.sh $INSTALLER_DIR/ppds/ $BUILD_DIR/system/stage/image/usr/cups/share/cups/model/

# Finish the build and package it
cd $BUILD_DIR/system
image finish

cd $BUILD_DIR/system/stage/image
zip -yr9 $INSTALLER_DIR/base-syllable.zip *
sync

# XXXKV: Ensure we have the latest installer scripts
cd $INSTALLER_DIR/installer
cvs update -dP
sync

# Build the CD
cd $INSTALLER_DIR
./build-cd.sh "base-syllable.zip" "$BUILD_DIR/Net-Binaries" "$VERSION"

ISO="syllable-$VERSION-$(date +%Y%m%d).iso"

if [ -e syllable*.iso.bz2 ]
then
  rm syllable*.iso.bz2
fi
bzip2 -k9 $ISO

# Generate md5's
MD5S=md5sums
md5sum base-syllable.zip $ISO > $MD5S

# Transfer the files
FILES=`printf "base-syllable.zip $ISO.bz2 $MD5S\n"`
if [ -n "$FTP_USER" ]
then
  ftp -n $FTP_SERVER << END
quote user $FTP_USER
quote pass $FTP_PASSWD
passive
prompt
mput $FILES
quit
END
fi

