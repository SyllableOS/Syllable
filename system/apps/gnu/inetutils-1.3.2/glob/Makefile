# Generated automatically from Makefile.in by configure.
# Makefile for standalone distribution of libglob.a (fnmatch, glob).

# Copyright (C) 1991, 92, 93, 94, 95 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with this library; see the file COPYING.LIB.  If
# not, write to the Free Software Foundation, Inc., 675 Mass Ave,
# Cambridge, MA 02139, USA.

# Ultrix 2.2 make doesn't expand the value of VPATH.
# This must repeat the value, because configure will remove `VPATH = .'.
srcdir = .

CC = gcc
CPPFLAGS = 
CFLAGS = -g -O

# Information determined by configure.
DEFS =  -DSTDC_HEADERS=1 -DHAVE_MEMORY_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DHAVE_DIRENT_H=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_STRCOLL=1 

# How to invoke ar.
AR = ar
ARFLAGS = rv

# How to invoke ranlib.
RANLIB = ranlib

.PHONY: all
all: libglob.a

libglob.a: glob.o fnmatch.o
	$(AR) $(ARFLAGS) $@ glob.o fnmatch.o
	$(RANLIB) $@

install:

# For some reason, Unix make wants the dependencies on the source files.
# Otherwise it refuses to use an implicit rule!
# And, get this: it doesn't work to use $(srcdir)/foo.c!!
glob.o: $(srcdir)/glob.h $(srcdir)/fnmatch.h glob.c
fnmatch.o: $(srcdir)/fnmatch.h fnmatch.c

.c.o:
	$(CC) -I. -I$(srcdir) -c \
	      $(DEFS) $(CPPFLAGS) $(CFLAGS) $< $(OUTPUT_OPTION)

.PHONY: clean realclean glob-clean glob-realclean distclean
clean glob-clean:
	-rm -f libglob.a *.o core
distclean glob-realclean: clean
	-rm -f TAGS tags Makefile config.status config.h config.log
realcean: distclean

# For inside the C library.
glob.tar glob.tar.Z:
	$(MAKE) -C .. $@
