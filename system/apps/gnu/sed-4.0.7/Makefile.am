## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.4
ACLOCAL_AMFLAGS = -I config

PACKAGE = sed

SUBDIRS = intl lib po sed doc testsuite

noinst_DATA = bootstrap.sh
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS COPYING.DOC README.boot bootstrap.sh 	      \
	config/codeset.m4  config/gettext.m4   config/iconv.m4        \
	config/getline.m4  config/glibc21.m4   config/isc-posix.m4    \
	config/lcmessage.m4 config/progtest.m4 config/strverscmp.m4   \
	config/texi2dvi config/help2man

html:
	cd doc && make html


