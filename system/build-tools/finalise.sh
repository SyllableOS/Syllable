#!/bin/sh

export LOCAL=$(pwd)

# Create some symlinks
cd $DIST_DIR/atheos/sys/bin
ln -s /usr/bin/bash bash
ln -s /usr/bin/bash sh
cd $LOCAL

# Clean out ALL Makefiles
cd $DIST_DIR

echo Removing Makefiles

for file in $(find * | grep -w Makefile )
do
	echo "Removing $file"
	rm $file
done

echo "Removing any CVS dirs"

for file in $(find * | grep -w CVS )
do
	echo "Removing $file"
	rm -r $file
done

# Make the tarball
cd $LOCAL
cd $DIST_DIR
tar -czvpf $DIST_NAME *
mv $DIST_NAME ..
cd ..

# Finished
exit 0

