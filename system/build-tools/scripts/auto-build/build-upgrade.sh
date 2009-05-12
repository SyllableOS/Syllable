#!/bin/bash

# Global variables
debug=false
keep_state=false

unpack()
{
	if [ $keep_state == false ]
	then
		rm -rf old new 2>>/dev/null
		mkdir old new 2>> /dev/null

		echo "Unpacking $1"
		unzip -qq $1 -d old
		if [ $? -gt 0 ];then return $?;fi

		echo "Unpacking $2"
		unzip -qq $2 -d new
		if [ $? -gt 0 ];then return $?;fi
	else
		if [ ! -e old ]
		then
			mkdir old 2>>/dev/null
			echo "Unpacking $1"
			unzip -qq $1 -d old
			if [ $? -gt 0 ];then return $?;fi
		fi
		if [ ! -e new ]
		then
			mkdir new 2>>/dev/null
			echo "Unpacking $2"
			unzip -qq $2 -d new
			if [ $? -gt 0 ];then return $?;fi
		fi
	fi

	return 0
}

build_file_list()
{
	cwd=$PWD

	cd $1
	find -print | sort -u > $2
	cd $cwd
}

slurp_file_list()
{
	# Only break on newline during read
	IFS=$'\x0A'

	# Read the list into an array until EOF
	read -a $2 -d $'\x04' < $1

	if [ $debug == true ]
	then
		list=$2
		count=${#list[@]}
		index=0
		while [ $index -lt $count ]
		do
			echo ${list[$index]}
			let index=$index+1
		done
	fi
}

extract_script()
{
	grep "#$2" < $0 | sed -e "s/#$2//g" > $1/$3
	chmod +x $1/$3
}

gen_upgrade_script()
{
	extract_script $1 1 upgrade._h

	# Set the correct version numbers
	sed -e "s/OLD_VERSION/$2/" -e "s/NEW_VERSION/$3/" < $1/upgrade._h > $1/upgrade.sh
	rm $1/upgrade._h
	chmod +x $1/upgrade.sh
}

gen_post_upgrade_script()
{
	extract_script $1 2 post-upgrade.sh
}

usage()
{
	echo "usage: $0 [-h|--help] | [-o <old package> -v <x.y.z> -n <new package> -V <x.y.z>] [-k] [-F]"
	echo
	echo "Creates an upgrade package that can upgrade the previous version to the"
	echo "latest version."
	echo
	echo "-h, --help	Display this message"
	echo "-o		Base package of the old version"
	echo "-v		Old version number"
	echo "-n		Base package of the new version"
	echo "-V		New version number"
	echo "-k		Do not delete any files which have already been created"
	echo "-F		Do not create the final upgrade package"
}

# Script entry point
old_package=""
old_version="0.0.0"
new_package=""
new_version="9.9.9"
final_archive=true

while [ $# -gt 0 ]
do
	case "$1" in
		"-h"|"--help" )
			usage
			exit 0
			;;

		"-o" )
			shift
			old_package=$1
			;;

		"-v" )
			shift
			old_version=$1
			;;

		"-n" )
			shift
			new_package=$1
			;;

		"-V" )
			shift
			new_version=$1
			;;

		"-k" )
			keep_state=true
			;;

		"-N" )
			final_archive=false
			;;

		# Undocumented test switch for quick access to functions
		"-t" )
			exit 0
			;;
	esac
	shift
done

# Ensure we have both packages
if [[ -z $old_package || -z $new_package ]]
then
	usage
	exit 1
fi
if [ ! -e $old_package ]
then
	echo "$old_package does not exist"
	exit 1
fi
if [ ! -e $new_package ]
then
	echo "$new_package does not exist"
	exit 1
fi

# Everything is good
echo "Building upgrade from $old_package to $new_package"
if [ $keep_state == true ];then echo "Keeping previous state";fi
echo

unpack $old_package $new_package
if [ $? -gt 0 ];then echo "fail";exit $?;fi

if [ $keep_state == false ];then rm *.list 2>> /dev/null ;fi

if [ ! -e $PWD/all_old.list ]
then
	echo "Creating old file list"
	build_file_list old $PWD/all_old.list
fi
if [ ! -e $PWD/all_new.list ]
then
	echo "Creating new file list"
	build_file_list new $PWD/all_new.list
fi

# Compare the two versions & log differences
if [ $keep_state == false ]
then
	rm removed.list added.list changed.list 2>>/dev/null
fi

# Read the list of files into two arrays
declare -a old_files new_files

