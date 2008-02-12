#!/bin/bash

VERSION=0.6.6

BUILD_DIR=$HOME/Build
LOG_DIR=$HOME/Logs
LOG=$LOG_DIR/std-out-err.log
FINISH_FAILURES=$LOG_DIR/finish-failures.log
FINISH_SUMMERY=$LOG_DIR/finish-summery.log
INSTALLER_DIR=$BUILD_DIR/Installer
WORKING_COPY=$INSTALLER_DIR/system/stage/
SCRIPTS_DIR=$HOME/bin
LOG_DIR=$HOME/Logs

# If you want to automatically upload the build to
# an FTP server, set the following environment variables
# in your .profile:
#
# FTP_SERVER
# FTP_USER
# FTP_PASSWD

echo "Cleaning up old files"

# Clean up from any previous build
if [ -e $INSTALLER_DIR/base-syllable.zip ]
then
  rm $INSTALLER_DIR/base-syllable.zip
fi
if [ -e $INSTALLER_DIR/files ]
then
  rm -rf $INSTALLER_DIR/files
fi
if [ -e $WORKING_COPY ]
then
  rm -rf $WORKING_COPY
fi
mkdir -p $WORKING_COPY

# Copy the build image to a working location
echo "Creating a working copy"

cp -a $BUILD_DIR/system/stage/image $WORKING_COPY

# Copy the GRUB files
cd $WORKING_COPY/image
cp -a usr/grub/lib/grub/i386-pc/* boot/grub/

# Move the CUPS PPDs
if [ -e $INSTALLER_DIR/ppds ]
then
  rm -rf $INSTALLER_DIR/ppds
fi
mkdir -p $INSTALLER_DIR/ppds
cp -a system/resources/cups/1.3.4/share/cups/model/* $INSTALLER_DIR/ppds/
for PPD in `find system/resources/cups/1.3.4/share/cups/model/ -name *.ppd*`
do
  rm $PPD
done

# Generate the printers model list
$SCRIPTS_DIR/printers.sh $INSTALLER_DIR/ppds/ $WORKING_COPY/image/system/resources/cups/1.3.4/share/cups/model/

# Finish the build and package it
cd $INSTALLER_DIR
image finish 1>>$LOG 2>&1
build log failures > $FINISH_FAILURES
build log summery > $FINISH_SUMMERY

echo "Packaging the development files"

# Package up and remove the development files
DEV_ARCHIVE="SyllableDesktop-$VERSION-$(date +%Y%m%d)-development.i586"

cd $WORKING_COPY/image/system
# Let external compression do its work
zip -ry0 $INSTALLER_DIR/$DEV_ARCHIVE.zip development
rm -rf development

echo "Packaging the installation files"

# Package the installation files
cd $WORKING_COPY/image
# Let external compression do its work
zip -ry0 $INSTALLER_DIR/base-syllable.zip *
sync

# XXXKV: Ensure we have the latest installer scripts
cd $INSTALLER_DIR/installer
cvs update -dP
sync

cd $INSTALLER_DIR/documentation
cvs update -dP
sync

# Build the CD
cd $INSTALLER_DIR
./build-cd.sh "base-syllable.zip" "$BUILD_DIR/Net-Binaries" "$VERSION"

echo "Compressing the ISO"

ISO="SyllableDesktop-$VERSION-$(date +%Y%m%d).i586.iso"

if [ -e Syllable*.iso.7z ]
then
  rm Syllable*.iso.7z
fi
7z a $ISO.7z $ISO

# Generate md5's
MD5S=md5sums
md5sum base-syllable.zip $ISO $ISO.7z $DEV_ARCHIVE.zip > $MD5S

echo "Uploading"

# Transfer the files
FILES1=`printf "$FINISH_FAILURES $FINISH_SUMMERY\n"`
FILES2=`printf "base-syllable.zip $ISO.7z $DEV_ARCHIVE.zip $MD5S\n"`
if [ -n "$FTP_USER" ]
then
  ftp -n $FTP_SERVER << END
quote user $FTP_USER
quote pass $FTP_PASSWD
passive
prompt
mput $FILES1
mput $FILES2
quit
END
fi
