#! ruby

# last modified for installer version 0.1.11.6

INSTALL_DIR="/boot/Install"

if(not FileTest.exist?(INSTALL_DIR))
	puts "This script must be run from the installation CD."
	$stdout.flush
	exit 1
end

Dir.chdir INSTALL_DIR

require 'helpers.rb'

system "clear"
system "cat doc/welcome.txt"

print "Press 'i' or 'I' to install now, or any other key to cancel the installation: "
choice = $stdin.getuc

if(choice.downcase == 'i')	# choice should never be nil, but better safe than sorry
	# do installation
	retval = nil
	begin
		retval = system "./do_install.rb"
		
		rescue SystemCallError
	end
	
	if(retval)
		# installation was successful; yield to a shell
		system "sync"		# make sure the data's stored before the user reboots
		puts "Syllable has now been installed!  Press Ctrl+Alt+Del to restart your computer.\n\n"
		$stdout.flush
		exec "/bin/bash", "--login"
	else
		# installation unsuccessful for some reason
		case retval
		when "fifty five"
			# a no-op to satisfy ruby1.8
		else
			puts "\n\nThe installation failed for an unknown reason.  Please report this to the mailing list at:\n\t<syllable-developer@lists.sourceforge.net>\n\n"
			$stdout.flush
			exec "/bin/bash", "--login"
		end
	end
else
	# cancel installation
	puts "Installation cancelled (user pressed #{choice.inspect} instead of \"i\" or \"I\"); starting a shell..."
	$stdout.flush
	exec "/bin/bash", "--login"
end

