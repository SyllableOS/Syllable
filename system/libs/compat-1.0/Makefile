# Blank Makefile for non-compilable items (I.E Fonts, Icons etc.)
# Just set DIST_TARGET_PATH below & copy this into
# the directory you are adding.
# REMEMBER: Add the new directory to the Makefile in the parent
# directory!

DIST_TARGET_PATH := $(DIST_DIR)/atheos/sys/libs/

SYS_COMPAT_ARCHIVE := sys-compat.bin.tgz
LIBC_COMPAT_ARCHIVE := libc-compat.bin.tgz

all:

deps:

install:

doc:

install-doc:

dist:
	tar -xzpf $(SYS_COMPAT_ARCHIVE) -C $(DIST_TARGET_PATH)
	tar -xzpf $(LIBC_COMPAT_ARCHIVE) -C $(DIST_TARGET_PATH)

clean:

