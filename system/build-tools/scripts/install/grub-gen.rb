#! ruby

# Generate a GRUB menu.lst based on detected partitions

# 26 March 2005, Kaj de Vos
#   Generate boot menu entries for both recognized and unrecognized partitions.
# Last modified by xsdg for installer version 0.1.11.

module GrubGen
	SyllableVersion = "0.5.6"
	Templates = {
		"syllable" => File.read("templates/grubgen/top"),
		"other"    => File.read("templates/grubgen/subsequent")
	}
	
	def save(partitions, sylpart, outfile)
		File.open(outfile, "w") {|fd| fd.puts gen(partitions, sylpart) }
	end
	
	def gen(partitions, sylpart)
		output	= Templates["syllable"].dup
		
		# replace placeholders with actual data
		output.gsub!("##SYLLABLE VERSION##", SyllableVersion)
		output.gsub!("##SYLLABLE ROOT DEVICE PATH##", sylpart)
		output.gsub!("##GRUB ROOT DEVICE##", syl_to_grub(sylpart))
				
		# Add chainload for all partitions other than the Syllable installation partition
		partitions.select {|k, v|
			k != sylpart  # and v != ["unknown"]
		}.sort_by {|k, v|
			k
		}.each {|part, info|
			tmp = Templates["other"].dup
			
			# replace placeholders with actual data
			tmp.gsub!("##PARTITION TYPE##", prettify_partition(info[0]))
			tmp.gsub!("##PARTITION PATH##", part)
			tmp.gsub!("##GRUB PARTITION DEVICE##", syl_to_grub(part))
			
			output << tmp
		}
		
		output
	end
	
	def syl_to_grub(sylpart)
		drive, part = sylpart.scan(%r{/([^/]+)/(\d+)$}).flatten		# extract the drivename and partition number
		out = ""
		
		case drive
		when /^([ch]d)([a-z])$/
			# map hd[letter] to hd[number]
			out = sprintf("(%s%s,%s)", $1, $2[0] - 97, part)
		
		when /^fd([a-z])$/
			# map fd[letter] to fd[number]
			out = "(fd#{$1[0] - 97})"
		else
			out = nil
			$stderr.puts "WARNING: unknown partition type: #{sylpart.inspect}"
		end
		
		out
	end
	
	def prettify_partition(part)
		# FIXME: do some prettification here :o)
		part
	end
	
	module_function :gen, :syl_to_grub, :prettify_partition, :save
	
end #module GrubGen

if(__FILE__ == $0)
	parts = {
		"/dev/disk/ata/hda/2"	=> ["unknown"],
		"/dev/disk/ata/hda/3"	=> ["afs", "452.8M", "105.8M", "347.1M", "23.4%"],
		"/dev/disk/ata/hda/4"	=> ["fat", "14.0G", "107.9M", "13.9G", "0.8%"],		# afs
		"/dev/disk/ata/hda/0"	=> ["afs", "1.9G", "243.4M", "1.7G", "12.4%"]
	}
	sylpart = "/dev/disk/ata/hda/3"
	
	puts GrubGen::gen(parts, sylpart)
end

