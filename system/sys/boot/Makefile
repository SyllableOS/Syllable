# Blank Makefile for non-compilable items (I.E Fonts, Icons etc.)
# Just set DIST_TARGET_PATH below & copy this into
# the directory you are adding.
# REMEMBER: Add the new directory to the Makefile in the parent
# directory!

DIST_TARGET_PATH := $(DIST_DIR)/boot/grub/

all:
	@$(MAKE) -C grub/

deps:
	@cd grub;\
	./configure;\
	cd ..;

install:
	@echo Nothing to be done for $(PWD)

doc:
	@echo Nothing to be done for $(PWD)

install-doc:
	@echo Nothing to be done for $(PWD)

dist:
	@cd grub;\
	cp stage1/stage1 $(DIST_TARGET_PATH);\
	cp stage2/*stage1_5 $(DIST_TARGET_PATH);\
	cp stage2/stage2 $(DIST_TARGET_PATH);\
	cp docs/syllable-menu.lst $(DIST_TARGET_PATH)/menu.lst;\
	cd ..;

clean:
	@$(MAKE) -C grub/ clean

