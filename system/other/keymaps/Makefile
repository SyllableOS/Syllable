# Makefile for keymaps.

DIST_TARGET_PATH := $(IMAGE)/system/keymaps/

all:

deps:

install:

doc:

install-doc:

dist:
	@for keymap in *.map; do\
		destname=`basename $$keymap .map`;\
		rm -f $(DIST_TARGET_PATH)$$destname;\
		keymapconv $$destname.map $(DIST_TARGET_PATH)$$destname;\
        done

clean:

