# system/libs Makefile for AtheOS/Sylable
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
# 090602 - Initial Version 								- Kristian Van Der Vliet
# 160602 - Test for Glibc, we normally don't want to
# 		   build Glibc from source!						- Kristian Van Der Vliet
# 230602 - Comment out Glibc from deps: rule for now	- Kristian Van Der Vliet
# 290602 - Fix directory names (CVS doesn't liike
		   symlinks!)									- Vanders

SUBDIRS = compat-1.0 ncurses-5.2

GLIBC_DIR := glibc

#ifeq ($(BUILD_LIBC),y)
#	SUBDIRS += $(GLIBC_DIR)
#endif

all:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir; then exit 1; fi;\
	done

deps:
	cd $(PWD)/compat-1.0; \
	$(MAKE) deps; \
	cd ..;

	cd $(PWD)/ncurses-5.2; \
	./configure --prefix=$(DIST_DIR)/atheos/usr; \
	cd ..;

#	if test $$BUILD_LIBC='y' ;then\
#		cd $(PWD)/$(GLIBC_DIR); \
#		./configure --prefix=$(DIST_DIR)/atheos/usr/glibc2 --build=i386-atheos --with-headers=$(ATHEOS_INCLUDE_PATH)/atheos; \
#		cd ..;\
#	fi;

install:
	@echo Nothing to be done for $(PWD)

clean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir clean; then exit 1; fi;\
	done

doc:
	@echo Nothing to be done for $(PWD)

install-doc:
	@echo Nothing to be done for $(PWD)

dist:
	cd $(PWD)/compat-1.0; \
	$(MAKE) dist; \
	cd ..;

	cd $(PWD)/ncurses-5.2; \
	$(MAKE) install; \
	cd ..;

distclean:
	@for subdir in $(SUBDIRS); do\
		PWD=$(PWD)/$$subdir;\
		if ! $(MAKE) -C $(PWD)/$$subdir distclean; then exit 1; fi;\
	done


