#! ruby

# last modified for installer version 0.1.11.6

require "helpers.rb"
require "lsfs.rb"
require "grub-gen.rb"

#
# Remember to always update the version numbers below!
Base = ["0.5.4", "base-syllable-%s.zip"]
Packages = {
	"ABrowse"	=> ["0.3.4", "abrowse-%s.bin.tar.gz"],
	"Whisper"	=> ["0.2.0", "whisper-%s.bin.tar.gz"],
	"Chat"		=> ["0.2.0", "chat-%s.bin.tar.gz"]
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
	puts "\nEnter the name of the device that you would like to install Syllable onto, or \"ls\" to see the partition list again"
	print "(E.g. /dev/disk/ata/hda/1): "
	
	resp = $stdin.getsc
	resp = shortmap[resp] if shortmap.include? resp
	
	if(not resp.nil? and valid_partitions[resp])
		# FIXME: ugly, ugly, ugly...
		$part = resp
		info = valid_partitions[$part]
		
		if(info[0] != "unknown")
			# overwriting a recognized partition; give a warning
			puts "WARNING: formatting #{$part} will destroy the #{info[1]} #{info[0]} partition that already exists here.  Are you sure?  Type \"yes\" to continue: "
			resp = $stdin.getsc
			break if resp == "yes"
			
			puts "User entered #{resp.inspect} instead of \"yes\"; returning to partition selection."
		else
			break
		end
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
retval = system "/usr/bin/unzip", "-d", "/inst/", "/boot/Packages/base/#{pkg}"
system "/usr/bin/sync"
system "/usr/bin/sync"

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
Packages.sort_by{|name, info| name}.each {
	|pkgname, info|
	puts "Installing #{pkgname}"
	pkg = sprintf(info[1], info[0])
	system "/usr/bin/tar", "-xzpf", "/boot/Packages/net/#{pkg}", "-C", "/inst/atheos/Applications/"
	}

# Sync disks
#puts "All files have been installed. Synchronising disks.  Please wait, as this may take some time"
#puts "WARNING! If you reboot or turn off your computer before Syllable has properly syncronised the disks, the installation WILL BE CORRUPT and you will not be able to boot your Syllable installation!  DO NOT INTERRUPT THE INSTALLATION AT THIS POINT!"

system "/usr/bin/sync"
system "/usr/bin/sync"

system "/usr/bin/clear"
puts 'Syllable has now been installed!  Please press "r" to reboot your computer (Remember to install Grub).  Press any other key to exit this installation script.'
resp = $stdin.getuc

system "/bin/reboot" if resp.downcase == "r"

exec "/bin/bash", "--login"


