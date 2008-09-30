#!/bin/bash

VERSION=0.6.6

LOG_DIR=$HOME/Logs
LOG=$LOG_DIR/std-out-err.log
FINISH_FAILURES=$LOG_DIR/finish-failures.log
FINISH_SUMMARY=$LOG_DIR/finish-summary.log

SCRIPTS_DIR=$HOME/bin
BUILD_DIR=$HOME/Build
INSTALLER_DIR=$BUILD_DIR/Installer
WORKING_COPY=$INSTALLER_DIR/system/stage/

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

# Finish the build and package it
cd $INSTALLER_DIR/system/
image finish 1>>$LOG 2>&1
build log failures > $FINISH_FAILURES
build log summary > $FINISH_SUMMARY

# Move the CUPS PPDs
if [ -e $INSTALLER_DIR/ppds ]
then
  rm -rf $INSTALLER_DIR/ppds
fi
mkdir -p $INSTALLER_DIR/ppds

cp -a $WORKING_COPY/image/system/resources/cups/1.3.4/share/cups/model/* $INSTALLER_DIR/ppds/
for PPD in `find $WORKING_COPY/image/system/resources/cups/1.3.4/share/cups/model/ -name *.ppd*`
do
  rm $PPD
done

# Generate the printers model list
$SCRIPTS_DIR/printers.sh $INSTALLER_DIR/ppds/ $WORKING_COPY/image/system/resources/cups/1.3.4/share/cups/model/

# Package up and remove the development files
FULL_VERSION="$VERSION-$(date +%Y%m%d)"
NAME="SyllableDesktop-$FULL_VERSION"
DEV_ARCHIVE="$NAME-development.i586"

construct distro SyllableDesktop $FULL_VERSION i586
mv $NAME.i586.zip ../base-syllable.zip

mv $DEV_ARCHIVE.zip.7z /usr/Builder/distributions/SyllableDesktop-$VERSION-development.i586.zip.7z
build pack development
mv SyllableDesktop-$VERSION-development-0.i586.zip ../$DEV_ARCHIVE.zip

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
FILES1=`printf "$LOG $FINISH_FAILURES $FINISH_SUMMARY\n"`
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
