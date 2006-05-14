#! ruby

# 26 March 2005, Kaj de Vos
#   Ask confirmation for both recognized and unrecognized partitions.
#   Removed invalid hardcoded paths.
# Last modified by xsdg for installer version 0.1.11.6.

require "helpers.rb"
require "lsfs.rb"
require "grub-gen.rb"

# Remember to always update the version numbers below!

Base = ["0.6.1", "base-syllable-%s.zip"]
Packages = {
	"ABrowse"	=> ["0.4a", "abrowse-%s.bin.zip"],
	"Whisper"	=> ["1.0a", "whisper-%s.bin.zip"],
	"Chat"		=> ["0.2.0", "chat-%s.bin.zip"]
}

# Now on with the installation
read_to_user "partition.txt"

print "Do you need to create a new partition? (Y/n) "
resp = $stdin.getuc

if(resp == "\r" or resp.downcase == "y")
	# run DiskManager
	system "/bin/DiskManager"
end


read_to_user "format.txt"

valid_partitions, shortmap = LSFS::list

loop {
	puts "\nEnter the letter or full path of the partition that you would like to install Syllable onto,"
	puts 'or "ls" to see the partition list again'
	print '(For example: b or /dev/disk/ata/hda/1): '
	
	resp = $stdin.getsc
	resp = shortmap[resp] if shortmap.include? resp
	
	if resp and valid_partitions[resp]
		# FIXME: ugly, ugly, ugly...
		$part = resp
		info = valid_partitions[$part]
		
		unless info[0] == "unknown"
			# Overwriting a recognized partition
			puts "WARNING: formatting #{$part} will destroy the #{info[1]} #{info[0]} partition that already exists here."
		else
			# Overwriting an unrecognized partition
			puts "WARNING: formatting #{$part} will destroy the unknown partition that already exists here."
		end

		puts 'Are you sure? Type "yes" to continue:'
		resp = $stdin.getsc
		break if resp == "yes"
		
		puts "User entered #{resp.inspect} instead of \"yes\"; returning to partition selection."
	else
		case resp
		when "ls", "list"
			LSFS::list([valid_partitions, shortmap])
		when /^q(uit)?$/, "x", "exit"
			# time to leave
			exit 1
		else
			puts "User entered #{resp.inspect}, which is not a valid partition; returning to partition selection.  Type \"ls\" if you wish to see the list of valid partitions again."
		end
	end
}

print "Formatting #{$part} as afs... "
$stderr.flush
retval = system "/bin/format", $part, "afs", "Syllable"
if(retval)
	# success
	puts "done.\n\n"
else
	# whoops!
	puts
	puts "ERROR: Failed to format #{$part}.  Stopping"
	# take care of this in the caller
	exit 2
end

read_to_user "installation.txt"

puts "\nPlease wait...\n\n"

Dir.mkdir "/inst"
system "/bin/mount", "-t", "afs", $part, "/inst"

pkg = sprintf(Base[1], Base[0])
retval = system "unzip", "-d", "/inst/", "/boot/Packages/base/#{pkg}"
system "sync"

if(retval)
	print "The base package has been installed.  Please press any key to continue."
	$stdin.getuc
else
	puts "\nFailed to extract base package to #{$part}.  Stopping."
	exit 3
end

print "Autogenerating GRUB menu.lst ... "
GrubGen::save(valid_partitions, $part, "/inst/boot/grub/menu.lst")
puts "done.\n"

loop {
	print "\n" + 'Press "l" to view the menu.lst file, "e" to edit it manually, "?" for an explanation of the menu.lst file, or any other key to continue with the installation: '
	resp = $stdin.getuc

	case resp.downcase
	when "l"
		read_to_user "/inst/boot/grub/menu.lst", true
	
	when "e"
		system "/bin/aedit", "/inst/boot/grub/menu.lst"
	
	when "?"
		read_to_user "configure.txt"
	else
		break
	end
}


# Install Syllable-Net packages
Packages.sort_by{|name, info| name}.each {|pkgname, info|
	puts "Installing #{pkgname}"
	pkg = sprintf(info[1], info[0])
	system "unzip", "/boot/Packages/net/#{pkg}", "-d", "/inst/atheos/Applications/"
}

# Sync disks
#puts "All files have been installed. Synchronising disks.  Please wait, as this may take some time"
#puts "WARNING! If you reboot or turn off your computer before Syllable has properly syncronised the disks, the installation WILL BE CORRUPT and you will not be able to boot your Syllable installation!  DO NOT INTERRUPT THE INSTALLATION AT THIS POINT!"

system "sync"

system "clear"
puts 'Syllable has now been installed!'
puts 'Please press "r" to reboot your computer. (Remember to install Grub.)'
puts 'Press any other key to exit this installation script.'
resp = $stdin.getuc

system "/bin/reboot" if resp.downcase == "r"

exec "/bin/bash", "--login"

