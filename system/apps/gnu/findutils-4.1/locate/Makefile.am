# The default database to build and search.
LOCATE_DB = $(localstatedir)/locatedb

PROGRAMS = locate
LIBPROGRAMS = frcode code bigram
SCRIPTS = updatedb
MANS = locate.1 updatedb.1 locatedb.5
CONFIG_HEADER = ../config.h

DIST_OTHER = locatedb.h updatedb.sh
CLEANFILES = updatedb

INCLUDES = -I.. -I$(top_srcdir)/lib -DLOCATE_DB=\"$(LOCATE_DB)\"

LDADD = ../find/version.o ../lib/libfind.a

$(PROGRAMS) $(LIBPROGRAMS): ../find/version.o ../lib/libfind.a

updatedb: updatedb.sh
	rm -f $@
	version=`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q $(top_srcdir)/find/version.c`; \
	find=`echo find|sed '$(transform)'`; \
	frcode=`echo frcode|sed '$(transform)'`; \
	bigram=`echo bigram|sed '$(transform)'`; \
	code=`echo code|sed '$(transform)'`; \
	sed \
	-e "s,@bindir@,$(bindir)," \
	-e "s,@libexecdir@,$(libexecdir)," \
	-e "s,@LOCATE_DB@,$(LOCATE_DB)," \
	-e "s,@version@,$$version," \
	-e "s,@find@,$$find," \
	-e "s,@frcode@,$$frcode," \
	-e "s,@bigram@,$$bigram," \
	-e "s,@code@,$$code," \
	$(srcdir)/updatedb.sh > $@
	chmod +x $@

install::
	$(top_srcdir)/mkinstalldirs $(localstatedir)

frcode.o code.o locate.o: locatedb.h
locate.o: ../lib/fnmatch.h ../lib/getopt.h
