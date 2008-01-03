#!/bin/bash

VERSION=0.6.5

BUILD_DIR=$HOME/Build
INSTALLER_DIR=$BUILD_DIR/Installer
SCRIPTS_DIR=$HOME/bin
LOG_DIR=$HOME/Logs

# If you want to automatically upload the build to
# an FTP server, set the following environment variables
# in your .profile:
#
# FTP_SERVER
# FTP_USER
# FTP_PASSWD

FINISH_LOG=$LOG_DIR/finish-std-out-err.log
FINISH_FAILURE_LOG=$LOG_DIR/finish-failures.log
FINISH_SUMMARY_LOG=$LOG_DIR/finish-summary.log

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
cp -a system/resources/cups/1.3.4/share/cups/model/* $INSTALLER_DIR/ppds/
for PPD in `find system/resources/cups/1.3.4/share/cups/model/ -name *.ppd*`
do
  rm $PPD
done

# Generate the printers model list
$SCRIPTS_DIR/printers.sh $INSTALLER_DIR/ppds/ $BUILD_DIR/system/stage/image/system/resources/cups/1.3.4/share/cups/model/

# Finish the build and package it
cd $BUILD_DIR/system
image finish > $FINISH_LOG 2>&1
#build log > $FINISH_LOG
build log failures > $FINISH_FAILURE_LOG
build log summary > $FINISH_SUMMARY_LOG
sync

# Package up and remove the development files
DEV_ARCHIVE="syllable-$VERSION-$(date +%Y%m%d)-development"

cd $BUILD_DIR/system/stage/image/system
zip -yr9 $INSTALLER_DIR/$DEV_ARCHIVE.zip development
rm -rf development

# Package the installation files
cd $BUILD_DIR/system/stage/image
zip -yr9 $INSTALLER_DIR/base-syllable.zip *
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

ISO="syllable-$VERSION-$(date +%Y%m%d).iso"

if [ -e syllable*.iso.7z ]
then
  rm syllable*.iso.7z
fi
7z a $ISO.7z $ISO

# Generate md5's
MD5S=md5sums
md5sum base-syllable.zip $ISO $ISO.7z $DEV_ARCHIVE.zip > $MD5S

# Transfer the files
FILES1=`printf "$FINISH_LOG $FINISH_FAILURE_LOG $FINISH_SUMMARY_LOG base-syllable.zip $ISO.7z\n"`
FILES2=`printf "$DEV_ARCHIVE.zip $MD5S\n"`
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
