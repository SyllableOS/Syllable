## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = gnits 1.5d

SUBDIRS = intl lib src doc man m4 po tests djgpp
EXTRA_DIST = Makefile.cfg Makefile.maint GNUmakefile .prev-version

ACLOCAL_AMFLAGS = -I m4

distcheck-hook:
	$(MAKE) my-distcheck
