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


TESTS = help-version
TESTS_ENVIRONMENT = \
  all_programs="$(bin_SCRIPTS) $(bin_PROGRAMS)"


EXTRA_DIST = Fetish.pm .env-warn envvar-check lang-default \
  expensive rwx-to-mode setgid-check group-names $(TESTS)

SUBDIRS = chgrp chmod cp dd du dircolors install ln ls ls-2 mkdir mv \
  rm rmdir shred touch

EXEEXT =
OBJEXT = o
subdir = tests
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
DIST_SOURCES =
DIST_COMMON = Makefile.am Makefile.in

DIST_SUBDIRS = $(SUBDIRS)
all: all-recursive

.SUFFIXES:

$(srcdir)/Makefile.in: Makefile.am  $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --gnits  tests/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.
all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive \
install-info-recursive uninstall-info-recursive \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	case "$@" in \
	  distclean-* | maintainer-clean-*) list='$(DIST_SUBDIRS)' ;; \
	  *) list='$(SUBDIRS)' ;; \
	esac; \
	rev=''; for subdir in $$list; do \
	  if test "$$subdir" = "."; then :; else \
	    rev="$$subdir $$rev"; \
	  fi; \
	done; \
	rev="$$rev ."; \
	target=`echo $@ | sed s/-recursive//`; \
	for subdir in $$rev; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test "$$subdir" = . || (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP) $(TAGS_FILES)
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	mkid -fID $$unique $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) \
		$(TAGS_FILES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
   if test "$$subdir" = .; then :; else \
	    test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
   fi; \
	done; \
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || etags $(ETAGS_ARGS) $$tags  $$unique $(LISP)

GTAGS:
	here=`CDPATH=: && cd $(top_builddir) && pwd` \
	  && cd $(top_srcdir) \
	  && gtags -i $$here

distclean-tags:
	-rm -f TAGS ID

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
	for subdir in $(SUBDIRS); do \
	  if test "$$subdir" = .; then :; else \
	    test -d $(distdir)/$$subdir \
	    || mkdir $(distdir)/$$subdir \
	    || exit 1; \
	    (cd $$subdir && \
	      $(MAKE) $(AM_MAKEFLAGS) \
	        top_distdir="$(top_distdir)" \
	        distdir=../$(distdir)/$$subdir \
	        distdir) \
	      || exit 1; \
	  fi; \
	done
check-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
check: check-recursive
all-am: Makefile

installdirs: installdirs-recursive
installdirs-am:

install: install-recursive
install-exec: install-exec-recursive
install-data: install-data-recursive
uninstall: uninstall-recursive

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-recursive

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
clean: clean-recursive

clean-am: clean-generic mostlyclean-am

distclean: distclean-recursive

distclean-am: clean-am distclean-generic distclean-tags

dvi:

dvi-am:

info:

info-am:

install-data-am:

install-exec-am:

install-info:

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-recursive

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-recursive

mostlyclean-am: mostlyclean-generic

.PHONY: all all-am all-recursive check check-TESTS check-am \
	check-recursive clean clean-generic clean-recursive distclean \
	distclean-generic distclean-recursive distclean-tags distdir \
	dvi dvi-am dvi-recursive info info-am info-recursive install \
	install-am install-data install-data-am install-data-recursive \
	install-exec install-exec-am install-exec-recursive \
	install-info install-info-recursive install-man \
	install-recursive install-strip installcheck installcheck-am \
	installcheck-recursive installdirs installdirs-am \
	installdirs-recursive maintainer-clean maintainer-clean-generic \
	maintainer-clean-recursive mostlyclean mostlyclean-generic \
	mostlyclean-recursive tags tags-recursive uninstall \
	uninstall-am uninstall-info-recursive uninstall-recursive


check-root:
	cd cp    && $(MAKE) check TESTS=special-bits
	cd touch && $(MAKE) check TESTS=fifo
	cd mv    && $(MAKE) check TESTS=mv-special-1

check-recursive: envvar-check root-hint

# Warn when `make check' is run with POSIXLY_CORRECT or CDPATH set.
envvar-check:
	test "$${POSIXLY_CORRECT+set}" = set \
	  && sed s/%%/POSIXLY_CORRECT/ $(srcdir)/.env-warn || :
	test "$${CDPATH+set}" = set \
	  && sed s/%%/CDPATH/ $(srcdir)/.env-warn || :

# Advertise `check-root' target.
root-hint:
	@echo '********************************************'
	@echo "NOTICE: Some tests will or might need to be"
	@echo "        run as root. Do \`make check-root'"
	@echo "        as \`root' to run just these tests."
	@echo '********************************************'

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
