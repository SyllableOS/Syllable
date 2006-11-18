#!/bin/bash

declare -a FILES

# source, dest
FILES=(	"/atheos/etc/profile, /atheos/etc/profile" 															\
		"/atheos/etc/passwd, /atheos/etc/passwd"															\
		"/atheos/etc/termcap, /atheos/etc/termcap"															\
																											\
		"/atheos/home/root/.profile, /atheos/home/root/.profile"											\
																											\
		"/atheos/sys/drivers/appserver/input/*, /atheos/sys/drivers/appserver/input/."						\
		"/atheos/sys/drivers/appserver/video/*, /atheos/sys/drivers/appserver/video/."						\
		"/atheos/sys/drivers/appserver/decorators/Photon, /atheos/sys/drivers/appserver/decorators/Photon"	\
																											\
		"/atheos/sys/drivers/dev/ps2, /atheos/sys/drivers/dev/ps2"											\
		"/atheos/sys/drivers/dev/disk/bios, /atheos/sys/drivers/dev/disk/bios"								\
		"/atheos/sys/drivers/dev/disk/usb, /atheos/sys/drivers/dev/disk/usb"								\
		"/atheos/sys/drivers/dev/misc/serial, /atheos/sys/drivers/dev/misc/serial"							\
		"/atheos/sys/drivers/dev/graphics/*, /atheos/sys/drivers/dev/graphics/."							\
																											\
		"/atheos/sys/drivers/dev/bus/pci, /atheos/sys/drivers/dev/bus/pci"									\
		"/atheos/sys/drivers/dev/bus/ata, /atheos/sys/drivers/dev/bus/ata"									\
		"/atheos/sys/drivers/dev/bus/usb, /atheos/sys/drivers/dev/bus/usb"									\
		"/atheos/sys/drivers/dev/bus/scsi, /atheos/sys/drivers/dev/bus/scsi"								\
																											\
		"/atheos/sys/drivers/dev/hcd/ata_pci, /atheos/sys/drivers/dev/hcd/ata_pci"							\
		"/atheos/sys/drivers/dev/hcd/usb_ehci, /atheos/sys/drivers/dev/hcd/usb_ehci"						\
		"/atheos/sys/drivers/dev/hcd/usb_uhci, /atheos/sys/drivers/dev/hcd/usb_uhci"						\
		"/atheos/sys/drivers/dev/hcd/usb_ohci, /atheos/sys/drivers/dev/hcd/usb_ohci"						\
																											\
		"/atheos/sys/drivers/dev/input/usb_hid, /atheos/sys/drivers/dev/input/usb_hid"						\
																											\
		"/atheos/sys/drivers/fs/iso9660, /atheos/sys/drivers/fs/iso9660"									\
		"/atheos/sys/drivers/fs/afs, /atheos/sys/drivers/fs/afs"											\
		"/atheos/sys/drivers/fs/fatfs, /atheos/sys/drivers/fs/fatfs"										\
		"/atheos/sys/drivers/fs/ext2, /atheos/sys/drivers/fs/ext2"											\
		"/atheos/sys/drivers/fs/ntfs, /atheos/sys/drivers/fs/ntfs"											\
																											\
		"/atheos/sys/kernel.so, /atheos/sys/kernel.so"														\
																											\
		"/atheos/sys/libs/*, /atheos/sys/libs/."															\
																											\
		"/atheos/sys/appserver, /atheos/sys/appserver"														\
																											\
		"/atheos/sys/config/appserver, /atheos/sys/config/appserver"										\
																											\
		"/atheos/sys/extensions/translators/pngtrans.so, /atheos/sys/extensions/translators/pngtrans.so"	\
																											\
		"/atheos/sys/fonts/DejaVuSans.ttf, /atheos/sys/fonts/DejaVuSans.ttf"								\
		"/atheos/sys/fonts/DejaVuSansMono.ttf, /atheos/sys/fonts/DejaVuSansMono.ttf"						\
		"/atheos/sys/fonts/DejaVuSansCondensed.ttf, /atheos/sys/fonts/DejaVuSansCondensed.ttf"				\
																											\
		"/atheos/sys/keymaps/American, /atheos/sys/keymaps/American"										\
																											\
		"/atheos/sys/bin/DiskManager, /atheos/sys/bin/DiskManager"											\
		"/atheos/sys/bin/aterm, /atheos/sys/bin/aterm"														\
		"/atheos/sys/bin/dbterm, /atheos/sys/bin/dbterm"													\
		"/atheos/sys/bin/devstat, /atheos/sys/bin/devstat"													\
		"/atheos/sys/bin/format, /atheos/sys/bin/format"													\
		"/atheos/sys/bin/fsprobe, /atheos/sys/bin/fsprobe"													\
		"/atheos/sys/bin/init, /atheos/sys/bin/init"														\
		"/atheos/sys/bin/mount, /atheos/sys/bin/mount"														\
		"/atheos/sys/bin/reboot, /atheos/sys/bin/reboot"													\
		"/atheos/sys/bin/unmount, /atheos/sys/bin/unmount"													\
																											\
		"/atheos/Applications/AEdit/AEdit, /atheos/sys/bin/aedit"											\
																											\
		"/atheos/usr/coreutils/bin/cat, /atheos/usr/local/bin/cat"											\
		"/atheos/usr/coreutils/bin/cp, /atheos/usr/local/bin/cp"											\
		"/atheos/usr/coreutils/bin/cut, /atheos/usr/local/bin/cut"											\
		"/atheos/usr/coreutils/bin/dd, /atheos/usr/local/bin/dd"											\
		"/atheos/usr/coreutils/bin/echo, /atheos/usr/local/bin/echo"										\
		"/atheos/usr/coreutils/bin/hostname, /atheos/usr/local/bin/hostname"								\
		"/atheos/usr/coreutils/bin/ln, /atheos/usr/local/bin/ln"											\
		"/atheos/usr/coreutils/bin/ls, /atheos/usr/local/bin/ls"											\
		"/atheos/usr/coreutils/bin/md5sum, /atheos/usr/local/bin/md5sum"									\
		"/atheos/usr/coreutils/bin/mkdir, /atheos/usr/local/bin/mkdir"										\
		"/atheos/usr/coreutils/bin/mv, /atheos/usr/local/bin/mv"											\
		"/atheos/usr/coreutils/bin/printf, /atheos/usr/local/bin/printf"									\
		"/atheos/usr/coreutils/bin/pwd, /atheos/usr/local/bin/pwd"											\
		"/atheos/usr/coreutils/bin/rm, /atheos/usr/local/bin/rm"											\
		"/atheos/usr/coreutils/bin/rmdir, /atheos/usr/local/bin/rmdir"										\
		"/atheos/usr/coreutils/bin/sleep, /atheos/usr/local/bin/sleep"										\
		"/atheos/usr/coreutils/bin/sync, /atheos/usr/local/bin/sync"										\
		"/atheos/usr/coreutils/bin/whoami, /atheos/usr/local/bin/whoami"									\
		"/atheos/usr/coreutils/bin/uname, /atheos/usr/local/bin/uname"										\
		"/atheos/usr/coreutils/bin/stty, /atheos/usr/local/bin/stty"										\
																											\
		"/atheos/usr/findutils/bin/find, /atheos/usr/local/bin/find"										\
		"/atheos/usr/gzip/bin/gzip, /atheos/usr/local/bin/gzip"												\
		"/atheos/usr/sed/bin/sed, /atheos/usr/local/bin/sed"												\
		"/atheos/usr/tar/bin/tar, /atheos/usr/local/bin/tar"												\
		"/atheos/usr/unzip/bin/unzip, /atheos/usr/local/bin/unzip"											\
		"/atheos/usr/ncurses/bin/clear, /atheos/usr/local/bin/clear"										\
		"/atheos/usr/less/bin/less, /atheos/usr/local/bin/less"												\
																											\
		"/atheos/usr/bash/bin/bash, /atheos/usr/local/bin/bash"												\
		"/atheos/usr/grep/bin/grep, /atheos/usr/local/bin/grep"												\
																											\
		"/atheos/usr/share/terminfo/r/rxvt-16color, /atheos/usr/share/terminfo/r/rxvt-16color"				\
		"/atheos/usr/share/terminfo/x/xterm, /atheos/usr/share/terminfo/x/xterm"							\
																											\
		"/atheos/usr/grub, /atheos/usr/grub"																\
																											\
		"/atheos/usr/grub/lib/grub/i386-pc/stage2_eltorito, /boot/grub/stage2_eltorito"											\
		"/atheos/usr/grub/lib/grub/i386-pc/stage1, /boot/grub/stage1"																\
		"/atheos/usr/grub/lib/grub/i386-pc/afs_stage1_5, /boot/grub/afs_stage1_5"													\
		"/atheos/usr/grub/lib/grub/i386-pc/stage2, /boot/grub/stage2"																\
)

