#!/bin/sh

OLD_DIST=$1
NEW_DIST=$2

# Pre-clean
if [ -e removed.txt ]; then
	rm removed.txt
fi

if [ -e changed.txt ]; then
	rm changed.txt
fi

# Start
echo Getting list of changed files

# Untar the old dist
tar -xzpf $OLD_DIST -C .

# Get the list of changed files
./diff.sh $NEW_DIST changed.txt

# Finished with the old dist
rm -r atheos
rm -r boot

echo Getting list of removed files

# Untar the new dist
tar -xzpf $NEW_DIST -C .

# Get the list of changed files
./diff.sh $OLD_DIST removed.txt

# Now we have two files; removed.txt & changed.txt
echo
echo Please check both removed.txt \& changed.txt.  Press Enter when ready
read FOO

echo
echo Copying changed files

# Get the changed files
mkdir upgrade

for FILE in $( cat changed.txt )
do
	cp --parents -dpr $FILE upgrade/
done

# Check the upgrade files are in place
echo
echo Please check the upgrade/ directory to ensure all files have been copied.  Press Enter when ready
read FOO

# Tar it all up
cd upgrade
tar -czvpf upgrade.tgz *
mv upgrade.tgz ..
cd ..
rm -r upgrade/*
mv upgrade.tgz upgrade/

# Copy in the removed files list
cp removed.txt upgrade/

# Copy in the upgrade script
cp upgrade.sh upgrade/

# Tar up the upgrade
tar -czvpf upgrade.tgz upgrade/

# Clean up
rm -r atheos
rm -r boot

# Finished
echo
echo Done
exit 0

