#! /bin/sh

if [ "$USER" != "root" ]
then
	echo "The installation is not being run by the administrative user."
	echo "Installation aborted."
	echo "You can log in as the \"root\" user and try again."
	exit 1
fi

echo "This will install the components contained in the developer files pack."
echo "Previously installed developer files will be deleted first."
echo ""
read -p "Do you want to continue (y/N)? " -e answer

if [ "$answer" != "y" ]
then
	echo "Installation aborted."
	exit 1
fi

echo ""

if [ -e "/system/development" ]
then
	for package in `ls /system/development/resources/`
	do
		if [ "$package" == "orca" ]
		then
			package unregister /system/development/resources/orca /system/indexes
			sync
		else
			for version in `ls /system/development/resources/$package/`
			do
				package unregister /system/development/resources/$package/$version /system/indexes
				sync
			done
		fi
	done

	echo "Deleting existing development files"
	rm -r /system/development
	sync
fi

echo ""

echo "Installing files"
unzip Syllable*-development.*.zip -d /system
sync

for package in `ls /system/development/resources/`
do
	if [ "$package" == "orca" ]
	then
		package register /system/development/resources/orca /system/indexes
		sync
	else
		for version in `ls /system/development/resources/$package/`
		do
			echo ""
			package register /system/development/resources/$package/$version /system/indexes
			sync
		done
	fi
done

echo ""
echo "Done."