declare -a LINKS

# target, link
LINKS=(	"/usr/local/bin/gzip, /atheos/usr/local/bin/gunzip"	\
		"/usr/local/bin/bash, /atheos/sys/bin/bash"			\
		"/usr/local/bin/bash, /atheos/sys/bin/sh"			\
		"/usr/local/bin/pwd, /atheos/sys/bin/pwd"			\
		"/usr/ruby/bin/ruby, /atheos/usr/local/bin/ruby"	\
)

declare -a ENV

ENV=(	"PATH=/usr/local/bin:/usr/grub/bin:/usr/grub/sbin:/boot/atheos/sys/bin"		\
		"DLL_PATH=@bindir@/lib:./:/boot/atheos/sys/libs:/boot/atheos/sys"			\
		"TEMP=/tmp"																	\
		"SYSTEM=Syllable"															\
		"COLORTERM=rxvt-16color"													\
		"TERMINFO=/usr/share/terminfo"												\
		"PAGER=less"																\
		"LESSCHARSET=latin1"														\
		"EDITOR=aedit"																\
		"SHELL=bash"																\
		"HOME=/home/root"															\
)

declare -a GRUB

GRUB=(	"color	cyan/blue white/blue"																		\
		"timeout	10"																						\
		""																									\
		"title	Install Syllable"																			\
		"kernel /atheos/sys/kernel.so rootfs=iso9660 root=@boot disable_config=true uspace_end=0xf7ffffff"	\
		"module /atheos/sys/drivers/dev/bus/pci"															\
		"module /atheos/sys/drivers/dev/bus/ata"															\
		"module /atheos/sys/drivers/dev/hcd/ata_pci"														\
		"module /atheos/sys/drivers/fs/iso9660"																\
		""																									\
		"title	Install Syllable from a USB CD-ROM drive"													\
		"kernel /atheos/sys/kernel.so rootfs=iso9660 root=@boot disable_config=true uspace_end=0xf7ffffff"	\
		"module /atheos/sys/drivers/dev/bus/pci"															\
		"module /atheos/sys/drivers/dev/bus/ata"															\
		"module /atheos/sys/drivers/dev/bus/usb"															\
		"module /atheos/sys/drivers/dev/bus/scsi"															\
		"module /atheos/sys/drivers/dev/hcd/ata_pci"														\
		"module /atheos/sys/drivers/dev/hcd/usb_ohci"														\
		"module /atheos/sys/drivers/dev/hcd/usb_uhci"														\
		"module /atheos/sys/drivers/dev/hcd/usb_ehci"														\
		"module /atheos/sys/drivers/dev/disk/usb"															\
		"module /atheos/sys/drivers/fs/iso9660"																\
)

