#!/bin/sh
#
# Remember to always update the version numbers below!
BASE_VER=0.5.0
BASE_PACKAGE=base-syllable-$BASE_VER.tar.gz

# The following packages change versions less often
# ABrowse
AB_VER=0.3.4
AB_PACKAGE=abrowse-$AB_VER.bin.tar.gz
# Whisper
WH_VER=0.1.8
WH_PACKAGE=whisper-$WH_VER.bin.tar.gz
# Chat
CH_VER=0.0.1
CH_PACKAGE=chat-$CH_VER.bin.tar.gz

# Now on with the installation
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

echo; echo "Please wait...";echo
tar -xzvpf /boot/Packages/base/$BASE_PACKAGE -C /inst/

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
tar -xzpf /boot/Packages/net/$AB_PACKAGE -C /inst/atheos/Applications/

echo; echo "Installing Whisper"
tar -xzpf /boot/Packages/net/$WH_PACKAGE -C /inst/atheos/Applications/

echo; echo "Installing Chat"
tar -xzpf /boot/Packages/net/$CH_PACKAGE -C /inst/atheos/Applications/

# Sync disks
echo; echo "All files have been installed.  Finalising installation."
echo; echo "Synching disks.  Please wait; this may take some time.."
echo "WARNING! If you reboot or turn off your computer before Syllable has properly synched the disk, the installation WILL BE CORRUPT and you will not be able to boot your Syllable installation!  DO NOT INTERUPT THE INSTALLATION AT THIS POINT!"
sync

clear
echo "Syllable has now been installed!  Please press (R) to reboot your computer (Please remember to install Grub)  Press any other key to exit this installation script."
read ANSWER

if [[ $ANSWER = 'R' || $ANSWER = 'r' ]];then
	reboot
fi;
exit 0;

