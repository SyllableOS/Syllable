#! /bin/sh

if [ "$USER" != "root" ]
then
	echo "You should be running the installation as the administrative user."
	echo "Installation aborted."
	echo "Log in as the \"root\" user and try again."
	exit 1
fi

echo "This will install the packages in the Developer Pack. Previously"
echo "installed packages of the same name will be removed first."
echo ""
read -p "Do you want to continue (y/N)? " -e answer

if [ "$answer" != "y" ]
then
	echo "Installation aborted."
	exit 1
fi

for package in autoconf automake make binutils gcc patch m4 flex bison cvs
do
	echo ""

	if [ "$PWD" != "/usr" ]
	then
		if [ -e "/usr/$package" ]
		then
			echo "Removing existing /usr/$package."
			pkgmanager -r /usr/$package
			rm -r /usr/$package
		fi
		echo "Moving $package to /usr."
		mv $package /usr
	fi
	echo "Registering $package."
	pkgmanager -a /usr/$package
done

echo ""
sync
echo "Finished."
