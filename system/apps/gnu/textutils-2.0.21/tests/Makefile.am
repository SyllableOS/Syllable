## Process this file with automake to produce Makefile.in -*-Makefile-*-.

TESTS = help-version
TESTS_ENVIRONMENT = \
  all_programs="$(bin_SCRIPTS) $(bin_PROGRAMS)"

EXTRA_DIST = README Makefile.am.in $(TESTS) .env-warn mk-script Fetish.pm \
  lang-default
SUBDIRS = \
  cut fmt head join md5sum misc od pr sha1sum sort sum tac tail tail-2 \
  tr tsort unexpand uniq wc

check-recursive: envvar-check

# Warn when `make check' is run with POSIXLY_CORRECT or CDPATH set.
envvar-check:
	test "$${POSIXLY_CORRECT+set}" = set \
	  && sed s/%%/POSIXLY_CORRECT/ $(srcdir)/.env-warn || :
	test "$${CDPATH+set}" = set \
	  && sed s/%%/CDPATH/ $(srcdir)/.env-warn || :
