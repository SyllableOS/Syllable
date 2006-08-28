# Copyright (C) 1995-2002, 2003, 2004 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

# Makefile for elf subdirectory of GNU C Library.  Syllable version.

subdir := elf

include ../Makeconfig

headers	= elf.h bits/elfclass.h link.h bits/link.h
routines = $(sysdep-dl-routines)

others		= sln
install-rootsbin = sln

ifeq (yes,$(build-shared))
extra-objs	= soinit.os sofini.os interp.os
endif

include ../Rules

# Create a dummy ld.so to keep the few external targets that implictly
# depend on it happy
$(objpfx)ld.so:
	touch $(objpfx)ld.so

ifneq (ld.so,$(rtld-installed-name))
# Make sure ld.so.1 exists in the build directory so we can link
# against it.
$(objpfx)$(rtld-installed-name): $(objpfx)ld.so
	rm -f $@
	ln -s $(<F) $@
generated += $(rtld-installed-name)
endif

ifeq (yes,$(build-shared))
# Make sure these things are built in the `make lib' pass so they can be used
# to run programs during the `make others' pass.
lib-noranlib: $(objpfx)$(rtld-installed-name) \
	      $(addprefix $(objpfx),$(extra-objs))
endif

# interp.c exists just to get this string into the libraries.
CFLAGS-interp.c = -D'RUNTIME_LINKER="$(slibdir)/$(rtld-installed-name)"' \
		  -DNOT_IN_libc=1
$(objpfx)interp.os: $(common-objpfx)config.make

ifeq (yes,$(build-shared))
# Special target called by parent to install just the dynamic linker.
.PHONY: ldso_install
ldso_install:
endif

