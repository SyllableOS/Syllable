## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr

EXTRA_PROGRAMS = df

bin_PROGRAMS = chgrp chown chmod cp dd dircolors du \
ginstall ln dir vdir ls mkdir \
mkfifo mknod mv rm rmdir shred sync touch @DF_PROG@

datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale

noinst_HEADERS = \
  system.h sys2.h copy.h cp-hash.h ls.h dircolors.h remove.h \
  chown-core.h
EXTRA_DIST = dcgen dircolors.hin

INCLUDES = -I.. -I$(srcdir) -I$(top_srcdir)/lib -I../intl
DEFS = -DLOCALEDIR=\"$(localedir)\" -DSHAREDIR=\"$(datadir)\" @DEFS@
LDADD = @INTLLIBS@ ../lib/libfetish.a

dir_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
ls_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
shred_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@
vdir_LDADD = $(LDADD) @LIB_CLOCK_GETTIME@

$(bin_PROGRAMS) $(libexec_PROGRAMS): ../lib/libfetish.a

# Use `ginstall' in the definition of PROGRAMS and in dependencies to avoid
# confusion with the `install' target.  The install rule transforms `ginstall'
# to install before applying any user-specified name transformations.

transform = s/ginstall/install/; @program_transform_name@
ginstall_SOURCES = install.c copy.c cp-hash.c

cp_SOURCES = cp.c copy.c cp-hash.c
dir_SOURCES = ls.c ls-dir.c
vdir_SOURCES = ls.c ls-vdir.c
ls_SOURCES = ls.c ls-ls.c
chown_SOURCES = chown.c chown-core.c
chgrp_SOURCES = chgrp.c chown-core.c

mv_SOURCES = mv.c copy.c cp-hash.c remove.c
rm_SOURCES = rm.c remove.c

BUILT_SOURCES = dircolors.h
PERL = @PERL@
editpl = sed -e 's,@''PERL''@,$(PERL),g'

MAINTAINERCLEANFILES = dircolors.h

dircolors.h: dcgen dircolors.hin
	$(PERL) -w -- $(srcdir)/dcgen $(srcdir)/dircolors.hin > $@-t
	mv $@-t $@
