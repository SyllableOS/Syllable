## Process this file with automake to produce Makefile.in -*-Makefile-*-
man_MANS = basename.1 chroot.1 date.1 dirname.1 echo.1 env.1 \
expr.1 false.1 groups.1 hostname.1 id.1 logname.1 pathchk.1 \
pinky.1 printenv.1 printf.1 pwd.1 sleep.1 tee.1 test.1 true.1 tty.1 \
users.1 who.1 whoami.1 yes.1 \
hostid.1 nice.1 nohup.1 su.1 stty.1 uname.1
man_aux = $(man_MANS:.1=.x)

# Files to install if configure determines we can build the corresponding
# executables.
# MANFILES2 = nice.1 nohup.1 su.1 stty.1 uname.1

EXTRA_DIST = $(man_MANS) $(HELP2MAN) $(man_aux) \
  Makefile.maint GNUmakefile Makefile.summ
PERL = @PERL@
HELP2MAN = help2man
MAINTAINERCLEANFILES = $(man_MANS)

# For additional rules usually of interest only to the maintainer,
# see GNUmakefile and Makefile.maint.
