# system/apps/gnu Makefile for AtheOS/Sylable
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
# 090602 - Copied & modified for system/apps/gnu	- Vanders
# 090602 - Added --build= argument, as bash & some
#		   others require it.						- Vanders
# 100602 - Added --disable-servers for Inetutils	- Vanders
# 110602 - Split SUBDIRS into "standard" and
#		   "other" types.							- Vanders
# 260602 - Make Groff seperate						- Vanders
# 270602 - Make Groff un-seperate!					- Vanders
# 290602 - Fix paths								- Vanders
# 060802 - Add sed-3.02							- Vanders

MOST_SUBDIRS= bash-2.05a diffutils-2.8.1 fileutils-4.1 findutils-4.1 grep-2.5 groff-1.17 gzip-1.2.4a less-374 sed-3.02 sh-utils-2.0 tar-1.13 textutils-2.0.21

BZIP_SUBDIR= bzip2-1.0.2
INETUTILS_SUBDIR= inetutils-1.3.2

SUBDIRS= $(MOST_SUBDIRS) $(INETUTILS_SUBDIR)

BUILD_TYPE= i386-atheos
PREFIX_PATH= $(DIST_DIR)/atheos/usr

all:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir; then exit 1; fi;\
	done

deps:
	for subdir in $(MOST_SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if [ -e $(PWD)/$$subdir/configure ];then\
			echo;echo Entering $(PWD)/$$subdir;echo;\
			cd $(PWD)/$$subdir/ ;\
			if ! ./configure --prefix=$(PREFIX_PATH) --build=$(BUILD_TYPE);then\
				exit 1;\
			fi;\
			cd ..;\
		fi;\
	done

	echo Entering $(PWD)/$(INETUTILS_SUBDIR);\
	cd $(PWD)/$(INETUTILS_SUBDIR);\
	./configure --prefix=$(PREFIX_PATH) --build=$(BUILD_TYPE) --disable-servers;\
	cd ..

install:
	@echo Nothing to be done for $(PWD)

clean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir clean; then exit 1; fi;\
	done

	if ! $(MAKE) -C $(PWD)/$(BZIP_SUBDIR) clean; then exit 1;fi

doc:
	@echo Nothing to be done for $(PWD)

install-doc:
	@echo Nothing to be done for $(PWD)

dist:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir install; then exit 1; fi;\
	done

	cd $(PWD)/$(BZIP_SUBDIR);\
	$(MAKE) install PREFIX=$(PREFIX_PATH);\
	cd ..

distclean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir distclean; then exit 1; fi;\
	done

	if ! $(MAKE) -C $(PWD)/$(BZIP_SUBDIR) distclean; then exit 1;fi

