dnl Process this file with autoconf to produce a configure script. -*-m4-*-
AC_INIT(GNU textutils, 2.0.21, bug-textutils@gnu.org)
AC_CONFIG_SRCDIR(src/fold.c)

AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h:config.hin)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE

dnl Checks for programs.

jm_PERL dnl requires perl5.003 or better
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GCC_TRADITIONAL
AC_PROG_RANLIB
AC_AIX
AC_MINIX

jm_MACROS

jm_LIB_CHECK

AM_GNU_GETTEXT

AC_CONFIG_FILES(
  Makefile
  doc/Makefile
  intl/Makefile
  lib/Makefile
  man/Makefile
  m4/Makefile
  po/Makefile.in
  src/Makefile
  djgpp/Makefile
  tests/Makefile
  tests/cut/Makefile
  tests/fmt/Makefile
  tests/head/Makefile
  tests/join/Makefile
  tests/md5sum/Makefile
  tests/misc/Makefile
  tests/od/Makefile
  tests/pr/Makefile
  tests/sha1sum/Makefile
  tests/sort/Makefile
  tests/sum/Makefile
  tests/tac/Makefile
  tests/tail/Makefile
  tests/tail-2/Makefile
  tests/tr/Makefile
  tests/tsort/Makefile
  tests/unexpand/Makefile
  tests/uniq/Makefile
  tests/wc/Makefile
  )
AC_OUTPUT
