# system/apps/other Makefile for Sylable
#
# This Makefile covers one of the few places within
# the source tree where we need to run a configure
# script first, and then also have to run "make install",
# rather than "make dist".
#
# Also, because we need to pass different --prefix= args
# when we run each configure script, we cannot use a loop
# and SUBDIRS as we normally do elsewhere.  You should be
# aware of this when adding or removing sub-directories;
# remember to add the directory to both SUBDIRS and to the
# "deps:" target!
#
# Changes
#
# 090602 - Initial Version							- Vanders
# 090602 - Copied & modified for system/apps/other	- Vanders
# 280602 - Fix paths								- Vanders

SUBDIRS= man-1.5i2

BUILD_TYPE:= i386-atheos
PREFIX_PATH:= $(DIST_DIR)/atheos/usr
PREFIX:= $(PREFIX_PATH)

all:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir; then exit 1; fi;\
	done

deps:
	@for subdir in $(SUBDIRS); do\
		cd $(PWD)/$$subdir/ ;\
		if ! ./configure +lang all +fhs;then\
			exit 1;\
		fi;\
		cd ..;\
	done

install:

clean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir clean; then exit 1; fi;\
	done

doc:

install-doc:

dist:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir install; then exit 1; fi;\
	done

distclean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir distclean; then exit 1; fi;\
	done

