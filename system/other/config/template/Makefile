# Blank Makefile for non-compilable items (I.E Fonts, Icons etc.)
# Just set DIST_TARGET_PATH below & copy this into
# the directory you are adding.
# REMEMBER: Add the new directory to the Makefile in the parent
# directory!

DIST_TARGET_PATH := $(DIST_DIR)/atheos/home

all:

deps:

install:

doc:

install-doc:

dist:
	mkdir -p $(DIST_TARGET_PATH)/.template
	cp -a * $(DIST_TARGET_PATH)/.template/
	chown -R root:root $(DIST_TARGET_PATH)/.template

	cp -a * $(DIST_TARGET_PATH)/root/
	chown -R root:root $(DIST_TARGET_PATH)/root
	cp -a * $(DIST_TARGET_PATH)/guest/
	chown -R guest:users $(DIST_TARGET_PATH)/guest

clean:

