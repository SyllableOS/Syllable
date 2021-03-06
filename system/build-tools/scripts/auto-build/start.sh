#!/bin/bash

export LOG_DIR=$HOME/Logs
export LOG=$LOG_DIR/stderr.log
export SOURCE_DIR=$HOME/Source/Anonymous
export BUILD_DIR=$HOME/Build

SCRIPTS_DIR=$HOME/bin
INSTALLER_DIR=$BUILD_DIR/Installer

if [ -z $CVS_TAG ]
then
  CVS_TAG=HEAD
fi

# Clean up from any previous attempt
if [ ! -e $LOG_DIR ]
then
  mkdir $LOG_DIR
else
  rm $LOG_DIR/*
fi
touch $LOG
if [ -e $BUILD_DIR/system ]
then
  rm -rf $BUILD_DIR/system
fi
sync

# Mark the start
printf "Update started at %s\n" "`date`"

# Make sure we have the latest of source & scripts
echo "Updating sources"
cd $SOURCE_DIR/syllable
unbuffer cvs -z9 -q update -r $CVS_TAG -dP >>$LOG
sync

# Copy sources
cp -a $SOURCE_DIR/syllable/system $BUILD_DIR
sync

# Copy scripts
cp -f $SOURCE_DIR/syllable/system/build-tools/scripts/auto-build/*.sh $SCRIPTS_DIR
cp -f $SOURCE_DIR/syllable/system/build-tools/scripts/auto-build/build-cd.sh $INSTALLER_DIR
sync

printf "Update finished at %s\n" "`date`"

# Invoke the build script
exec $SCRIPTS_DIR/build.sh

exit $?
