#!/bin/sh

# Pre-clean
stale_files='index.html all_packages.in all_packages.out all_packages packages'
for stale in $stale_files;do
	if [ -e $stale ];then
		rm $stale
	fi
done

# Get the latest package list
wget http://prdownloads.sourceforge.net/syllable/ -O index.html

# Parse the list and pull out all links
cat index.html | grep .tgz | cut -d \" -f 8 | sort -r > all_packages.in

# Remove the following exceptions
package_exceptions='base documentation upgrade rc3'
for exc in $package_exceptions;do
	cat all_packages.in | grep -v $exc > all_packages.out
	mv all_packages.out all_packages.in
done
mv all_packages.in all_packages

# Pull only the newest packages
this_file=""
last_file=""
this_package=""
last_package=""
for this_file in `cat all_packages`;do
	this_package=`echo $this_file | cut -d . -f 1`
	if [[ $this_package != $last_package ]];then
		echo $this_file >> packages
	else
		echo "Package $this_file is an older version of package $last_file"
	fi
	last_package=$this_package
	last_file=$this_file
done

# Pull all the files down
mkdir -p Download
mv packages Download/
cd Download

# Possible mirrors are
#
# http://optusnet.dl.sourceforge.net/sourceforge/
# http://heanet.dl.sourceforge.net/sourceforge/
# http://aleron.dl.sourceforge.net/sourceforge/
# http://umn.dl.sourceforge.net/sourceforge/
# http://belnet.dl.sourceforge.net/sourceforge/

sourceforge_mirror=http://heanet.dl.sourceforge.net/sourceforge/

for package in `cat packages`;do
	package_file=`echo $package | cut -d / -f 3`
	echo $package_file
	if [ ! -e $package_file ];then
		echo "Getting $package"
		url=$sourceforge_mirror$package
		wget $url
	fi
done

# Clean up
for stale in $stale_files;do
	if [ -e $stale ];then
		rm $stale
	fi
done

