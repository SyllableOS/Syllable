## Process this file with automake to produce Makefile.in -*-Makefile-*-

AUTOMAKE_OPTIONS = ../src/ansi2knr

noinst_LIBRARIES = libsu.a

INCLUDES = -I.. -I$(srcdir) -I../intl

libsu_a_SOURCES = getdate.y posixtm.c readtokens.c \
  argmatch.c basename.c canon-host.c closeout.c full-write.c \
  getopt.c getopt1.c getugroups.c human.c long-options.c quotearg.c readutmp.c \
  stripslash.c version-etc.c xgetcwd.c xgethostname.c xmalloc.c \
  xstrdup.c xstrtod.c xstrtol.c xstrtoul.c xstrtoumax.c

libsu_a_LIBADD = @LIBOBJS@ @ALLOCA@
libsu_a_DEPENDENCIES = $(libsu_a_LIBADD)

noinst_HEADERS = argmatch.h closeout.h error.h fnmatch.h getdate.h \
  getline.h getopt.h group-member.h human.h lchown.h long-options.h \
  pathmax.h posixtm.h quotearg.h readtokens.h \
  readutmp.h regex.h \
  version-etc.h xalloc.h xstrtod.h xstrtol.h

BUILT_SOURCES = getdate.c lstat.c stat.c

EXTRA_DIST = xstat.in
lstat.c: xstat.in
	sed -e '/@IGNORE@/d' -e 's/@xstat@/lstat/g' $(srcdir)/xstat.in > $@-t
	mv $@-t $@

stat.c: xstat.in
	sed -e '/@IGNORE@/d' -e 's/@xstat@/stat/g' $(srcdir)/xstat.in > $@-t
	mv $@-t $@
