# Blank Makefile for non-compilable items (I.E Fonts, Icons etc.)
# Just set DIST_TARGET_PATH below & copy this into
# the directory you are adding.
# REMEMBER: Add the new directory to the Makefile in the parent
# directory!

DIST_TARGET_PATH := $(DIST_DIR)/atheos/sys/libs/
LIBC_DIR := libc

all:
	@echo Nothing to be done for $(PWD)

deps:
	@echo Nothing to be done for $(PWD)

install:
	@echo Nothing to be done for $(PWD)

doc:
	@echo Nothing to be done for $(PWD)

install-doc:
	@echo Nothing to be done for $(PWD)

dist:
	cp -dp *.so $(DIST_TARGET_PATH)

#	@if test $$BUILD_LIBC = 'n' ;then\
#		cd $(PWD)/$(LIBC_DIR); \
#		cp -dp * $(DIST_TARGET_PATH); \
#		cd ..; \
#	fi;

	cd $(PWD)/$(LIBC_DIR); \
	cp -dp * $(DIST_TARGET_PATH); \
	cd ..;

clean:
	@echo Nothing to be done for $(PWD)

