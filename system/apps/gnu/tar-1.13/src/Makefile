# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Makefile for GNU tar sources.
# Copyright (C) 1994, 1995, 1996, 1997, 1999 Free Software Foundation, Inc.

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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/tar
pkglibdir = $(libdir)/tar
pkgincludedir = $(includedir)/tar

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/local/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i386-unknown-atheos
host_triplet = i386-unknown-atheos
CATALOGS =  cs.gmo de.gmo es.gmo fr.gmo it.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo ru.gmo sl.gmo sv.gmo
CATOBJEXT = .gmo
CC = gcc
CPP = gcc -E
DATADIRNAME = share
EXEEXT = 
GENCAT = 
GETCONF = getconf
GMOFILES =  cs.gmo de.gmo es.gmo fr.gmo it.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo ru.gmo sl.gmo sv.gmo
GMSGFMT = no
GT_NO = 
GT_YES = #YES#
INCLUDE_LOCALE_H = #include <locale.h>
INSTOBJEXT = .mo
INTLDEPS = $(top_builddir)/intl/libintl.a
INTLLIBS = $(top_builddir)/intl/libintl.a
INTLOBJS = $(GETTOBJS)
LIBOBJS = ftruncate$U.o lchown$U.o
MAKEINFO = makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = no
PACKAGE = tar
POFILES =  cs.po de.po es.po fr.po it.po ko.po nl.po no.po pl.po pt.po ru.po sl.po sv.po
POSUB = po
RANLIB = ranlib
RMT = rmt
U = 
USE_INCLUDED_LIBINTL = yes
USE_NLS = yes
VERSION = 1.13
YACC = bison -y
l = 

AUTOMAKE_OPTIONS = gnits ansi2knr
bin_PROGRAMS = tar
libexec_PROGRAMS = rmt
EXTRA_PROGRAMS = rmt

noinst_HEADERS = arith.h common.h rmt.h system.h tar.h
rmt_SOURCES = rmt.c
tar_SOURCES = arith.c buffer.c compare.c create.c delete.c extract.c incremen.c list.c mangle.c misc.c names.c open3.c rtapelib.c tar.c update.c


datadir = $(prefix)/share
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DHAVE_CONFIG_H
INCLUDES = -I.. -I../intl -I$(top_srcdir)/lib

# Link libtar.a after $(top_builddir)/intl/libintl.a, since $(top_builddir)/intl/libintl.a might invoke rpl_realloc.
LDADD = $(top_builddir)/intl/libintl.a ../lib/libtar.a
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
bin_PROGRAMS =  tar$(EXEEXT)
libexec_PROGRAMS =  rmt
PROGRAMS =  $(bin_PROGRAMS) $(libexec_PROGRAMS)

CPPFLAGS = 
LDFLAGS = 
LIBS = 
ANSI2KNR = 
rmt_OBJECTS =  rmt$U.o
rmt_LDADD = $(LDADD)
rmt_DEPENDENCIES =  ../lib/libtar.a
rmt_LDFLAGS = 
tar_OBJECTS =  arith$U.o buffer$U.o compare$U.o create$U.o delete$U.o \
extract$U.o incremen$U.o list$U.o mangle$U.o misc$U.o names$U.o \
open3$U.o rtapelib$U.o tar$U.o update$U.o
tar_LDADD = $(LDADD)
tar_DEPENDENCIES =  ../lib/libtar.a
tar_LDFLAGS = 
CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in ansi2knr.1 ansi2knr.c


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = gtar
GZIP_ENV = --best
SOURCES = $(rmt_SOURCES) $(tar_SOURCES)
OBJECTS = $(rmt_OBJECTS) $(tar_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnits --include-deps src/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

mostlyclean-libexecPROGRAMS:

clean-libexecPROGRAMS:
	-test -z "$(libexec_PROGRAMS)" || rm -f $(libexec_PROGRAMS)

distclean-libexecPROGRAMS:

maintainer-clean-libexecPROGRAMS:

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libexecdir)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

mostlyclean-krextra:

clean-krextra:
	-rm -f ansi2knr

distclean-krextra:

maintainer-clean-krextra:
ansi2knr: ansi2knr.o
	$(LINK) ansi2knr.o $(LIBS)
ansi2knr.o: $(CONFIG_HEADER)


