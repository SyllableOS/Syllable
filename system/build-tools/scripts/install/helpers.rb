#! ruby

# last modified for installer version 0.1.11.6

# HELPER FUNCTIONS
class IO
	# a function for getting unbuffered characters
	def getuc(suppress_linefeed = false)
		system "/usr/bin/stty raw"
		char = self.getc.chr
		system "/usr/bin/stty -raw"
		$defout.puts unless suppress_linefeed
		char
	end
	
	# automatically chomp if non-nil
	def getsc(*args)
		str = self.gets(*args)
		str = str.chomp.downcase unless str.nil?
		$defout.puts
		str
	end
end

# a function to clear the screen and have the user read a file
def read_to_user(filename, full_path = false)
	filename = "doc/#{filename}" unless full_path
	
	system "/usr/bin/clear"
	system "/usr/bin/less", "--quit-at-eof", "--no-init", "--tilde", filename
end

