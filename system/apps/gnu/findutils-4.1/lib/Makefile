# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake from Makefile.am.
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix = ${prefix}

bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
libexecdir = $(exec_prefix)/libexec
datadir = $(prefix)/share
sysconfdir = $(prefix)/etc
sharedstatedir = $(prefix)/com
localstatedir = $(prefix)/var
libdir = $(exec_prefix)/lib
infodir = $(prefix)/info
mandir = $(prefix)/man
includedir = $(prefix)/include
oldincludedir = /usr/include

INSTALL = /usr/local/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
transform = s,x,x,

ALL = ${PROGRAMS} ${LIBPROGRAMS} ${SCRIPTS} ${LIBSCRIPTS} ${LIBFILES}
CC = gcc
LEX = @LEX@
YACC = @YACC@
ANSI2KNR = ./ansi2knr

DEFS = -DHAVE_CONFIG_H
CPPFLAGS = 
CFLAGS = -g -O
LDFLAGS = 
LIBS = 

find_OBJECTS = regex.o dirname.o error.o filemode.o getopt.o getopt1.o idcache.o listfile.o modechange.o nextelem.o savedir.o xmalloc.o getline.o xstrdup.o xgetcwd.o fnmatch.o 
LIBFILES =  libfind.a
AR = ar
RANLIB = ranlib
ALLOCA = 
LIBOBJS = 

SOURCES =  ${find_SOURCES}
DIST_CONF = Makefile.am Makefile.in
DIST_FILES = $(DIST_CONF) $(SOURCES) $(TEXINFOS) $(INFOS) $(MANS) $(DIST_OTHER)

LIBRARIES = find
find_SOURCES = regex.c dirname.c error.c filemode.c getopt.c getopt1.c \
idcache.c listfile.c modechange.c nextelem.c savedir.c xmalloc.c getline.c \
xstrdup.c xgetcwd.c fnmatch.c $(find_OPT_SOURCES)

find_OPT_SOURCES = fileblocks.c memcmp.c memset.c mktime.c stpcpy.c strdup.c \
strftime.c strspn.c strstr.c strtol.c alloca.c

DIST_OTHER = fnmatch.h getopt.h modechange.h modetype.h pathmax.h \
regex.h wait.h

CONFIG_HEADER = ../config.h
INCLUDES = -I.. -I$(srcdir)

all:: ${ALL}

.c.o:
	$(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $<

$(find_OBJECTS): ../config.h
install:: install-libraries

install-libraries: $(LIBFILES)

uninstall:: uninstall-libraries

uninstall-libraries:

libfind.a: $(find_OBJECTS)  
	rm -f libfind.a
	$(AR) cru libfind.a $(find_OBJECTS)  
	$(RANLIB) libfind.a

mostlyclean:
	rm -f *.o core

clean: mostlyclean
	rm -f $(PROGRAMS) $(LIBPROGRAMS) $(LIBFILES) $(TEXFILES) $(CLEANFILES)

distclean: clean
	rm -f Makefile *.tab.c $(DISTCLEANFILES)
	rm -f config.cache config.log config.status ${CONFIG_HEADER} stamp-h

realclean: distclean
	rm -f TAGS $(INFOS)

dist: $(DIST_FILES) $(DIST_DIRS)
	-mkdir ../`cat ../distname`/$(subdir)
	@for file in $(DIST_FILES); do \
	  echo linking $$file; \
	  ln $(srcdir)/$$file ../`cat ../distname`/$(subdir)/$$file || \
	    { echo copying $$file instead; cp -p $(srcdir)/$$file ../`cat ../distname`/$(subdir)/$$file;}; \
	done

check dvi info install uninstall::

tags:: TAGS

TAGS::
	cd $(srcdir); etags $(SOURCES)

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

fnmatch.o: fnmatch.h
getopt1.o: getopt.h
listfile.o xgetcwd.o: pathmax.h
modechange.o: modechange.h
regex.o: regex.h
