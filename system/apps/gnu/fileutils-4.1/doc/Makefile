# Makefile.in generated automatically by automake 1.4e from Makefile.am.

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/fileutils
pkglibdir = $(libdir)/fileutils
pkgincludedir = $(includedir)/fileutils

top_builddir = ..

ACLOCAL = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run aclocal
AUTOCONF = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run autoconf
AUTOMAKE = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run automake
AUTOHEADER = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run autoheader

INSTALL = /usr/local/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :

host_alias = @host_alias@
host_triplet = i386-pc-atheos


AMDEP = 
AMTAR = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run tar
AWK = gawk
CATALOGS =  cs.gmo da.gmo de.gmo el.gmo es.gmo fr.gmo gl.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sl.gmo sv.gmo zh.gmo
CATOBJEXT = .gmo
CC = gcc
CPP = gcc -E
DATADIRNAME = share
DEPDIR = .deps
DF_PROG = df
FESETROUND_LIBM = -lm
GENCAT = 
GETLOADAVG_LIBS = 
GLIBC21 = yes
GMOFILES =  cs.gmo da.gmo de.gmo el.gmo es.gmo fr.gmo gl.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sl.gmo sv.gmo zh.gmo
GMSGFMT = no
GNU_PACKAGE = GNU fileutils
GT_NO = 
GT_YES = #YES#
INCLUDE_LOCALE_H = #include <locale.h>
INSTALL_STRIP_PROGRAM = $${INSTALL} -s
INSTALL_STRIP_PROGRAM_ENV = 
INSTOBJEXT = .mo
INTLDEPS = $(top_builddir)/intl/libintl.a
INTLLIBS = $(top_builddir)/intl/libintl.a
INTLOBJS = $(GETTOBJS)
KMEM_GROUP = 
LIBICONV = 
LIBOBJS = readutmp$U.o lchown$U.o lstat$U.o lstat$U.o stat$U.o __fpending$U.o ftruncate$U.o regex$U.o getloadavg$U.o memrchr$U.o fsusage$U.o mountlist$U.o
LIB_CLOCK_GETTIME = 
LIB_CRYPT = -lcrypt
LIB_NANOSLEEP = 
MAN = 
MKINSTALLDIRS = /source/Syllable/0.3.9/system/apps/gnu/fileutils/mkinstalldirs
MSGFMT = no
NEED_SETGID = false
OPTIONAL_BIN_PROGS = 
OPTIONAL_BIN_ZCRIPTS = 
PACKAGE = fileutils
PERL = perl
POFILES =  cs.po da.po de.po el.po es.po fr.po gl.po it.po ja.po ko.po nl.po no.po pl.po pt.po pt_BR.po ru.po sk.po sl.po sv.po zh.po
POSUB = po
POW_LIBM = 
RANLIB = ranlib
SEQ_LIBM = 
SQRT_LIBM = -lm
STRIP = 
U = 
USE_INCLUDED_LIBINTL = yes
USE_NLS = yes
VERSION = 4.1
YACC = bison -y
_am_include = #
install_sh = ./install-sh
l = 


info_TEXINFOS = fileutils.texi

EXTRA_DIST = perm.texi getdate.texi

# The following is necessary if the package name is 8 characters or longer.
# If the info documentation would be split into 10 or more separate files,
# then this is necessary even if the package name is 7 characters long.
#
# Tell makeinfo to put everything in a single info file: <package>.info.
# Otherwise, it would also generate files with names like <package>.info-[123],
# and those names all map to one 14-byte name (<package>.info-) on some crufty
# old systems.
MAKEINFO = makeinfo --no-split
EXEEXT =
OBJEXT = o
subdir = doc
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
DIST_SOURCES =
TEXI2DVI = texi2dvi
INFO_DEPS = fileutils.info
DVIS = fileutils.dvi
TEXINFOS = fileutils.texi
DIST_COMMON = Makefile.am Makefile.in mdate-sh stamp-vti texinfo.tex \
version.texi

all: all-am

.SUFFIXES:
.SUFFIXES: .dvi .info .ps .texi .texinfo .txi

$(srcdir)/Makefile.in: Makefile.am  $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --gnits  doc/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status

$(srcdir)/version.texi: $(srcdir)/stamp-vti
	@:

$(srcdir)/stamp-vti: fileutils.texi $(top_srcdir)/configure.in
	@(set `$(SHELL) $(srcdir)/mdate-sh $(srcdir)/fileutils.texi`; \
	echo "@set UPDATED $$1 $$2 $$3"; \
	echo "@set UPDATED-MONTH $$2 $$3"; \
	echo "@set EDITION $(VERSION)"; \
	echo "@set VERSION $(VERSION)") > vti.tmp
	@cmp -s vti.tmp $(srcdir)/version.texi \
	  || (echo "Updating $(srcdir)/version.texi"; \
	      cp vti.tmp $(srcdir)/version.texi)
	-@rm -f vti.tmp
	@cp $(srcdir)/version.texi $@

mostlyclean-vti:
	-rm -f vti.tmp

maintainer-clean-vti:
	-rm -f $(srcdir)/stamp-vti $(srcdir)/version.texi

fileutils.info: fileutils.texi $(srcdir)/version.texi
fileutils.dvi: fileutils.texi $(srcdir)/version.texi