if [[ ! -e removed.list || ! -e added.list || ! -e changed.list ]]
then
	slurp_file_list all_old.list old_files
	slurp_file_list all_new.list new_files

	if [ $debug == true ]
	then
		echo "old="${#old_files[@]}
		echo "new="${#new_files[@]}
	fi
fi

# Start with files that have been removed...
if [ ! -e removed.list ]
then
	echo "Checking for files that have been removed..."

	index=0
	count=${#old_files[@]}

	let opc=$count/100

	while [ $index -lt $count ]
	do
		f=${old_files[$index]}
		if [ ! -e "new/$f" ]
		then
			# Special case where we must guard against removing libc before the upgrade is complete
			if [[ "$f" =~ libc(-[0-9]\.[0-9]\.so*|.so.[0-9]) || -d "old/$f" ]]
			then
				echo "$f" >> post-upgrade.list
			else
				echo "$f" >> removed.list
			fi
		fi
		let index=$index+1

		let pc=$index/$opc
		printf "\r%2d%%" $pc
	done
	printf "\n\n"
fi

# ...files that have been added...
if [ ! -e added.list ]
then
	echo "Checking for files that have been added..."

	index=0
	count=${#new_files[@]}

	let opc=$count/100

	while [ $index -lt $count ]
	do
		f=${new_files[$index]}
		if [ ! -e "old/$f" ]; then echo "$f" >> added.list;fi
		let index=$index+1

		let pc=$index/$opc
		printf "\r%2d%%" $pc
	done
	printf "\n\n"
fi

# ...files that have changed
if [ ! -e changed.list ]
then
	echo "Checking for files that have changed..."

	index=0
	count=${#new_files[@]}

	let opc=$count/100

	while [ $index -lt $count ]
	do
		f=${new_files[$index]}
		if [[ -e "old/$f" && -e "new/$f" && -f "new/$f" ]]
		then
			cmp "old/$f" "new/$f" >>/dev/null
			if [ $? -gt 0 ];then echo "$f" >> changed.list;fi
		fi
		let index=$index+1

		let pc=$index/$opc
		printf "\r%2d%%" $pc
	done
	printf "\n\n"
fi

# This is a lot of work just to provide some simple information to the user
echo `wc -l removed.list | cut -d ' ' -f 1`" files removed, "`wc -l changed.list | cut -d ' ' -f 1`" files changed and "`wc -l added.list | cut -d ' ' -f 1`" files added."

# Now we have the lists of files we need for the upgrade, build an upgrade overlay tree
if [[ $keep_state == false && -e upgrade_tree ]];then rm -rf upgrade_tree;fi

cwd=$PWD

if [ ! -e upgrade_tree ]
then
	mkdir upgrade_tree 2>>/dev/null

	declare -a upgrade_files
	cd new

	for l in $cwd/changed.list $cwd/added.list
	do
		slurp_file_list $l upgrade_files

		index=0
		count=${#upgrade_files[@]}
		while [ $index -lt $count ]
		do
			f=${upgrade_files[$index]}
			# Skip directories because we'll end up copying the individual files within them anyway
			if [ ! -d "$f" ]
			then
				printf "\r\x15$f                         "
				cp -a --parents "$f" $cwd/upgrade_tree
			fi
			let index=$index+1
		done
	done
	printf "\n\n"
	cd $cwd
fi

if [[ $keep_state == false && -e upgrade.zip ]];then rm upgrade.zip;fi

if [ ! -e upgrade.zip ]
then
	# Create the raw upgrade archive
	echo "Creating upgrade archive"

	cd new
	zip -qyr0 $cwd/upgrade.zip *
	cd $cwd
fi

# Create the upgrade package
if [[ $keep_state == false && -e upgrade ]];then rm -rf upgrade;fi
mkdir upgrade 2>>/dev/null

# Always copy the latest files
for f in upgrade.zip removed.list post-upgrade.list
do
	if [ -e $f ];then cp -f $f upgrade;fi
done
gen_upgrade_script upgrade $old_version $new_version

if [ -e upgrade/post-upgrade.list ]
then
	gen_post_upgrade_script upgrade
fi

if [[ $final_archive == true ]]
then
	rm SyllableDesktop-$new_version-Upgrade.i586.7z 2>>/dev/null
	7z a SyllableDesktop-$new_version-Upgrade.i586.7z upgrade
fi

exit 0

# This is the user upgrade script which is extracted by the gen_upgrade_script()
# function in a *slightly* hackish fashion.
#
#1#!/bin/bash
#1
#1OLD=OLD_VERSION
#1NEW=NEW_VERSION
#1
#1echo "This will upgrade from Syllable V$OLD to V$NEW. It requires that you have"
#1echo "Syllable $OLD installed. You can not upgrade from any other versions. You"
#1echo "should run this script as the \"root\" user. Many system components will be"
#1echo "replaced and it will be necessarry to reboot your machine after the upgrade."
#1echo
#1read -p "Do you want to continue? [y/N]:" -e answer
#1
#1if [ "$answer" != "y" ]; then echo "Upgrade aborted"; exit 1; fi;
#1
#1if [ -e pre-upgrade.sh ]; then ./pre-upgrade.sh; fi
#1
#1echo
#1echo "Will now upgrade all system files. Do not interupt the script after this"
#1echo "point, as it will leave you with an inconsistent and possibly unusable"
#1echo "system."
#1echo
#1read -p "Hit enter to continue:" -e answer
#1
#1if [ -e removed.list ]; then
#1echo; read -p "Would you like to delete obsolete files? [y/N]:" -e answer
#1
#1	if [ "$answer" == "y" ]
#1	then
#1		for file in $( cat removed.list )
#1		do
#1			if [ -e "/boot/$file" ];then rm -Rv "/boot/$file"; fi
#1		done
#1	else
#1		echo "Leaving obsolete files.";
#1	fi
#1fi
#1
#1if [ -e unzip ]
#1then
#1	./unzip -K -o -d /boot/ upgrade.zip
#1else
#1	unzip -K -o -d /boot/ upgrade.zip
#1fi
#1sync
#1
#1echo
#1echo "All system files are now upgraded.  You must reboot your machine to complete"
#1echo "this upgrade."
#1
#1if [ -e post-upgrade.sh ]
#1then
#1	echo
#1	echo "You must run the script \"post-upgrade.sh\" after you have rebooted to"
#1	echo "complete the upgrade."
#1fi

# This is the user post-upgrade script which is extracted by the
# gen_post_upgrade_script() function in a *slightly* hackish fashion.
#
#2#!/bin/bash
#2
#2echo "Performing post-upgrade configuration."
#2
#2if [ -e post-upgrade.list ]
#2then
#2		for file in $( cat post-upgrade.list )
#2		do
#2			if [ -e "/boot/$file" ];then rm -Rv "/boot/$file"; fi
#2		done
#2fi
#2
#2echo "Upgrade of Syllable is complete."
#2exit 0
