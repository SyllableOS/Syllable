#!/bin/bash

working_dir=/Syllable

image_size=2G
partition=partition_$image_size.raw
raw_disc=disc_$image_size.raw
vmdk_disc=disc_$image_size.vmdk

# Extract the MBR & boot block from a raw disc image
# This is not used when building a disc image.
function extract_mbr()
{
	dd if=$1 of=mbr_$image_size.raw bs=512 count=63
}

# Extract the partition from a raw disc image
# This is not used when building a disc image.
function extract_partition()
{
	dd if=$1 of=partition_$image_size.raw bs=512 skip=63
}

# Create an empty raw image file
#
# $1 : Name of the image file to create
function create_partition()
{
	if [ -e $1 ]
	then
		rm $1
	fi
	echo "Creating empty $image_size disc image"
	qemu-img create -f raw $1 $image_size
	format $1 afs Syllable
	sync
}

# Concatanate the raw partition image onto the header to create a bootable raw disc image
#
# $1 : Raw header file
# $2 : Raw partition image
# $3 : Output disc image
function create_disc()
{
	echo "Combining partition and MBR"
	cp $1 $3
	cat $2 >> $3
	sync
}

# Convert a raw disc to a VMWare VMDK file
#
# $1 : Raw disc image
# $2 : Output disc image
function convert_to_vmdk()
{
	echo "Converting $1 to $2"
	qemu-img convert -f raw $1 -O vmdk $2
	sync
}

# Write a GRUB menu.lst suitable for the VM
#
# $1 : Path to the (mounted) file-system to write the menu.lst into
function setup_grub()
{
	file=$1/boot/grub/menu.lst
	if [ -e $file ]
	then
		rm $file
	fi

	# Emit a working menu.lst
	printf "\tcolor cyan/blue white/blue\n" >> $file
	printf "\ttimeout	10\n" >> $file
	printf "\n" >> $file
	printf "\ttitle	Start Syllable\n" >> $file
	printf "\troot	(hd0,0)\n" >> $file
	printf "\tkernel	/system/kernel.so root=/dev/disk/ata/hda/0 uspace_end=0x7fffffff enable_ata_dma=false\n" >> $file
	printf "\tmodule	/system/config/kernel.cfg\n" >> $file
	printf "\tmodule	/system/drivers/dev/bus/acpi\n" >> $file
	printf "\tmodule	/system/drivers/dev/bus/pci\n" >> $file
	printf "\tmodule	/system/drivers/dev/bus/ata\n" >> $file
	printf "\tmodule	/system/drivers/dev/hcd/ata_pci\n" >> $file
	printf "\tmodule	/system/drivers/fs/afs\n" >> $file
}

# Install Syllable onto the raw disc image
#
# $1 : Raw partition file
# $2 : base-syllable.zip to extract
function install()
{
	if [ -e syllable ]
	then
		rmdir syllable
	fi
	mkdir syllable

	echo "Installing $2"
	mount -d $1 syllable
	unzip $2 -d syllable
	sleep 5
	sync
	setup_grub syllable

	unmount syllable
	rmdir syllable
}

# Script entry point

cd $working_dir

create_partition $partition
install $partition base-syllable.zip
create_disc mbr_$image_size.raw $partition $raw_disc
convert_to_vmdk $raw_disc $vmdk_disc
rm $raw_disc
