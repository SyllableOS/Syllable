name				:= man2html
source-path		:= src/$(name)
main				:= $(source-path)/$(name)
scripts			:= rebuild_all.sh mksection.sh mkindex.sh addpage.sh manaddpackage.sh
posix-apps-path	:= /usr
executables-dir	:= bin
register			:= pkgmanager -a
install-path		:= $(posix-apps-path)/$(name)
distro-path		:= ($DIST_DIR)/atheos$(install-path)

.PHONY: all deps clean install dist

all: $(main)

$(main):
	$(MAKE) -C $(source-path)

deps:

clean:
	$(MAKE) -C $(source-path) clean

install: all $(install-path)
	cp $(main) $(scripts) $(install-path)/$(executables-dir)
	$(register) $(install-path)

$(install-path):
	mkdir $(install-path)
	mkdir $(install-path)/$(executables-dir)

dist: all $(distro-path)
	cp $(main) $(scripts) $(distro-path)/$(executables-dir)

$(distro-path):
	mkdir $(distro-path)
	mkdir $(distro-path)/$(executables-dir)

