files				:= menu.lst
system-root			:= /boot
boot-loader-path	:= boot/grub
install-path		:= $(system-root)/$(boot-loader-path)
distro-path			:= $(DIST_DIR)/$(boot-loader-path)

.PHONY: all deps clean install dist

all:

deps:

clean:

install:
	cp -a $(files) $(install-path)

dist:
	cp -a $(files) $(distro-path)

