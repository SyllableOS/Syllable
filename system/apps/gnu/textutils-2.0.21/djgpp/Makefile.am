## Process this file with automake to produce Makefile.in -*-Makefile-*-

EXTRA_DIST = fnchange.lst fnchange.lsx README README.in \
  config.bat config.sed config.site \
  getconf edtests.bat tscript.sed
MAINTAINERCLEANFILES = fnchange.lst README

all-local: README fnchange.lst

fnchange.lst: fnchange.lsx $(top_srcdir)/configure.ac
	sed \
	    -e '/^#/d' \
	    -e 's/@V@/$(PACKAGE)-$(VERSION)/g' \
	  $(srcdir)/fnchange.lsx > t-$@
	mv t-$@ $@

README: README.in $(top_srcdir)/configure.ac
	sed \
	    -e 's/@VERSION\@/$(VERSION)/g' \
	    -e 's/@V@/$(PACKAGE)-$(VERSION)/g' \
	  $(srcdir)/README.in > t-$@
	mv t-$@ $@
