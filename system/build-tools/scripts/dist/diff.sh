#!/bin/sh

# Initiliase variables
NEW_DIST=$1
ADDED_LIST=$2

DIFF_LIST=diff.txt

# Diff the tarball to the file-system
tar -dzf $NEW_DIST 1>$DIFF_LIST

# Extract the filenames
cut $DIFF_LIST -d : -f 1 >> $ADDED_LIST

# Clean up
rm $DIFF_LIST

exit 0