WORKING_DIR=files
BASE_DIR=$WORKING_DIR/base
CD_DIR=$WORKING_DIR/cdrom

# Default for the Ruby package if one is not specified
RUBY_PACKAGE=ruby-1.8.4.bin.1.zip

# Default for the Installer scripts if a directory is not specified
INSTALLER_DIR=installer
DOCUMENTATION_DIR=documentation

# function print_usage
#
# Prints usage information.  This function will cause the script
# to exit with a return code of 1
#
# No arguments

function print_usage()
{
  echo "USAGE : build-cd [BASE FILE] [NET DIRECTORY] [VERSION] ([RUBY PACKAGE]) ([INSTALLER DIR])"
  exit 1
}

# function exit_with_usage
#
# Called by other functions when an abnormal exit is required.  Prints
# an error and calls print_usage
#
# $1 error code
# $2..$n extra information.  Error code specific

function exit_with_usage()
{
  echo -n "Error: "

  case $1 in

    "1" )
      # File does not exist
      echo "The file \"$2\" does not exist."
      ;;

    "2" )
      # Directory does not exist
      echo "The directory \"$2\" does not exist."
      ;;

  esac

  echo
  print_usage
}

# function initialiase
#
# Perform basic checks, create a working directory if it doesn't
# exist and sanity check the arguments.
#
# $1 - path to base-syllable file
# $2 - path to the Syllable-net directory
# $3 - version number
# $4 - (Optional) path to the Ruby binary package
# $5 - (Optional) path to the installer script directory

