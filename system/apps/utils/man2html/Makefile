name				:= man2html
source-path		:= src/$(name)
executable			:= $(source-path)/$(name)
binaries			:= rebuild_all.sh mksection.sh mkindex.sh addpage.sh manaddpackage.sh
posix-apps-path	:= /usr
binaries-dir		:= bin
install-path		:= $(posix-apps-path)/$(name)
distro-path		:= ($DIST_ROOT)/atheos$(install-path)

all: $(executable)
	make -C $(source-path)/

deps:

clean:
#	make -C $(source-path)/ clean

install: all $(install-path)
	cp $(executable) $(install-path)/$(binaries-dir)/
	cp $(binaries) $(install-path)/$(binaries-dir)/
	pkgmanager -a $(install-path)/

$(install-path):
	mkdir $(install-path)
	mkdir $(install-path)/$(binaries-dir)

dist: all $(distro-path)
	cp $(executable) $(distro-path)/$(binaries-dir)/
	cp $(binaries) $(distro-path)/$(binaries-dir)/

$(distro-path):
	mkdir $(distro-path)
	mkdir $(distro-path)/$(binaries-dir)