.texi.info:
	@cd $(srcdir) && rm -f $@ $@-[0-9] $@-[0-9][0-9]
	cd $(srcdir) \
	  && $(MAKEINFO) `echo $< | sed 's,.*/,,'`

.texi.dvi:
	TEXINPUTS=$(srcdir):$$TEXINPUTS \
	  MAKEINFO='$(MAKEINFO) -I $(srcdir)' $(TEXI2DVI) $<

.texi:
	@cd $(srcdir) && rm -f $@ $@-[0-9] $@-[0-9][0-9]
	cd $(srcdir) \
	  && $(MAKEINFO) `echo $< | sed 's,.*/,,'`

DVIPS = dvips
.dvi.ps:
	$(DVIPS) $< -o $@

uninstall-info-am:
	$(PRE_UNINSTALL)
	@if $(SHELL) -c 'install-info --version | sed 1q | fgrep -s -v -i debian' >/dev/null 2>&1; then \
	  list='$(INFO_DEPS)'; \
	  for file in $$list; do \
	    echo " install-info --info-dir=$(DESTDIR)$(infodir) --remove $(DESTDIR)$(infodir)/$$file"; \
	    install-info --info-dir=$(DESTDIR)$(infodir) --remove $(DESTDIR)$(infodir)/$$file; \
	  done; \
	else :; fi
	@$(NORMAL_UNINSTALL)
	@list='$(INFO_DEPS)'; \
	for file in $$list; do \
	  (if cd $(DESTDIR)$(infodir); then \
	     echo " rm -f $$file $$file-[0-9] $$file-[0-9][0-9])"; \
	     rm -f $$file $$file-[0-9] $$file-[0-9][0-9]; \
	   else :; fi); \
	done

dist-info: $(INFO_DEPS)
	list='$(INFO_DEPS)'; \
	for base in $$list; do \
	  d=$(srcdir); \
	  for file in `CDPATH=: && cd $$d && eval echo $$base*`; do \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file; \
	  done; \
	done

mostlyclean-aminfo:
	-rm -f fileutils.aux fileutils.cp fileutils.cps fileutils.dvi fileutils.fn \
	  fileutils.ky fileutils.log fileutils.op fileutils.pg \
	  fileutils.ps fileutils.toc fileutils.tp fileutils.vr

maintainer-clean-aminfo:
	cd $(srcdir) && \
	for i in $(INFO_DEPS); do \
	  rm -f $$i; \
	  if test "`echo $$i-[0-9]*`" != "$$i-[0-9]*"; then \
	    rm -f $$i-[0-9]*; \
	  fi; \
	done
tags: TAGS
TAGS:


DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ..
distdir = $(top_distdir)/$(PACKAGE)-$(VERSION)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
	$(MAKE) $(AM_MAKEFLAGS) \
	  top_distdir="${top_distdir}" distdir="$(distdir)" \
	  dist-info
check-am: all-am
check: check-am
all-am: Makefile $(INFO_DEPS)

installdirs:
	$(mkinstalldirs) $(DESTDIR)$(infodir)

install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am

install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  INSTALL_PROGRAM_ENV='$(INSTALL_STRIP_PROGRAM_ENV)' install

mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
clean: clean-am

clean-am: clean-generic mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-generic

dvi: dvi-am

dvi-am: $(DVIS)

info: info-am

info-am: $(INFO_DEPS)

install-data-am: install-info-am

install-exec-am:

install-info: install-info-am

install-info-am: $(INFO_DEPS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(infodir)
	@list='$(INFO_DEPS)'; \
	for file in $$list; do \
	  d=$(srcdir); \
	  for ifile in `CDPATH=: && cd $$d && echo $$file $$file-[0-9] $$file-[0-9][0-9]`; do \
	    if test -f $$d/$$ifile; then \
	      echo " $(INSTALL_DATA) $$d/$$ifile $(DESTDIR)$(infodir)/$$ifile"; \
	      $(INSTALL_DATA) $$d/$$ifile $(DESTDIR)$(infodir)/$$ifile; \
	    else : ; fi; \
	  done; \
	done
	@$(POST_INSTALL)
	@if $(SHELL) -c 'install-info --version | sed 1q | fgrep -s -v -i debian' >/dev/null 2>&1; then \
	  list='$(INFO_DEPS)'; \
	  for file in $$list; do \
	    echo " install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/$$file";\
	    install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/$$file || :;\
	  done; \
	else : ; fi
install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-aminfo \
	maintainer-clean-generic maintainer-clean-vti

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-aminfo mostlyclean-generic mostlyclean-vti

uninstall-am: uninstall-info-am

uninstall-info: uninstall-info-am

.PHONY: all all-am check check-am clean clean-generic dist-info \
	distclean distclean-generic distdir dvi dvi-am info info-am \
	install install-am install-data install-data-am install-exec \
	install-exec-am install-info install-info-am install-man \
	install-strip installcheck installcheck-am installdirs \
	maintainer-clean maintainer-clean-aminfo \
	maintainer-clean-generic maintainer-clean-vti mostlyclean \
	mostlyclean-aminfo mostlyclean-generic mostlyclean-vti \
	uninstall uninstall-am uninstall-info-am


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
