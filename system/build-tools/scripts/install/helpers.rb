#! ruby

# 26 March 2005, Kaj de Vos
#   Removed invalid hardcoded paths.
# Last modified by xsdg for installer version 0.1.11.6.

# HELPER FUNCTIONS
class IO
	# a function for getting unbuffered characters
	def getuc(suppress_linefeed = false)
		system "stty raw"
		char = self.getc.chr
		system "stty -raw"
		$defout.puts unless suppress_linefeed
		char
	end
	
	# automatically chomp if non-nil
	def getsc(*args)
		str = self.gets(*args)
		str = str.chomp.downcase if str
		$defout.puts
		str
	end
end

# a function to clear the screen and have the user read a file
def read_to_user(filename, full_path = false)
	filename = "doc/#{filename}" unless full_path
	
	system "clear"
	system "less", "--quit-at-eof", "--no-init", "--tilde", filename
end

