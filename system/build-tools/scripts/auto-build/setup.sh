#!/bin/bash

BUILD_HOME=/home/build

function check_user()
{
	let RET=0

	if [ "$(id --user)" != 0 ]
	then
		echo 'This script must be run as the system ("root") user.'
		let RET=1
	fi

	if [ ! -e "$BUILD_HOME" ]
	then
		echo "The 'build' user does not exist (I expect the $BUILD_HOME directory to exist)"
		let RET=1
	fi

	return $RET
}

SOURCE_DIR="Source/Anonymous"

declare -a DIRS
DIRS=(	"Temp"							\
		"bin"							\
		"$SOURCE_DIR"					\
		"Build/Installer/installer"		\
		"Build/Installer/documentation"	\
		"Build/Net-Binaries"			\
		"Logs"							\
		"Packages"
)

function create_directories()
{
	# Make sure we're in the home directory
	cd $BUILD_HOME

	# Create each directory as required
	COUNT=${#DIRS[*]}
	INDEX=0

	while [ "$INDEX" -lt "$COUNT" ]
	do
		mkdir -p "${DIRS[$INDEX]}"
		let "INDEX = $INDEX + 1"
	done

	return 0
}

PACKS_URL=http://downloads.syllable.org/Syllable/i586/packs

declare -a PACKS
PACKS=(	"DevelopersDelight/DevelopersDelight-8.i586.zip"	\
		"ShellEssentials/ShellEssentials-1.i586.zip"	\
		"NetworkNecessities/NetworkNecessities-3.i586.zip"	\
		"PerlPit/PerlPit-6.i586.zip"
)

function install_packs()
{
	cd $BUILD_HOME/Temp

	# Download all of the packs first
	COUNT=${#PACKS[*]}
	INDEX=0

	while [ "$INDEX" -lt "$COUNT" ]
	do
		PACK="${PACKS[$INDEX]}"
		FILE="`basename $PACK`"

		if [ ! -e "$FILE" ]
		then
			echo "Downloading $FILE"
			curl "$PACKS_URL/$PACK" > $FILE
		fi

		let "INDEX = $INDEX + 1"
	done

	# Now install them
	COUNT=${#PACKS[*]}
	INDEX=0

	while [ "$INDEX" -lt "$COUNT" ]
	do
		cd $BUILD_HOME/Temp

		PACK="`basename ${PACKS[$INDEX]}`"
		if [ "`echo $PACK | grep .zip`" == "$PACK" ]
		then
			# Unzip
			unzip "$PACK"
		else
			# Un-7zip
			7z x "$PACK"
		fi

		# Run the install.sh
		DIR="`dirname ${PACKS[$INDEX]}`"
		cd $DIR
		# This is interactive but `yes` is buggy on Desktop
		./install.sh

		# Cleanup
		cd ..
		rm -r $DIR $PACK

		let "INDEX = $INDEX + 1"
	done

	# Update Builder
	build update
}

RESOURCES_URL=http://downloads.syllable.org/Syllable/i586/resources

declare -a RESOURCES
RESOURCES=(	"GNU-CompilerCollection/gcc-3.4.3-3.i586.resource"
)

function install_resources()
{
	cd $BUILD_HOME/Packages

	# Download all of the resources first
	COUNT=${#RESOURCES[*]}
	INDEX=0

	while [ "$INDEX" -lt "$COUNT" ]
	do
		RESOURCE="${RESOURCES[$INDEX]}"
		FILE="`basename $RESOURCE`"

		if [ ! -e "$FILE" ]
		then
			echo "Downloading $FILE"
			curl "$RESOURCES_URL/$RESOURCE" > $FILE
		fi

		let "INDEX = $INDEX + 1"
	done

	# Now install them
	COUNT=${#RESOURCES[*]}
	INDEX=0

	while [ "$INDEX" -lt "$COUNT" ]
	do
		cd $BUILD_HOME/Packages

		RESOURCE="${RESOURCES[$INDEX]}"
		FILE="`basename $RESOURCE`"
		if [ "`echo $RESOURCE | grep gcc`" != "$RESOURCE" ]
		then
			build install $FILE
			if [ $? == 0 ]
			then
				rm $FILE
			fi
		fi

		let "INDEX = $INDEX + 1"
	done
}

function checkout_source()
{
	# Make sure we're in the right place before we spray a CVS repository everywhere
	cd "$BUILD_HOME/$SOURCE_DIR"

	let RET=$?
	if [ $RET -eq 0 ]
	then
		cvs -z9 -d:pserver:anonymous@syllable.cvs.sourceforge.net:/cvsroot/syllable co syllable
		let RET=$?
	fi

	return $RET
}

function download_sources()
{
	# Try to get all the third party source files required for a build
	build download system
	build download base
}

function install_files()
{
	cd "$BUILD_HOME"

	cp -a $SOURCE_DIR/syllable/system/build-tools/scripts/auto-build/*.sh bin/
	chmod 755 bin/*.sh

	cp -a $SOURCE_DIR/syllable/system/build-tools/scripts/install/* Build/Installer/installer/
	cp -a $SOURCE_DIR/syllable/system/doc/install/* Build/Installer/documentation/
}

# Script entry point

check_user
if [ $? != 0 ]
then
	exit 1
fi

# Setup everything under the build user
cd $BUILD_HOME

create_directories

install_packs
install_resources

checkout_source
if [ $? != 0 ]
then
	echo "Failed to checkout a copy of the source repository"
fi

download_sources

install_files

echo "Please wait while I finalise the configuration..."
chown -R build:users /home/build

echo "Automatic configuration is complete."
echo
echo "You must provide the network applications in Build/Net-Binaries and"
echo "a copy of the Ruby 1.8.6 resource package in the Build/Installer"
echo "directory."
echo
echo "You may also wish to set the FTP_SERVER, FTP_USER and FTP_PASS"
echo "environment variables in the build users profile"

exit 0