function initialise()
{
  if [[ -z $1 || -z $2 || -z $3 ]]
  then
    print_usage
  fi

  # Check that the base-syllable file & Syllable-net directory exist
  if [ ! -e $1 ]
  then
    exit_with_usage 1 $1
  fi
  if [ ! -e $2 ]
  then
    exit_with_usage 2 $2
  fi

  # If the user specified a Ruby binary package, use it
  if [ ! -z $4 ]
  then
    RUBY_PACKAGE=$4
  fi
  printf "Using Ruby package %s\n" $RUBY_PACKAGE
  if [ ! -e $RUBY_PACKAGE ]
  then
    exit_with_usage 1 $RUBY_PACKAGE
  fi

  # If the user specified a path to the installer scripts, use it
  if [ ! -z $5 ]
  then
    INSTALLER_DIR=$5
  fi
  printf "Using Installer scripts from %s\n" $INSTALLER_DIR
  if [ ! -e $INSTALLER_DIR ]
  then
    exit_with_usage 2 $INSTALLER_DIR
  fi

  # We'll need space to work in
  if [ ! -e $WORKING_DIR ]
  then
    mkdir $WORKING_DIR
    mkdir $BASE_DIR
    mkdir $CD_DIR

    # Unzip the base-syllable file into $BASE_DIR
    unzip $1 -d $BASE_DIR
  elif [ -e $CD_DIR ]
  then
    printf "Removing %s\n" "$CD_DIR"
    rm -rf $CD_DIR
    mkdir -p $CD_DIR
  fi
  printf "\n"

  sync
}

# function generate_init_script
#
# Emit the init.sh script suitable for booting from the CD
#
# No arguments

