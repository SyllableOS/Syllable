## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = gnits 1.2h

SUBDIRS = lib intl src doc man m4 po tests
EXTRA_DIST = ChangeLog.0 Makefile.maint

install-root:
	cd src && $(MAKE) $@

# Remove this file here (it is created via configure), not from within intl.
DISTCLEANFILES = intl/libintl.h

ACLOCAL_AMFLAGS = -I m4

distcheck-hook:
	$(MAKE) my-distcheck
