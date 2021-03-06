## Process this file with automake to produce Makefile.in -*-Makefile-*-

AUTOMAKE_OPTIONS = ../src/ansi2knr

noinst_LIBRARIES = libfetish.a

INCLUDES = -I.. -I$(srcdir) -I../intl
DEFS = -DLIBDIR=\"$(libdir)\" @DEFS@

libfetish_a_SOURCES = \
  getdate.y posixtm.c addext.c argmatch.c backupfile.c basename.c \
  canon-host.c closeout.c diacrit.c dirname.c dup-safer.c \
  exclude.c filemode.c fopen-safer.c \
  full-write.c getopt.c getopt1.c getstr.c getugroups.c hard-locale.c hash.c \
  human.c idcache.c isdir.c linebuffer.c localcharset.c long-options.c \
  makepath.c mbswidth.c md5.c memcasecmp.c memcoll.c modechange.c \
  path-concat.c physmem.c \
  quote.c quotearg.c readtokens.c safe-read.c same.c save-cwd.c \
  savedir.c sha.c stripslash.c unicodeio.c userspec.c version-etc.c xgetcwd.c \
  xgethostname.c xmalloc.c xstrdup.c xstrtod.c xstrtol.c xstrtoul.c \
  xstrtoumax.c yesno.c

libfetish_a_LIBADD = @LIBOBJS@ @ALLOCA@
libfetish_a_DEPENDENCIES = $(libfetish_a_LIBADD)

noinst_HEADERS = \
  argmatch.h backupfile.h bumpalloc.h closeout.h diacrit.h dirname.h error.h \
  exclude.h filemode.h fnmatch.h __fpending.h fsusage.h \
  getdate.h getline.h getopt.h \
  getstr.h getpagesize.h group-member.h hard-locale.h hash.h human.h lchown.h \
  linebuffer.h long-options.h mbswidth.h md5.h memcasecmp.h memcoll.h \
  makepath.h mbswidth.h modechange.h mountlist.h nanosleep.h obstack.h \
  path-concat.h pathmax.h physmem.h posixtm.h \
  quote.h quotearg.h readtokens.h \
  readutmp.h regex.h safe-read.h same.h save-cwd.h savedir.h sha.h \
  stdio-safer.h strverscmp.h unicodeio.h unistd-safer.h version-etc.h \
  xalloc.h xstrtod.h xstrtol.h

BUILT_SOURCES = getdate.c lstat.c stat.c
MAINTAINERCLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = lstat.c stat.c

EXTRA_DIST = xstat.in config.charset ref-add.sin ref-del.sin
lstat.c: xstat.in
	sed \
	  -e '/@IGNORE@/d' \
	  -e 's/@xstat@/lstat/g' \
	  -e '/_LSTAT_ONLY@/d' \
	  -e '/@BEGIN_STAT_ONLY@/,/@END_STAT_ONLY@/d' \
	  $(srcdir)/xstat.in > $@-t
	mv $@-t $@

stat.c: xstat.in
	sed \
	  -e '/@IGNORE@/d' \
	  -e 's/@xstat@/stat/g' \
	  -e '/_STAT_ONLY@/d' \
	  -e '/@BEGIN_LSTAT_ONLY@/,/@END_LSTAT_ONLY@/d' \
	  $(srcdir)/xstat.in > $@-t
	mv $@-t $@


# The following is needed in order to install a simple file in $(libdir)
# which is shared with other installed packages. We use a list of referencing
# packages so that "make uninstall" will remove the file if and only if it
# is not used by another installed package.
# On systems with glibc-2.1 or newer, the file is redundant, therefore we
# avoid installing it.

all-local: charset.alias ref-add.sed ref-del.sed lstat.c stat.c

charset_alias = $(DESTDIR)$(libdir)/charset.alias
charset_tmp = $(DESTDIR)$(libdir)/charset.tmp
install-exec-local: all-local
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	if test -f $(charset_alias); then \
	  sed -f ref-add.sed $(charset_alias) > $(charset_tmp) ; \
	  $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	  rm -f $(charset_tmp) ; \
	else \
	  if test @GLIBC21@ = no; then \
	    sed -f ref-add.sed charset.alias > $(charset_tmp) ; \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias) ; \
	    rm -f $(charset_tmp) ; \
	  fi ; \
	fi

uninstall-local: all-local
	if test -f $(charset_alias); then \
	  sed -f ref-del.sed $(charset_alias) > $(charset_tmp); \
	  if grep '^# Packages using this file: $$' $(charset_tmp) \
	      > /dev/null; then \
	    rm -f $(charset_alias); \
	  else \
	    $(INSTALL_DATA) $(charset_tmp) $(charset_alias); \
	  fi; \
	  rm -f $(charset_tmp); \
	fi

charset.alias: config.charset
	$(SHELL) $(srcdir)/config.charset '@host@' > t-$@
	mv t-$@ $@

SUFFIXES = .sed .sin
.sin.sed:
	sed -e '/^#/d' -e 's/@''PACKAGE''@/@PACKAGE@/g' $< > $@-t
	mv $@-t $@

CLEANFILES = charset.alias ref-add.sed ref-del.sed
