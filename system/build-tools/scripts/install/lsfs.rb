#! ruby
# List all available partitions and their respective filesystems

# 26 March 2005, Kaj de Vos
#   Included partitions with unknown type in the list for the partition menu.
# Last modified by xsdg for installer version 0.1.11.6.

module LSFS

def scan
	print "Scanning partitions and filesystems... "
	$stdout.flush
	
	parts = Hash.new
	shortmap = Hash.new	# map from short id to partition name
	
	short_id = "a"
	
	all_parts = Dir["/dev/disk/ata/hd*/*"] + Dir["/dev/disk/bios/hd*/*"]
	
	all_parts.select{|partition|
		partition =~ %r{/\d+$}
	}.sort.each {|partition|
		if tmp = `/bin/fsprobe #{partition} 2> /dev/null`.split("\n").last
			# successfully identified
			parts[partition] = tmp.split		# store partition info
		else
			# unknown partition type
			parts[partition] = ["unknown"]
		end

		shortmap[short_id.to_s] = partition	# store the short map for the partition
		short_id.succ!				# increment the map character
	}
	
	puts "done."

	[parts, shortmap]
end

def list(parts = scan)
	puts "ID\tPartition\t\tType\tSize\tUsed\tAvail"
	parts, shortmap = parts
	shortmap.sort.each {|smap, partition|
		properties = parts[partition]
		
		printf "%s)\t%s:\t%s\t%s\t%s\t%s\n",
			smap,
			partition,
			properties[0],
			properties[1],
			properties[2],
			properties[3]
	}

	[parts, shortmap]
end

module_function :scan, :list
end #module LSFS

LSFS::list if __FILE__ == $0

