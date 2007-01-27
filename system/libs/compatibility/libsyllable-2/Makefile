name				:= LibSyllable-2
package				:= $(name).zip
system-libs-path	:= /system/libs
install-path		:= $(system-libs-path)
distro-path			:= $(IMAGE)/system/libs

.PHONY: all deps clean install dist

all:

deps:

clean:

install:
	unzip $(package) -d $(install-path)

dist:
	unzip -o $(package) -d $(distro-path)