mostlyclean-kr:
	-rm -f *_.c

clean-kr:

distclean-kr:

maintainer-clean-kr:

rmt$(EXEEXT): $(rmt_OBJECTS) $(rmt_DEPENDENCIES)
	@rm -f rmt$(EXEEXT)
	$(LINK) $(rmt_LDFLAGS) $(rmt_OBJECTS) $(rmt_LDADD) $(LIBS)

tar$(EXEEXT): $(tar_OBJECTS) $(tar_DEPENDENCIES)
	@rm -f tar$(EXEEXT)
	$(LINK) $(tar_LDFLAGS) $(tar_OBJECTS) $(tar_LDADD) $(LIBS)
arith_.c: arith.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/arith.c; then echo $(srcdir)/arith.c; else echo arith.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > arith_.c
buffer_.c: buffer.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/buffer.c; then echo $(srcdir)/buffer.c; else echo buffer.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > buffer_.c
compare_.c: compare.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/compare.c; then echo $(srcdir)/compare.c; else echo compare.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > compare_.c
create_.c: create.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/create.c; then echo $(srcdir)/create.c; else echo create.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > create_.c
delete_.c: delete.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/delete.c; then echo $(srcdir)/delete.c; else echo delete.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > delete_.c
extract_.c: extract.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/extract.c; then echo $(srcdir)/extract.c; else echo extract.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > extract_.c
incremen_.c: incremen.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/incremen.c; then echo $(srcdir)/incremen.c; else echo incremen.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > incremen_.c
list_.c: list.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/list.c; then echo $(srcdir)/list.c; else echo list.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > list_.c
mangle_.c: mangle.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/mangle.c; then echo $(srcdir)/mangle.c; else echo mangle.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > mangle_.c
misc_.c: misc.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/misc.c; then echo $(srcdir)/misc.c; else echo misc.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > misc_.c
names_.c: names.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/names.c; then echo $(srcdir)/names.c; else echo names.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > names_.c
open3_.c: open3.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/open3.c; then echo $(srcdir)/open3.c; else echo open3.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > open3_.c
rmt_.c: rmt.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/rmt.c; then echo $(srcdir)/rmt.c; else echo rmt.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > rmt_.c
rtapelib_.c: rtapelib.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/rtapelib.c; then echo $(srcdir)/rtapelib.c; else echo rtapelib.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > rtapelib_.c
tar_.c: tar.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/tar.c; then echo $(srcdir)/tar.c; else echo tar.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > tar_.c
update_.c: update.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/update.c; then echo $(srcdir)/update.c; else echo update.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > update_.c
arith_.o buffer_.o compare_.o create_.o delete_.o extract_.o \
incremen_.o list_.o mangle_.o misc_.o names_.o open3_.o rmt_.o \
rtapelib_.o tar_.o update_.o : $(ANSI2KNR)

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am: install-binPROGRAMS install-libexecPROGRAMS
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-binPROGRAMS uninstall-libexecPROGRAMS
uninstall: uninstall-am
all-am: Makefile $(ANSI2KNR) $(PROGRAMS) $(HEADERS)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(bindir) $(DESTDIR)$(libexecdir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-binPROGRAMS mostlyclean-libexecPROGRAMS \
		mostlyclean-compile mostlyclean-krextra mostlyclean-kr \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-binPROGRAMS clean-libexecPROGRAMS clean-compile \
		clean-krextra clean-kr clean-tags clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-binPROGRAMS distclean-libexecPROGRAMS \
		distclean-compile distclean-krextra distclean-kr \
		distclean-tags distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-binPROGRAMS \
		maintainer-clean-libexecPROGRAMS \
		maintainer-clean-compile maintainer-clean-krextra \
		maintainer-clean-kr maintainer-clean-tags \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-libexecPROGRAMS distclean-libexecPROGRAMS \
clean-libexecPROGRAMS maintainer-clean-libexecPROGRAMS \
uninstall-libexecPROGRAMS install-libexecPROGRAMS mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-krextra distclean-krextra clean-krextra \
maintainer-clean-krextra mostlyclean-kr distclean-kr clean-kr \
maintainer-clean-kr tags mostlyclean-tags distclean-tags clean-tags \
maintainer-clean-tags distdir info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
