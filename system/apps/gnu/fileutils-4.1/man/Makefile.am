## Process this file with automake to produce Makefile.in -*-Makefile-*-
man_MANS = chgrp.1 chmod.1 chown.1 cp.1 dir.1 dd.1 df.1 dircolors.1 \
  du.1 ginstall.1 ln.1 ls.1 mkdir.1 mkfifo.1 mknod.1 mv.1 rm.1 rmdir.1 \
  shred.1 sync.1 touch.1 vdir.1
man_aux = $(man_MANS:.1=.x)

EXTRA_DIST = $(man_MANS) $(HELP2MAN) $(man_aux) \
  Makefile.maint GNUmakefile Makefile.summ
PERL = @PERL@
HELP2MAN = help2man
MAINTAINERCLEANFILES = $(man_MANS)

# Use `ginstall' in the definition of man_MANS to avoid
# confusion with the `install' target.  The install rule transforms `ginstall'
# to install before applying any user-specified name transformations.
transform = s/ginstall/install/; @program_transform_name@

# For additional rules usually of interest only to the maintainer,
# see GNUmakefile and Makefile.maint.
