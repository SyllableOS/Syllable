#! /bin/sh

echo "This will upgrade from Syllable V0.4.0 to V0.4.0a"
echo "It requires that you have Syllable 0.4.0 installed."
echo "You can not upgrade from any other versions."
echo "You should run this script as the \"root\" user"
echo "Many system components will be replaced"
echo "and it will be necessarry to reboot your machine after"
echo "the upgrade."
echo ""
read -p "Do you want to continue? [y/N]:" -e answer

if [ "$answer" != "y" ]; then echo "Upgrade aborted"; exit 1; fi;

echo ""
echo "Will now upgrade all system files. Do not interupt the"
echo "script after this point, as it will leave you with an inconsistent"
echo "and possibly unusable system"
echo ""
read -p "Hit enter to continue:" -e answer

if [ -e removed.txt ]; then
	echo
	read -p "Would you like to delete obsolete files? [y/N]:" -e answer

	if [ "$answer" == "y" ]; then
		for file in $( cat removed.txt ); do rm -Rv "/boot/$file"; done
	else
		echo "Leaving obsolete files.";
	fi
fi

tar -C /boot/ -xUvpzf upgrade.tgz
sync

echo ""
echo "All system files are now upgraded.  You must reboot"
echo "your machine to complete this upgrade."

