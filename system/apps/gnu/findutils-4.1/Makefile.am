DIST_OTHER = COPYING ChangeLog TODO install-sh config.h.in stamp-h.in
SUBDIRS = lib find xargs locate doc testsuite
CONFIG_HEADER = config.h

distname:
	echo findutils-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q find/version.c` > $@
