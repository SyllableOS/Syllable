## Process this file with automake to produce Makefile.in
bin_PROGRAMS = sed

localedir = $(datadir)/locale

sed_SOURCES = sed.c compile.c execute.c regex.c fmt.c
noinst_HEADERS = sed.h

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/intl \
	-I$(top_srcdir) -I$(top_builddir)/lib

AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

sed_LDADD = ../lib/libsed.a @INTLLIBS@
sed_DEPENDENCIES = ../lib/libsed.a

$(PROGRAMS): $(LDADD)


