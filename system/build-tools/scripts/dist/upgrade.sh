#! /bin/sh

echo "This will upgrade from Syllable V0.5.3 to V0.5.4"
echo "It requires that you have Syllable 0.5.3 installed."
echo "You can not upgrade from any other versions."
echo "You should run this script as the \"root\" user"
echo "Many system components will be replaced"
echo "and it will be necessarry to reboot your machine after"
echo "the upgrade."
echo ""
read -p "Do you want to continue? [y/N]:" -e answer

if [ "$answer" != "y" ]; then echo "Upgrade aborted"; exit 1; fi;

if [ -e pre-upgrade.sh ]
then
  ./pre-upgrade.sh
fi

echo ""
echo "Will now upgrade all system files. Do not interupt the"
echo "script after this point, as it will leave you with an inconsistent"
echo "and possibly unusable system"
echo ""
read -p "Hit enter to continue:" -e answer

if [ -e removed.list ]; then
	echo
	read -p "Would you like to delete obsolete files? [y/N]:" -e answer

	if [ "$answer" == "y" ]; then
		for file in $( cat removed.list ); do rm -Rv "/boot/$file"; done
	else
		echo "Leaving obsolete files.";
	fi
fi

if [ -e unzip ]
then
  ./unzip -K -d /boot/ upgrade.zip
else
  unzip -K -d /boot/ upgrade.zip
fi
sync

echo ""
echo "All system files are now upgraded.  You must reboot"
echo "your machine to complete this upgrade."

if [ -e post-upgrade.sh ]
then
  echo ""
  echo "You must run the script \"post-upgrade.sh\" after"
  echo "you have rebooted to complete the upgrade"
fi
