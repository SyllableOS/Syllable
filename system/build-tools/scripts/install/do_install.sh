#!/bin/sh

clear
cat doc/partition.txt

echo -n "Do you need to create a new partition? (Y/n) "
read ANSWER

if [[ $ANSWER = 'Y' || $ANSWER = 'y' ]]; then
      DiskManager
fi;

clear
cat doc/format.txt

while [ 1 ]
do
	echo "Enter the name of the device that you would like to install Syllable onto"; echo -n "(E.g. /dev/disk/ide/hda/1 ): "
	read DRIVE
	if [ -e $DRIVE ]; then
		echo; echo -n "Format $DRIVE  Are you sure? (Y/n) "
		read ANSWER
		if [[ $ANSWER = 'Y' || $ANSWER = 'y' ]];then
			break
		else
			echo; continue
		fi;
	else
		echo; echo "$DRIVE does not exist"
	fi;

done

format $DRIVE afs "Syllable"

if [ $? -ne 0 ]; then
	echo; echo "Failed to format $DRIVE.  Stopping"
	exit 3;
fi;

mkdir /inst
mount -t afs $DRIVE /inst

clear
cat doc/installation.txt

echo; echo "Please wait..."
tar -xzpf /boot/Packages/base/base-syllable-0.4.5.tar.gz -C /inst/

if [ $? -ne 0 ]; then
	echo; echo "Failed to extract base package to $DRIVE.  Stopping"
	exit 4;
else
	echo; echo -n "The base package has been installed.  Please press any key to continue."
	read
fi;

clear
cat doc/configure1.txt
echo; echo -n "Please press any key to continue."
read
cat doc/configure2.txt
echo; echo -n "Please press any key to continue."
read
cat doc/configure3.txt
echo; echo -n "Please press any key to continue."
read

aedit /inst/boot/grub/menu.lst

clear

# Install Syllable-Net packages
echo; echo "Installing ABrowse"
tar -xzpf /boot/Packages/net/abrowse-0.3.4.bin.tar.gz -C /inst/atheos/Applications/

echo; echo "Installing Whisper"
tar -xzpf /boot/Packages/net/whisper-0.1.8.bin.tar.gz -C /inst/atheos/Applications/

echo; echo "Installing Chat"
tar -xzpf /boot/Packages/net/chat-0.0.1.bin.tar.gz -C /inst/atheos/Applications/

echo
exit 0