function generate_init_script()
{
  printf "#!/bin/sh\n\n" >> $CD_DIR/atheos/sys/init.sh

  # Set environment variables
  COUNT=${#ENV[*]}
  INDEX=0

  while [ "$INDEX" -lt "$COUNT" ]
  do
    printf "export %s\n" "${ENV[$INDEX]}" >> $CD_DIR/atheos/sys/init.sh
    let "INDEX = $INDEX + 1"
  done

  printf "\naterm /usr/ruby/bin/ruby /boot/Install/install.rb &\n" >> $CD_DIR/atheos/sys/init.sh

  # Enable this to get an extra shell when testing
  # printf "aterm &\n" >> $CD_DIR/atheos/sys/init.sh

  chmod +x $CD_DIR/atheos/sys/init.sh
}

# function generate_grub_script
#
# Emit the GRUB menu.lst suitable for booting from the CD
#
# No arguments

function generate_grub_script()
{
  COUNT=${#GRUB[*]}
  INDEX=0

  while [ "$INDEX" -lt "$COUNT" ]
  do
    printf "%s\n" "${GRUB[$INDEX]}" >> $CD_DIR/boot/grub/menu.lst
    let "INDEX = $INDEX + 1"
  done
}

# function copy_files
#
# Copy the files listed in FILES from $BASE_DIR to their
# destination in $CD_DIR
#
# No arguments

function copy_files()
{
  # Copy files from base
  COUNT=${#FILES[*]}
  INDEX=0

  while [ "$INDEX" -lt "$COUNT" ]
  do
    SOURCE=$BASE_DIR/`echo ${FILES[$INDEX]} | cut -d ',' -f 1 | tr -d ' '`
    DEST=$CD_DIR/`echo ${FILES[$INDEX]} | cut -d ',' -f 2 | tr -d ' '`

    printf "Copy \"%s\" to \"%s\"\n" "$SOURCE" "$DEST"

    # Ensure the destination directory exists
    DEST_DIR=`dirname "$DEST"`
    if [ ! -e "$DEST_DIR" ]
    then
      mkdir -p "$DEST_DIR"
    fi

    # Do not quote $SOURCE; we want globing to work here
    cp -a $SOURCE $DEST

    let "INDEX = $INDEX + 1"
  done
  sync

  # Install Ruby (Must be done before links)
  unzip $RUBY_PACKAGE -d $CD_DIR/atheos/usr/

  # Create links
  COUNT=${#LINKS[*]}
  INDEX=0

  while [ "$INDEX" -lt "$COUNT" ]
  do
    TARGET=`echo ${LINKS[$INDEX]} | cut -d ',' -f 1 | tr -d ' '`
    LINK_NAME=$CD_DIR/`echo ${LINKS[$INDEX]} | cut -d ',' -f 2 | tr -d ' '`

    printf "Link \"%s\" to \"%s\"\n" "$LINK_NAME" "$TARGET"

    # Ensure the destination directory exists
    DEST_DIR=`dirname "$LINK_NAME"`
    if [ ! -e "$DEST_DIR" ]
    then
      mkdir -p "$DEST_DIR"
    fi

    ln -s $TARGET $LINK_NAME

    let "INDEX = $INDEX + 1"
  done
  sync

  # Init script
  generate_init_script

  # GRUB
  generate_grub_script
}

# function copy_packages
#
# Copy the installation package files to the CD
#
# $1 - path to base-syllable file
# $2 - path to Sylable-net directory

function copy_packages()
{
  mkdir -p $CD_DIR/Packages/base
  printf "Copying base file: %s\n" "$1"
  cp $1 $CD_DIR/Packages/base/base-syllable.zip

  mkdir -p $CD_DIR/Packages/net
  printf "Copying Syllable-net files: %s\n" "$2"
  cp -a $2/* $CD_DIR/Packages/net/
}

# function copy_installer
#
# Copy the installation scripts to the CD
#
# $1 - version number

function copy_installer()
{
  mkdir -p $CD_DIR/Install
  printf "Copying installation scripts: %s\n" "$INSTALLER_DIR"
  cp -a $INSTALLER_DIR/* $CD_DIR/Install/

  chmod +x $CD_DIR/Install/*.rb

  # Process the welcome.txt file
  sed -e "s/VER/$1/g" $CD_DIR/Install/doc/welcome-template.txt > $CD_DIR/Install/doc/welcome.txt
  rm $CD_DIR/Install/doc/welcome-template.txt

  # Put the install.txt file where the user can find it
  cp -f $DOCUMENTATION_DIR/install.txt $CD_DIR/INSTALL.TXT

  sync
}

# function create_iso
# 
# Create an ISO CD image from the CD directory
# 
# $1 - version number

function create_iso()
{
  NAME="Syllable $1"
  ISO="syllable.iso"

  # Do not mess with the cdrecord options unless you know exactly what you are doing.  Getting
  # the options wrong will mean the CD will be unbootable (And likely unreadable)
  mkisofs -iso-level 3 --allow-leading-dots -R -V "$NAME" -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o "$ISO" "$CD_DIR"
  sync
}

# Entry point
# $1 - path to base-syllable file
# $2 - path to the Syllable-net directory
# $3 - version number
# $4 - (Optional) path to the Ruby binary package
# $5 - (Optional) path to the installer script directory

_BASE=$1
_NET=$2
_VER=$3
_RUBY=$4
_INSTALLER=$5

initialise $_BASE $_NET $_VER $_RUBY $_INSTALLER

# Create the basic bootable system
copy_files

# Copy the installation files
copy_packages $_BASE $_NET
copy_installer $_VER

# Create the ISO
create_iso $_VER

exit 0

