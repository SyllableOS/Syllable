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
top_srcdir = ../..

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

top_builddir = ../..

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
MAKEINFO = ${SHELL} /source/Syllable/0.3.9/system/apps/gnu/fileutils/missing --run makeinfo
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


AUTOMAKE_OPTIONS = 1.1 gnits

TESTS = p-1 p-2 special-1 perm parents t-slash
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  srcdir=$(srcdir) \
  PATH=`pwd`/../../src:$$PATH

EXEEXT =
OBJEXT = o
subdir = tests/mkdir
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
DIST_SOURCES =
DIST_COMMON = Makefile.am Makefile.in

all: all-am

.SUFFIXES:

$(srcdir)/Makefile.in: Makefile.am  $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --gnits  tests/mkdir/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status
tags: TAGS
TAGS:


check-TESTS: $(TESTS)
	@failed=0; all=0; xfail=0; xpass=0; \
	srcdir=$(srcdir); export srcdir; \
	list='$(TESTS)'; \
	if test -n "$$list"; then \
	  for tst in $$list; do \
	    if test -f ./$$tst; then dir=./; \
	    elif test -f $$tst; then dir=; \
	    else dir="$(srcdir)/"; fi; \
	    if $(TESTS_ENVIRONMENT) $${dir}$$tst; then \
	      all=`expr $$all + 1`; \
	      case " $(XFAIL_TESTS) " in \
	      *" $$tst "*) \
	        xpass=`expr $$xpass + 1`; \
	        failed=`expr $$failed + 1`; \
	        echo "XPASS: $$tst"; \
	      ;; \
	      *) \
	        echo "PASS: $$tst"; \
	      ;; \
	      esac; \
	    elif test $$? -ne 77; then \
	      all=`expr $$all + 1`; \
	      case " $(XFAIL_TESTS) " in \
	      *" $$tst "*) \
	        xfail=`expr $$xfail + 1`; \
	        echo "XFAIL: $$tst"; \
	      ;; \
	      *) \
	        failed=`expr $$failed + 1`; \
	        echo "FAIL: $$tst"; \
	      ;; \
	      esac; \
	    fi; \
	  done; \
	  if test "$$failed" -eq 0; then \
	    if test "$$xfail" -eq 0; then \
	      banner="All $$all tests passed"; \
	    else \
	      banner="All $$all tests behaved as expected ($$xfail expected failures)"; \
	    fi; \
	  else \
	    if test "$$xpass" -eq 0; then \
	      banner="$$failed of $$all tests failed"; \
	    else \
	      banner="$$failed of $$all tests did not behave as expected ($$xpass unexpected passes)"; \
	    fi; \
	  fi; \
	  dashes=`echo "$$banner" | sed s/./=/g`; \
	  echo "$$dashes"; \
	  echo "$$banner"; \
	  echo "$$dashes"; \
	  test "$$failed" -eq 0; \
	fi

DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ../..
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
check-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
check: check-am
all-am: Makefile

installdirs:

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

dvi:

dvi-am:

info:

info-am:

install-data-am:

install-exec-am:

install-info:

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-generic

.PHONY: all all-am check check-TESTS check-am clean clean-generic \
	distclean distclean-generic distdir dvi dvi-am info info-am \
	install install-am install-data install-data-am install-exec \
	install-exec-am install-info install-man install-strip \
	installcheck installcheck-am installdirs maintainer-clean \
	maintainer-clean-generic mostlyclean mostlyclean-generic \
	uninstall uninstall-am


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
