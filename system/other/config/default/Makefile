# Blank Makefile for non-compilable items (I.E Fonts, Icons etc.)
# Just set DIST_TARGET_PATH below & copy this into
# the directory you are adding.
# REMEMBER: Add the new directory to the Makefile in the parent
# directory!

DIST_TARGET_PATH := $(DIST_DIR)/atheos/sys/config/

all:

deps:

install:

doc:

install-doc:

dist:
	@cp -a appserver dev $(DIST_TARGET_PATH)
	@cd default_user/;\
	cp -p .profile $(DIST_TARGET_PATH)/default_user/.profile;\
	cp -p .profile $(DIST_DIR)/atheos/home/root/.profile;\
	cp -p .profile $(DIST_DIR)/atheos/home/guest/.profile;\
	unzip Desktop/default-desktop-objects.zip -d $(DIST_TARGET_PATH)/default_user/Desktop;\
	unzip Desktop/default-desktop-objects.zip -d $(DIST_DIR)/atheos/home/root/Desktop;\
	unzip Desktop/default-desktop-objects.zip -d $(DIST_DIR)/atheos/home/guest/Desktop;\
	cd ..;

clean:

