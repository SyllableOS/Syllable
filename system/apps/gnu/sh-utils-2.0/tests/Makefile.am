## Process this file with automake to produce Makefile.in -*-Makefile-*-.

TESTS = nice

EXTRA_DIST = README Makefile.am.in $(TESTS) .env-warn mk-script Fetish.pm
SUBDIRS = basename date factor seq stty test

check-recursive: envvar-check

# Warn when `make check' is run with POSIXLY_CORRECT or CDPATH set.
envvar-check:
	test "$${POSIXLY_CORRECT+set}" = set \
	  && sed s/%%/POSIXLY_CORRECT/ $(srcdir)/.env-warn || :
	test "$${CDPATH+set}" = set \
	  && sed s/%%/CDPATH/ $(srcdir)/.env-warn || :
