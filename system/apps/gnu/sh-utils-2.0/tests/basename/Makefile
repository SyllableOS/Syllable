# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4a from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
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

DESTDIR =

pkgdatadir = $(datadir)/sh-utils
pkglibdir = $(libdir)/sh-utils
pkgincludedir = $(includedir)/sh-utils

top_builddir = ../..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/local/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_STRIP_FLAG =
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i386-unknown-atheos
host_triplet = i386-unknown-atheos
AMTAR = tar
AMTARFLAGS = o
CATALOGS =  cs.gmo de.gmo el.gmo es.gmo fr.gmo it.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sv.gmo zh.gmo
CATOBJEXT = .gmo
CC = gcc
CPP = gcc -E
DATADIRNAME = share
GENCAT = 
GETCONF = getconf
GETLOADAVG_LIBS = 
GMOFILES =  cs.gmo de.gmo el.gmo es.gmo fr.gmo it.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sv.gmo zh.gmo
GMSGFMT = no
GNU_PACKAGE = GNU sh-utils
GT_NO = 
GT_YES = #YES#
INCLUDE_LOCALE_H = #include <locale.h>
INSTOBJEXT = .mo
INTLDEPS = $(top_builddir)/intl/libintl.a
INTLLIBS = $(top_builddir)/intl/libintl.a
INTLOBJS = $(GETTOBJS)
KMEM_GROUP = 
LIBOBJS = lchown$U.o lstat$U.o stat$U.o getloadavg$U.o
LIB_CRYPT = -lcrypt
MAKEINFO = makeinfo
MAN =  uname.1 chroot.1 hostid.1 nice.1 nohup.1 stty.1
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = no
NEED_SETGID = false
OPTIONAL_BIN_PROGS =  uname chroot hostid nice uptime stty
OPTIONAL_BIN_ZCRIPTS =  nohup
PACKAGE = sh-utils
PERL = perl
POFILES =  cs.po de.po el.po es.po fr.po it.po nl.po no.po pl.po pt.po pt_BR.po ru.po sk.po sv.po zh.po
POSUB = po
POW_LIBM = 
RANLIB = ranlib
SEQ_LIBM = -lm
SQRT_LIBM = -lm
U = 
USE_INCLUDED_LIBINTL = yes
USE_NLS = yes
VERSION = 2.0
YACC = bison -y
l = 


AUTOMAKE_OPTIONS = 1.4 gnits

TESTS = basic
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="perl" \
  PATH=../../src:$$PATH \
  PROG=basename

subdir = tests/basename
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../../config.h
CONFIG_CLEAN_FILES = 
DIST_SOURCES = 
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

GZIP_ENV = --best
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnits --include-deps tests/basename/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done

check-TESTS: $(TESTS)
	@failed=0; all=0; xfail=0; xpass=0; \
	srcdir=$(srcdir); export srcdir; \
	for tst in $(TESTS); do \
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
	test "$$failed" -eq 0
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am:
uninstall: uninstall-am
all-am: Makefile
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_STRIP_FLAG=-s install
installdirs:


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: tags distdir check-TESTS info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all install-strip installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
