#!/bin/bash

OLD_DIST=$1
NEW_DIST=$2

# Ensure we have valid packages
if [ -z "$OLD_DIST" ]
then
  echo -n "Enter the full path to the old distribution:"
  read OLD_DIST
fi

if [ ! -e "$OLD_DIST" ]
then
  echo "$OLD_DIST does not exist."
  exit 1
fi

if [ -z "$NEW_DIST" ]
then
  echo -n "Enter the full path to the new distribution:"
  read NEW_DIST
fi

if [ ! -e "$NEW_DIST" ]
then
  echo "$NEW_DIST does not exist."
  exit 1
fi

# Create staging areas
if [ -e old ]
then
  echo "Removing previous directory \"old\""
  rm -rf old
fi
mkdir -p old

if [ -e new ]
then
  echo "Removing previous directory \"new\""
  rm -rf new
fi
mkdir -p new

# Work out if these are (old) tarballs or (new) zip files and unpack them
if [ ! -z "`echo $OLD_DIST | grep tar`" ]
then
  # Tarball
  echo "Un-tarring $OLD_DIST to old/"
  tar xzvpf $OLD_DIST -C old
else
  # Zip file
  echo "Un-zipping $OLD_DIST to old/"
  unzip -d old $OLD_DIST
fi
sync

if [ ! -z "`echo $NEW_DIST | grep tar`" ]
then
  # Tarball
  echo "Un-tarring $NEW_DIST to new/"
  tar xzvpf $NEW_DIST -C new
else
  # Zip file
  echo "Un-zipping $NEW_DIST to new/"
  unzip -d new $NEW_DIST
fi
sync

# Diff them.  We want two lists of files; obsolete diles to be removed
# during the upgrade and modified and new files to be copied to the system
# to perform the upgrade.
if [ -e all_files.list ]
then
  rm all_files.list
fi

cd old/
find >> ../all_files.list
cd ..

cd new/
find >> ../all_files.list
cd ..

if [ -e new.list ]
then
  rm new.list
fi

if [ -e old.list ]
then
  rm old.list
fi

if [ -e changed.list ]
then
  rm changed.list
fi

for FILE in `sort -u all_files.list`
do
  echo "$FILE"

  if [ ! -e "old/$FILE" ]
  then
    # Doesn't exist in old, must be a new file
    echo "$FILE" >> new.list
    continue
  fi

  if [ ! -e "new/$FILE" ]
  then
    # Doesn't exist in new, must be an old file
    echo "$FILE" >> old.list
    continue
  fi

  # Must exist in both old and new

  # Ignore directories that exist in both old and new
  if [ -d "old/$FILE" ]
  then
    continue
  fi

  # Check for symlinks
  if [ -h "old/$FILE" ]
  then
    # Get the link target of both links
    OLD_TARGET=`ls -l old/$FILE | cut -d \> -f 2`
    NEW_TARGET=`ls -l new/$FILE | cut -d \> -f 2`

    if [[ "$OLD_TARGET" != "$NEW_TARGET" ]]
    then
      # Symlink changed
      echo "$FILE" >> changed.list
    else
      continue
    fi
  fi

  # See if the file has changed
  if [ ! -z "`cmp old/$FILE new/$FILE`" ]
  then
    # File has changed
    echo "$FILE" >> changed.list
  fi
done

echo "Check files"
read FOO

# Create upgrade directory and copy files
if [ -e upgrade-files ]
then
  echo "Removing previous directory \"upgrade-files\""
  rm -rf upgrade-files
fi
mkdir -p upgrade-files

# Copy upgrade files
echo "Copying new files"
cd new/
for FILE in `cat ../new.list`
do
  cp --parents -dpr "$FILE" ../upgrade-files/
done

echo "Copying changed files"
for FILE in `cat ../changed.list`
do
  cp --parents -dpr "$FILE" ../upgrade-files/
done
cd ..

cd upgrade-files
# Zip up the upgrade files
zip -yr9 upgrade.zip atheos boot
mv upgrade.zip ..
cd ..

rm -rf upgrade-files

if [ -e upgrade ]
then
  echo "Removing previous directory \"upgrade\""
  rm -rf upgrade
fi
mkdir -p upgrade

# Copy in scripts and files
mv upgrade.zip upgrade/
cp -f old.list upgrade/removed.list
cp -f pre-upgrade.sh upgrade.sh post-upgrade.sh upgrade/
cp -f /usr/zip/bin/unzip upgrade/

# Zip it all up
zip -yr9 upgrade.zip upgrade

# Cleanup
rm -rf upgrade
rm -rf old
rm -rf new

exit 0
