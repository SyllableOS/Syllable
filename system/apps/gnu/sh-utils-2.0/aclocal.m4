dnl aclocal.m4 generated automatically by aclocal 1.4a

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
dnl We require 2.13 because we rely on SHELL being computed by configure.
AC_PREREQ([2.13])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
dnl We check for tar when the user configures the end package.
dnl This is sad, since we only need this for "dist".  However,
dnl there's no other good way to do it.  We prefer GNU tar if
dnl we can find it.  If we can't find a tar, it doesn't really matter.
AC_CHECK_PROGS(AMTAR, gnutar gtar tar)
AMTARFLAGS=
if test -n "$AMTAR"; then
  if $SHELL -c "$AMTAR --version" > /dev/null 2>&1; then
    dnl We have GNU tar.
    AMTARFLAGS=o
  fi
fi
AC_SUBST(AMTARFLAGS)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

#serial 1

dnl From Jim Meyering.
dnl Find a new-enough version of Perl.
dnl

AC_DEFUN(jm_PERL,
[
  dnl FIXME: don't hard-code 5.003
  dnl FIXME: should we cache the result?
  AC_MSG_CHECKING([for perl5.003 or newer])
  if test "${PERL+set}" = set; then
    # `PERL' is set in the user's environment.
    candidate_perl_names="$PERL"
    perl_specified=yes
  else
    candidate_perl_names='perl perl5'
    perl_specified=no
  fi

  found=no
  AC_SUBST(PERL)
  PERL="$missing_dir/missing perl"
  for perl in $candidate_perl_names; do
    # Run test in a subshell; some versions of sh will print an error if
    # an executable is not found, even if stderr is redirected.
    if ( $perl -e 'require 5.003' ) > /dev/null 2>&1; then
      PERL=$perl
      found=yes
      break
    fi
  done

  AC_MSG_RESULT($found)
  test $found = no && AC_MSG_WARN([
*** You don't seem to have perl5.003 or newer installed.
*** Because of that, you may be unable to regenerate certain files
*** if you modify the sources from which they are derived.] )
])

#serial 4

dnl By default, many hosts won't let programs access large files;
dnl one must use special compiler options to get large-file access to work.
dnl For more details about this brain damage please see:
dnl http://www.sas.com/standards/large.file/x_open.20Mar96.html

dnl Written by Paul Eggert <eggert@twinsun.com>.

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_FLAGS(FLAGSNAME)
AC_DEFUN(AC_SYS_LARGEFILE_FLAGS,
  [AC_CACHE_CHECK([for $1 value to request large file support],
     ac_cv_sys_largefile_$1,
     [ac_cv_sys_largefile_$1=`($GETCONF LFS_$1) 2>/dev/null` || {
	ac_cv_sys_largefile_$1=no
	ifelse($1, CFLAGS,
	  [case "$host_os" in
	   # IRIX 6.2 and later require cc -n32.
changequote(, )dnl
	   irix6.[2-9]* | irix6.1[0-9]* | irix[7-9].* | irix[1-9][0-9]*)
changequote([, ])dnl
	     if test "$GCC" != yes; then
	       ac_cv_sys_largefile_CFLAGS=-n32
	     fi
	     ac_save_CC="$CC"
	     CC="$CC $ac_cv_sys_largefile_CFLAGS"
	     AC_TRY_LINK(, , , ac_cv_sys_largefile_CFLAGS=no)
	     CC="$ac_save_CC"
	   esac])
      }])])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_SPACE_APPEND(VAR, VAL)
AC_DEFUN(AC_SYS_LARGEFILE_SPACE_APPEND,
  [case $2 in
   no) ;;
   ?*)
     case "[$]$1" in
     '') $1=$2 ;;
     *) $1=[$]$1' '$2 ;;
     esac ;;
   esac])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_MACRO_VALUE(C-MACRO, CACHE-VAR, COMMENT, CODE-TO-SET-DEFAULT)
AC_DEFUN(AC_SYS_LARGEFILE_MACRO_VALUE,
  [AC_CACHE_CHECK([for $1], $2,
     [$2=no
changequote(, )dnl
      $4
      for ac_flag in $ac_cv_sys_largefile_CFLAGS no; do
	case "$ac_flag" in
	-D$1)
	  $2=1 ;;
	-D$1=*)
	  $2=`expr " $ac_flag" : '[^=]*=\(.*\)'` ;;
	esac
      done
changequote([, ])dnl
      ])
   if test "[$]$2" != no; then
     AC_DEFINE_UNQUOTED([$1], [$]$2, [$3])
   fi])

AC_DEFUN(AC_SYS_LARGEFILE,
  [AC_REQUIRE([AC_CANONICAL_HOST])
   AC_ARG_ENABLE(largefile,
     [  --disable-largefile     omit support for large files])
   if test "$enable_largefile" != no; then
     AC_CHECK_TOOL(GETCONF, getconf)
     AC_SYS_LARGEFILE_FLAGS(CFLAGS)
     AC_SYS_LARGEFILE_FLAGS(LDFLAGS)
     AC_SYS_LARGEFILE_FLAGS(LIBS)

     for ac_flag in $ac_cv_sys_largefile_CFLAGS no; do
       case "$ac_flag" in
       no) ;;
       -D_FILE_OFFSET_BITS=*) ;;
       -D_LARGEFILE_SOURCE | -D_LARGEFILE_SOURCE=*) ;;
       -D_LARGE_FILES | -D_LARGE_FILES=*) ;;
       -D?* | -I?*)
	 AC_SYS_LARGEFILE_SPACE_APPEND(CPPFLAGS, "$ac_flag") ;;
       *)
	 AC_SYS_LARGEFILE_SPACE_APPEND(CFLAGS, "$ac_flag") ;;
       esac
     done
     AC_SYS_LARGEFILE_SPACE_APPEND(LDFLAGS, "$ac_cv_sys_largefile_LDFLAGS")
     AC_SYS_LARGEFILE_SPACE_APPEND(LIBS, "$ac_cv_sys_largefile_LIBS")
     AC_SYS_LARGEFILE_MACRO_VALUE(_FILE_OFFSET_BITS,
       ac_cv_sys_file_offset_bits,
       [Number of bits in a file offset, on hosts where this is settable.]
       [case "$host_os" in
	# HP-UX 10.20 and later
	hpux10.[2-9][0-9]* | hpux1[1-9]* | hpux[2-9][0-9]*)
	  ac_cv_sys_file_offset_bits=64 ;;
	esac])
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE,
       ac_cv_sys_largefile_source,
       [Define to make fseeko etc. visible, on some hosts.],
       [case "$host_os" in
	# HP-UX 10.20 and later
	hpux10.[2-9][0-9]* | hpux1[1-9]* | hpux[2-9][0-9]*)
	  ac_cv_sys_largefile_source=1 ;;
	esac])
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGE_FILES,
       ac_cv_sys_large_files,
       [Define for large files, on AIX-style hosts.],
       [case "$host_os" in
	# AIX 4.2 and later
	aix4.[2-9]* | aix4.1[0-9]* | aix[5-9].* | aix[1-9][0-9]*)
	  ac_cv_sys_large_files=1 ;;
	esac])
   fi
  ])


# serial 1

AC_DEFUN(AM_C_PROTOTYPES,
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES,1,[Define if compiler has function prototypes])
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=./ansi2knr
  # Ensure some checks needed by ansi2knr itself.
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])


# serial 1

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN(AM_PROG_CC_STDC,
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX 10.20 and later	-Ae
# HP-UX older versions	-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 -Ae "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT($am_cv_prog_cc_stdc)
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

#serial 1

dnl Just like AC_C_CONST from autoconf-2.12, but with an initializer
dnl for `charset x' and with
dnl   AC_DEFINE(const, )
dnl changed to
dnl   AC_DEFINE_UNQUOTED(const, [/* empty */])
dnl to avoid this warning:
dnl   [...]/m4: Warning: Excess arguments to built-in `define' ignored

undefine([AC_C_CONST])
AC_DEFUN(AC_C_CONST,
[dnl This message is consistent in form with the other checking messages,
dnl and with the result message.
AC_CACHE_CHECK([for working const], ac_cv_c_const,
[AC_TRY_COMPILE(,
changequote(<<, >>)dnl
<<
/* Ultrix mips cc rejects this.  */
typedef int charset[2]; const charset x = {0, 0};
/* SunOS 4.1.1 cc rejects this.  */
char const *const *ccp;
char **p;
/* NEC SVR4.0.2 mips cc rejects this.  */
struct point {int x, y;};
static struct point const zero = {0,0};
/* AIX XL C 1.02.0.0 rejects this.
   It does not let you subtract one const X* pointer from another in an arm
   of an if-expression whose if-part is not a constant expression */
const char *g = "string";
ccp = &g + (g ? g-g : 0);
/* HPUX 7.0 cc rejects these. */
++ccp;
p = (char**) ccp;
ccp = (char const *const *) p;
{ /* SCO 3.2v4 cc rejects this.  */
  char *t;
  char const *s = 0 ? (char *) 0 : (char const *) 0;

  *t++ = 0;
}
{ /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
  int x[] = {25, 17};
  const int *foo = &x[0];
  ++foo;
}
{ /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
  typedef const int *iptr;
  iptr p = 0;
  ++p;
}
{ /* AIX XL C 1.02.0.0 rejects this saying
     "k.c", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
  struct s { int j; const int *ap[3]; };
  struct s *b; b->j = 5;
}
{ /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
  const int foo = 10;
}
>>,
changequote([, ])dnl
ac_cv_c_const=yes, ac_cv_c_const=no)])
if test $ac_cv_c_const = no; then
  AC_DEFINE_UNQUOTED(const, [/* empty */])
fi
])

#serial 7

dnl Misc type-related macros for fileutils, sh-utils, textutils.

AC_DEFUN(jm_MACROS,
[
  AC_PREREQ(2.14.1)               dnl Minimum Autoconf version required.

  GNU_PACKAGE="GNU $PACKAGE"
  AC_DEFINE_UNQUOTED(GNU_PACKAGE, "$GNU_PACKAGE",
    [The concatenation of the strings \`GNU ', and PACKAGE.])
  AC_SUBST(GNU_PACKAGE)

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  jm_INCLUDED_REGEX([lib/regex.c])

  AC_REQUIRE([jm_ASSERT])
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_STRUCT_UTIMBUF])
  AC_REQUIRE([jm_STRUCT_DIRENT_D_TYPE])
  AC_REQUIRE([jm_STRUCT_DIRENT_D_INO])
  AC_REQUIRE([jm_CHECK_DECLS])

  AC_REQUIRE([jm_PREREQ])

  AC_REQUIRE([jm_FUNC_LCHOWN])
  AC_REQUIRE([jm_FUNC_CHOWN])
  AC_REQUIRE([jm_FUNC_MKTIME])
  AC_REQUIRE([jm_FUNC_LSTAT])
  AC_REQUIRE([jm_FUNC_STAT])
  AC_REQUIRE([jm_FUNC_REALLOC])
  AC_REQUIRE([jm_FUNC_MALLOC])
  AC_REQUIRE([jm_FUNC_READDIR])
  AC_REQUIRE([jm_FUNC_MEMCMP])
  AC_REQUIRE([jm_FUNC_GLIBC_UNLOCKED_IO])
  AC_REQUIRE([jm_FUNC_FNMATCH])
  AC_REQUIRE([jm_AFS])
  AC_REQUIRE([jm_AC_PREREQ_XSTRTOUMAX])
  AC_REPLACE_FUNCS(strcasecmp strncasecmp)
  AC_REPLACE_FUNCS(dup2)
  AC_REPLACE_FUNCS(memchr)
  AC_REPLACE_FUNCS(memmove)
  AC_CHECK_FUNCS(getpagesize)

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE(ARGMATCH_DIE, [usage (1)],
	    [Define to the function xargmatch calls on failures.])
  AC_DEFINE(ARGMATCH_DIE_DECL, [extern void usage ()],
	    [Define to the declaration of the xargmatch failure function.])

  dnl Used to define SETVBUF in sys2.h.
  dnl This evokes the following warning from autoconf:
  dnl ...: warning: AC_TRY_RUN called without default to allow cross compiling
  AC_FUNC_SETVBUF_REVERSED

  AM_FUNC_GETLINE
  if test $am_cv_func_working_getline != yes; then
    AC_CHECK_FUNCS(getdelim)
  fi

])

AC_DEFUN(jm_CHECK_ALL_TYPES,
[
  AC_TYPE_GETGROUPS
  AC_TYPE_MODE_T
  AC_TYPE_OFF_T
  AC_TYPE_PID_T
  AC_TYPE_SIGNAL
  AC_TYPE_SIZE_T
  AC_TYPE_UID_T
  AC_CHECK_TYPE(ino_t, unsigned long)
  AC_CHECK_TYPE(ssize_t, int)
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
])

#serial 1
dnl This test replaces the one in autoconf.
dnl Currently this macro should have the same name as the autoconf macro
dnl because gettext's gettext.m4 (distributed in the automake package)
dnl still uses it.  Otherwise, the use in gettext.m4 makes autoheader
dnl give these diagnostics:
dnl   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
dnl   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])
AC_DEFUN(AC_ISC_POSIX,
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

#serial 5

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

dnl Usage: jm_INCLUDED_REGEX([lib/regex.c])
dnl
AC_DEFUN(jm_INCLUDED_REGEX,
  [
    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    # Assume we'll default to using the included regex.c.
    ac_use_included_regex=yes

    # However, if the system regex support is good enough that it passes the
    # the following run test, then default to *not* using the included regex.c.
    # If cross compiling, assume the test would fail and use the included
    # regex.c.  The failing regular expression is from `Spencer ere test #75'
    # in grep-2.3.
    AC_CACHE_CHECK([for working re_compile_pattern],
		   jm_cv_func_working_re_compile_pattern,
      AC_TRY_RUN(
	changequote(<<, >>)dnl
	<<
#include <stdio.h>
#include <regex.h>
	  int
	  main ()
	  {
	    static struct re_pattern_buffer regex;
	    const char *s;
	    re_set_syntax (RE_SYNTAX_POSIX_EGREP);
	    /* Add this third left square bracket, [, to balance the
	       three right ones below.  Otherwise autoconf-2.14 chokes.  */
	    s = re_compile_pattern ("a[[:]:]]b\n", 9, &regex);
	    /* This should fail with _Invalid character class name_ error.  */
	    exit (s ? 0 : 1);
	  }
	>>,
	changequote([, ])dnl

	       jm_cv_func_working_re_compile_pattern=yes,
	       jm_cv_func_working_re_compile_pattern=no,
	       dnl When crosscompiling, assume it's broken.
	       jm_cv_func_working_re_compile_pattern=no))
    if test $jm_cv_func_working_re_compile_pattern = yes; then
      ac_use_included_regex=no
    fi

    test -n "$1" || AC_MSG_ERROR([missing argument])
    syscmd([test -f $1])
    ifelse(sysval, 0,
      [

	AC_ARG_WITH(included-regex,
	[  --without-included-regex don't compile regex; this is the default on
                          systems with version 2 of the GNU C library
                          (use with caution on other system)],
		    jm_with_regex=$withval,
		    jm_with_regex=$ac_use_included_regex)
	if test "$jm_with_regex" = yes; then
	  AC_SUBST(LIBOBJS)
	  LIBOBJS="$LIBOBJS regex.$ac_objext"
	fi
      ],
    )
  ]
)

#serial 2
dnl based on code from Eleftherios Gkioulekas

AC_DEFUN(jm_ASSERT,
[
  AC_MSG_CHECKING(whether to enable assertions)
  AC_ARG_ENABLE(assert,
	[  --disable-assert        turn off assertions],
	[ AC_MSG_RESULT(no)
	  AC_DEFINE(NDEBUG,1,[Define to 1 if assertions should be disabled.]) ],
	[ AC_MSG_RESULT(yes) ]
               )
])

#serial 3

dnl From Paul Eggert.

# Define HAVE_INTTYPES_H if <inttypes.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.

AC_DEFUN(jm_AC_HEADER_INTTYPES_H,
[
  AC_CACHE_CHECK([for inttypes.h], jm_ac_cv_header_inttypes_h,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
#include <inttypes.h>],
    [uintmax_t i = (uintmax_t) -1;],
    jm_ac_cv_header_inttypes_h=yes,
    jm_ac_cv_header_inttypes_h=no)])
  if test $jm_ac_cv_header_inttypes_h = yes; then
    AC_DEFINE_UNQUOTED(HAVE_INTTYPES_H, 1,
[Define if <inttypes.h> exists, doesn't clash with <sys/types.h>,
   and declares uintmax_t. ])
  fi
])

#serial 2

dnl From Jim Meyering

dnl Define HAVE_STRUCT_UTIMBUF if `struct utimbuf' is declared --
dnl usually in <utime.h>.
dnl Some systems have utime.h but don't declare the struct anywhere.

AC_DEFUN(jm_STRUCT_UTIMBUF,
[
  AC_CHECK_HEADERS(utime.h)
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CACHE_CHECK([for struct utimbuf], fu_cv_sys_struct_utimbuf,
    [AC_TRY_COMPILE(
      [
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
      ],
      [static struct utimbuf x; x.actime = x.modtime;],
      fu_cv_sys_struct_utimbuf=yes,
      fu_cv_sys_struct_utimbuf=no)
    ])

  if test $fu_cv_sys_struct_utimbuf = yes; then
    AC_DEFINE_UNQUOTED(HAVE_STRUCT_UTIMBUF, 1,
[Define if struct utimbuf is declared -- usually in <utime.h>.
   Some systems have utime.h but don't declare the struct anywhere. ])
  fi
])

#serial 2

dnl From Jim Meyering.
dnl
dnl Check whether struct dirent has a member named d_type.
dnl

AC_DEFUN(jm_STRUCT_DIRENT_D_TYPE,
  [AC_REQUIRE([AC_HEADER_DIRENT])dnl
   AC_CACHE_CHECK([for d_type member in directory struct],
		  jm_cv_struct_dirent_d_type,
     [AC_TRY_LINK(dnl
       [
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
       ],
       [struct dirent dp; dp.d_type = 0;],

       jm_cv_struct_dirent_d_type=yes,
       jm_cv_struct_dirent_d_type=no)
     ]
   )
   if test $jm_cv_struct_dirent_d_type = yes; then
     AC_DEFINE(D_TYPE_IN_DIRENT, 1,
  [Define if there is a member named d_type in the struct describing
   directory headers.])
   fi
  ]
)

#serial 2

dnl From Jim Meyering.
dnl
dnl Check whether struct dirent has a member named d_ino.
dnl

AC_DEFUN(jm_STRUCT_DIRENT_D_INO,
  [AC_REQUIRE([AC_HEADER_DIRENT])dnl
   AC_CACHE_CHECK([for d_ino member in directory struct],
		  jm_cv_struct_dirent_d_ino,
     [AC_TRY_LINK(dnl
       [
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
       ],
       [struct dirent dp; dp.d_ino = 0;],

       jm_cv_struct_dirent_d_ino=yes,
       jm_cv_struct_dirent_d_ino=no)
     ]
   )
   if test $jm_cv_struct_dirent_d_ino = yes; then
     AC_DEFINE(D_INO_IN_DIRENT, 1,
  [Define if there is a member named d_ino in the struct describing
   directory headers.])
   fi
  ]
)

#serial 5

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(jm_CHECK_DECLS,
[
  headers='
#include <stdio.h>
#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
'

  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_DEFINE(HAVE_DECL_FREE, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_LSEEK, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_MALLOC, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_MEMCHR, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_REALLOC, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STPCPY, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRSTR, 1, [Define if this function is declared.])
  fi

  jm_CHECK_DECLARATIONS($headers, free lseek malloc \
                        memchr realloc stpcpy strstr)
])

#serial 3

AC_DEFUN(jm_CHECK_DECLARATION,
[
  AC_REQUIRE([AC_HEADER_STDC])dnl
  test -z "$ac_cv_header_memory_h" && AC_CHECK_HEADERS(memory.h)
  test -z "$ac_cv_header_string_h" && AC_CHECK_HEADERS(string.h)
  test -z "$ac_cv_header_strings_h" && AC_CHECK_HEADERS(strings.h)
  test -z "$ac_cv_header_stdlib_h" && AC_CHECK_HEADERS(stdlib.h)
  test -z "$ac_cv_header_unistd_h" && AC_CHECK_HEADERS(unistd.h)
  AC_MSG_CHECKING([whether $1 is declared])
  AC_CACHE_VAL(jm_cv_func_decl_$1,
    [AC_TRY_COMPILE($2,
      [
#ifndef $1
char *(*pfn) = (char *(*)) $1
#endif
      ],
      eval "jm_cv_func_decl_$1=yes",
      eval "jm_cv_func_decl_$1=no")])

  if eval "test \"`echo '$jm_cv_func_decl_'$1`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$3], , :, [$3])
  else
    AC_MSG_RESULT(no)
    ifelse([$4], , , [$4
])dnl
  fi
])dnl

dnl jm_CHECK_DECLARATIONS(INCLUDES, FUNCTION... [, ACTION-IF-DECLARED
dnl                       [, ACTION-IF-NOT-DECLARED]])
AC_DEFUN(jm_CHECK_DECLARATIONS,
[
  for jm_func in $2
  do
    jm_CHECK_DECLARATION($jm_func, $1,
    [
      jm_tr_func=HAVE_DECL_`echo $jm_func | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ`
      AC_DEFINE_UNQUOTED($jm_tr_func) $3], $4)dnl
  done
])

#serial 2

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN(jm_PREREQ,
[
  jm_PREREQ_ERROR
  jm_PREREQ_REGEX
])

dnl FIXME: maybe put this in a separate file
AC_DEFUN(jm_PREREQ_REGEX,
[
  dnl FIXME: Maybe provide a btowc replacement someday: solaris-2.5.1 lacks it.
  dnl FIXME: Check for wctype and iswctype, and and add -lw if necessary
  dnl to get them.
  AC_CHECK_FUNCS(bzero bcopy isascii btowc)
  AC_CHECK_HEADERS(alloca.h libintl.h wctype.h wchar.h)
  AC_HEADER_STDC
  AC_FUNC_ALLOCA
])

#serial 1

dnl FIXME: put these prerequisite-only *.m4 files in a separate
dnl directory -- otherwise, they'll conflict with existing files.

dnl These are the prerequisite macros for GNU's error.c file.
AC_DEFUN(jm_PREREQ_ERROR,
[
  AC_CHECK_FUNCS(strerror strerror_r vprintf doprnt)
  AC_HEADER_STDC
])

#serial 1

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN(jm_FUNC_LCHOWN,
[
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REPLACE_FUNCS(lchown)
])

#serial 4

dnl From Jim Meyering.
dnl Determine whether chown accepts arguments of -1 for uid and gid.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN(jm_FUNC_CHOWN,
[AC_REQUIRE([AC_TYPE_UID_T])dnl
 test -z "$ac_cv_header_unistd_h" \
   && AC_CHECK_HEADERS(unistd.h)
 AC_CACHE_CHECK([for working chown], jm_cv_func_working_chown,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   ifdef HAVE_UNISTD_H
#    include <unistd.h>
#   endif

    int
    main ()
    {
      char *f = "conftestchown";
      struct stat before, after;

      if (creat (f, 0600) < 0)
        exit (1);
      if (stat (f, &before) < 0)
        exit (1);
      if (chown (f, (uid_t) -1, (gid_t) -1) == -1)
        exit (1);
      if (stat (f, &after) < 0)
        exit (1);
      exit ((before.st_uid == after.st_uid
	     && before.st_gid == after.st_gid) ? 0 : 1);
    }
	      ],
	     jm_cv_func_working_chown=yes,
	     jm_cv_func_working_chown=no,
	     dnl When crosscompiling, assume chown is broken.
	     jm_cv_func_working_chown=no)
  ])
  if test $jm_cv_func_working_chown = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS chown.$ac_objext"
    AC_DEFINE_UNQUOTED(chown, rpl_chown,
      [Define to rpl_chown if the replacement function should be used.])
  fi
])

#serial 7

dnl From Jim Meyering.
dnl A wrapper around AC_FUNC_MKTIME.

AC_DEFUN(jm_FUNC_MKTIME,
[AC_REQUIRE([AC_FUNC_MKTIME])dnl

 dnl mktime.c uses localtime_r if it exists.  Check for it.
 AC_CHECK_FUNCS(localtime_r)

 if test $ac_cv_func_working_mktime = no; then
   AC_DEFINE_UNQUOTED(mktime, rpl_mktime,
    [Define to rpl_mktime if the replacement function should be used.])
 fi
])

#serial 1000

dnl From Paul Eggert
dnl Check for a working mktime.
dnl This is a preview of what should appear in the next public autoconf release.

dnl Override any existing definition.
undefine([AC_FUNC_MKTIME])

AC_DEFUN(AC_FUNC_MKTIME,
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CHECK_HEADERS(sys/time.h unistd.h)
AC_CHECK_FUNCS(alarm)
AC_CACHE_CHECK([for working mktime], ac_cv_func_working_mktime,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<</* Test program from Paul Eggert (eggert@twinsun.com)
   and Tony Leneis (tony@plaza.ds.adp.com).  */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if !HAVE_ALARM
# define alarm(X) /* empty */
#endif

/* Work around redefinition to rpl_putenv by other config tests.  */
#undef putenv

static time_t time_t_max;

/* Values we'll use to set the TZ environment variable.  */
static const char *const tz_strings[] = {
  (const char *) 0, "TZ=GMT0", "TZ=JST-9",
  "TZ=EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00"
};
#define N_STRINGS (sizeof (tz_strings) / sizeof (tz_strings[0]))

/* Fail if mktime fails to convert a date in the spring-forward gap.
   Based on a problem report from Andreas Jaeger.  */
static void
spring_forward_gap ()
{
  /* glibc (up to about 1998-10-07) failed this test) */
  struct tm tm;

  /* Use the portable POSIX.1 specification "TZ=PST8PDT,M4.1.0,M10.5.0"
     instead of "TZ=America/Vancouver" in order to detect the bug even
     on systems that don't support the Olson extension, or don't have the
     full zoneinfo tables installed.  */
  putenv ("TZ=PST8PDT,M4.1.0,M10.5.0");

  tm.tm_year = 98;
  tm.tm_mon = 3;
  tm.tm_mday = 5;
  tm.tm_hour = 2;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  if (mktime (&tm) == (time_t)-1)
    exit (1);
}

static void
mktime_test (now)
     time_t now;
{
  struct tm *lt;
  if ((lt = localtime (&now)) && mktime (lt) != now)
    exit (1);
  now = time_t_max - now;
  if ((lt = localtime (&now)) && mktime (lt) != now)
    exit (1);
}

static void
irix_6_4_bug ()
{
  /* Based on code from Ariel Faigon.  */
  struct tm tm;
  tm.tm_year = 96;
  tm.tm_mon = 3;
  tm.tm_mday = 0;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  mktime (&tm);
  if (tm.tm_mon != 2 || tm.tm_mday != 31)
    exit (1);
}

static void
bigtime_test (j)
     int j;
{
  struct tm tm;
  time_t now;
  tm.tm_year = tm.tm_mon = tm.tm_mday = tm.tm_hour = tm.tm_min = tm.tm_sec = j;
  now = mktime (&tm);
  if (now != (time_t) -1)
    {
      struct tm *lt = localtime (&now);
      if (! (lt
	     && lt->tm_year == tm.tm_year
	     && lt->tm_mon == tm.tm_mon
	     && lt->tm_mday == tm.tm_mday
	     && lt->tm_hour == tm.tm_hour
	     && lt->tm_min == tm.tm_min
	     && lt->tm_sec == tm.tm_sec
	     && lt->tm_yday == tm.tm_yday
	     && lt->tm_wday == tm.tm_wday
	     && ((lt->tm_isdst < 0 ? -1 : 0 < lt->tm_isdst)
		  == (tm.tm_isdst < 0 ? -1 : 0 < tm.tm_isdst))))
	exit (1);
    }
}

int
main ()
{
  time_t t, delta;
  int i, j;

  /* This test makes some buggy mktime implementations loop.
     Give up after 60 seconds; a mktime slower than that
     isn't worth using anyway.  */
  alarm (60);

  for (time_t_max = 1; 0 < time_t_max; time_t_max *= 2)
    continue;
  time_t_max--;
  delta = time_t_max / 997; /* a suitable prime number */
  for (i = 0; i < N_STRINGS; i++)
    {
      if (tz_strings[i])
	putenv (tz_strings[i]);

      for (t = 0; t <= time_t_max - delta; t += delta)
	mktime_test (t);
      mktime_test ((time_t) 60 * 60);
      mktime_test ((time_t) 60 * 60 * 24);

      for (j = 1; 0 < j; j *= 2)
        bigtime_test (j);
      bigtime_test (j - 1);
    }
  irix_6_4_bug ();
  spring_forward_gap ();
  exit (0);
}
>>,
changequote([, ])dnl
ac_cv_func_working_mktime=yes, ac_cv_func_working_mktime=no,
ac_cv_func_working_mktime=no)])
if test $ac_cv_func_working_mktime = no; then
  LIBOBJS="$LIBOBJS mktime.${ac_objext}"
fi
])

#serial 3

dnl From Jim Meyering.
dnl Determine whether lstat has the bug that it succeeds when given the
dnl zero-length file name argument.  The lstat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_LSTAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN(jm_FUNC_LSTAT,
[
 AC_CACHE_CHECK([whether lstat accepts an empty string],
  jm_cv_func_lstat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (lstat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_lstat_empty_string_bug=yes,
	 jm_cv_func_lstat_empty_string_bug=no,
	 dnl When crosscompiling, assume lstat is broken.
	 jm_cv_func_lstat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_lstat_empty_string_bug = yes; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS lstat.$ac_objext"
    AC_DEFINE_UNQUOTED(HAVE_LSTAT_EMPTY_STRING_BUG, 1,
[Define if lstat has the bug that it succeeds when given the zero-length
   file name argument.  The lstat from SunOS4.1.4 and the Hurd as of 1998-11-01)
   do this. ])
  fi
])

#serial 3

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_STAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN(jm_FUNC_STAT,
[
 AC_CACHE_CHECK([whether stat accepts an empty string],
  jm_cv_func_stat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (stat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_stat_empty_string_bug=yes,
	 jm_cv_func_stat_empty_string_bug=no,
	 dnl When crosscompiling, assume stat is broken.
	 jm_cv_func_stat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_stat_empty_string_bug = yes; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS stat.$ac_objext"
    AC_DEFINE_UNQUOTED(HAVE_STAT_EMPTY_STRING_BUG, 1,
[Define if stat has the bug that it succeeds when given the zero-length
   file name argument.  The stat from SunOS4.1.4 and the Hurd as of 1998-11-01)
   do this. ])
  fi
])

#serial 3

dnl From Jim Meyering.
dnl Determine whether realloc works when both arguments are 0.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN(jm_FUNC_REALLOC,
[
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken realloc -- as it might if this test were omitted.
 AC_DEFINE_UNQUOTED(HAVE_DONE_WORKING_REALLOC_CHECK, 1,
                    [Define if the realloc check has been performed. ])

 AC_CACHE_CHECK([for working realloc], jm_cv_func_working_realloc,
  [AC_TRY_RUN([
    char *realloc ();
    int
    main ()
    {
      exit (realloc (0, 0) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_realloc=yes,
	 jm_cv_func_working_realloc=no,
	 dnl When crosscompiling, assume realloc is broken.
	 jm_cv_func_working_realloc=no)
  ])
  if test $jm_cv_func_working_realloc = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS realloc.$ac_objext"
    AC_DEFINE_UNQUOTED(realloc, rpl_realloc,
      [Define to rpl_realloc if the replacement function should be used.])
  fi
])

#serial 3

dnl From Jim Meyering.
dnl Determine whether malloc accepts 0 as its argument.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN(jm_FUNC_MALLOC,
[
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken malloc -- as it might if this test were omitted.
 AC_DEFINE_UNQUOTED(HAVE_DONE_WORKING_MALLOC_CHECK, 1,
                    [Define if the malloc check has been performed. ])

 AC_CACHE_CHECK([for working malloc], jm_cv_func_working_malloc,
  [AC_TRY_RUN([
    char *malloc ();
    int
    main ()
    {
      exit (malloc (0) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_malloc=yes,
	 jm_cv_func_working_malloc=no,
	 dnl When crosscompiling, assume malloc is broken.
	 jm_cv_func_working_malloc=no)
  ])
  if test $jm_cv_func_working_malloc = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS malloc.$ac_objext"
    AC_DEFINE_UNQUOTED(malloc, rpl_malloc,
      [Define to rpl_malloc if the replacement function should be used.])
  fi
])

#serial 2

dnl SunOS's readdir is broken in such a way that rm.c has to add extra code
dnl to test whether a NULL return value really means there are no more files
dnl in the directory.
dnl
dnl Detect the problem by creating a directory containing 300 files (254 not
dnl counting . and .. is the minimum) and see if a loop doing `readdir; unlink'
dnl removes all of them.
dnl
dnl Define HAVE_WORKING_READDIR if readdir does *not* have this problem.

dnl Written by Jim Meyering.

AC_DEFUN(jm_FUNC_READDIR,
[dnl
AC_REQUIRE([AC_HEADER_DIRENT])
AC_CHECK_HEADERS(string.h)
AC_CACHE_CHECK([for working readdir], jm_cv_func_working_readdir,
  [dnl
  # Arrange for deletion of the temporary directory this test creates, in
  # case the test itself fails to delete everything -- as happens on Sunos.
  ac_clean_files="$ac_clean_files conf-dir"

  AC_TRY_RUN(
  changequote(<<, >>)dnl
  <<
#   include <stdio.h>
#   include <sys/types.h>
#   if HAVE_STRING_H
#    include <string.h>
#   endif

#   ifdef HAVE_DIRENT_H
#    include <dirent.h>
#    define NLENGTH(direct) (strlen((direct)->d_name))
#   else /* not HAVE_DIRENT_H */
#    define dirent direct
#    define NLENGTH(direct) ((direct)->d_namlen)
#    ifdef HAVE_SYS_NDIR_H
#     include <sys/ndir.h>
#    endif /* HAVE_SYS_NDIR_H */
#    ifdef HAVE_SYS_DIR_H
#     include <sys/dir.h>
#    endif /* HAVE_SYS_DIR_H */
#    ifdef HAVE_NDIR_H
#     include <ndir.h>
#    endif /* HAVE_NDIR_H */
#   endif /* HAVE_DIRENT_H */

#   define DOT_OR_DOTDOT(Basename) \
     (Basename[0] == '.' && (Basename[1] == '\0' \
			     || (Basename[1] == '.' && Basename[2] == '\0')))

    static void
    create_300_file_dir (const char *dir)
    {
      int i;

      if (mkdir (dir, 0700))
	abort ();
      if (chdir (dir))
	abort ();

      for (i = 0; i < 300; i++)
	{
	  char file_name[4];
	  FILE *out;

	  sprintf (file_name, "%03d", i);
	  out = fopen (file_name, "w");
	  if (!out)
	    abort ();
	  if (fclose (out) == EOF)
	    abort ();
	}

      if (chdir (".."))
	abort ();
    }

    static void
    remove_dir (const char *dir)
    {
      DIR *dirp;

      if (chdir (dir))
	abort ();

      dirp = opendir (".");
      if (dirp == NULL)
	abort ();

      while (1)
	{
	  struct dirent *dp = readdir (dirp);
	  if (dp == NULL)
	    break;

	  if (DOT_OR_DOTDOT (dp->d_name))
	    continue;

	  if (unlink (dp->d_name))
	    abort ();
	}
      closedir (dirp);

      if (chdir (".."))
	abort ();

      if (rmdir (dir))
	exit (1);
    }

    int
    main ()
    {
      const char *dir = "conf-dir";
      create_300_file_dir (dir);
      remove_dir (dir);
      exit (0);
    }
  >>,
  changequote([, ])dnl
  jm_cv_func_working_readdir=yes,
  jm_cv_func_working_readdir=no,
  jm_cv_func_working_readdir=no)])

  if test $jm_cv_func_working_readdir = yes; then
    AC_DEFINE_UNQUOTED(HAVE_WORKING_READDIR, 1,
[Define if readdir is found to work properly in some unusual cases. ])
  fi
])

#serial 3

dnl A replacement for autoconf's AC_FUNC_MEMCMP that detects
dnl the losing memcmp on some x86 Next systems.
AC_DEFUN(jm_AC_FUNC_MEMCMP,
[AC_CACHE_CHECK([for working memcmp], jm_cv_func_memcmp_working,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<
main()
{
  /* Some versions of memcmp are not 8-bit clean.  */
  char c0 = 0x40, c1 = 0x80, c2 = 0x81;
  if (memcmp(&c0, &c2, 1) >= 0 || memcmp(&c1, &c2, 1) >= 0)
    exit (1);

  /* The Next x86 OpenStep bug shows up only when comparing 16 bytes
     or more and with at least one buffer not starting on a 4-byte boundary.
     William Lewis provided this test program.   */
  {
    char foo[21];
    char bar[21];
    int i;
    for (i = 0; i < 4; i++)
      {
	char *a = foo + i;
	char *b = bar + i;
	strcpy (a, "--------01111111");
	strcpy (b, "--------10000000");
	if (memcmp (a, b, 16) >= 0)
	  exit (1);
      }
    exit (0);
  }
}
>>,
changequote([, ])dnl
   jm_cv_func_memcmp_working=yes,
   jm_cv_func_memcmp_working=no,
   jm_cv_func_memcmp_working=no)])
test $jm_cv_func_memcmp_working = no \
  && LIBOBJS="$LIBOBJS memcmp.$ac_objext"
AC_SUBST(LIBOBJS)dnl
])

AC_DEFUN(jm_FUNC_MEMCMP,
[AC_REQUIRE([jm_AC_FUNC_MEMCMP])dnl
 if test $jm_cv_func_memcmp_working = no; then
   AC_DEFINE_UNQUOTED(memcmp, rpl_memcmp,
     [Define to rpl_memcmp if the replacement function should be used.])
 fi
])

#serial 2

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros are available.
dnl Use only those *_unlocked macros that are declared.
dnl

AC_DEFUN(jm_FUNC_GLIBC_UNLOCKED_IO,
  [
    io_functions='clearerr_unlocked feof_unlocked ferror_unlocked
    fflush_unlocked fputc_unlocked fread_unlocked fwrite_unlocked
    getc_unlocked getchar_unlocked putc_unlocked putchar_unlocked'
    for jm_io_func in $io_functions; do
      # Check for the existence of each function only if its declared.
      # Otherwise, we'd get the Solaris5.5.1 functions that are not
      # declared, and that have been removed from Solaris5.6.  The resulting
      # 5.5.1 binaries would not run on 5.6 due to shared library differences.
      jm_CHECK_DECLARATIONS([#include <stdio.h>
			    ], $jm_io_func,
			    jm_declared=yes,
			    jm_declared=no)
      if test $jm_declared = yes; then
        AC_CHECK_FUNCS($jm_io_func)
      fi
    done
  ]
)

#serial 2

dnl Determine whether to add fnmatch.o to LIBOBJS and to
dnl define fnmatch to rpl_fnmatch.
dnl

AC_DEFUN(jm_FUNC_FNMATCH,
[
  AC_REQUIRE([AM_GLIBC])
  AC_FUNC_FNMATCH
  if test $ac_cv_func_fnmatch_works = no \
      && test $ac_cv_gnu_library = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS fnmatch.$ac_objext"
    AC_DEFINE_UNQUOTED(fnmatch, rpl_fnmatch,
      [Define to rpl_fnmatch if the replacement function should be used.])
  fi
])

#serial 2

dnl From Gordon Matzigkeit.
dnl Test for the GNU C Library.
dnl FIXME: this should migrate into libit.

AC_DEFUN(AM_GLIBC,
  [
    AC_CACHE_CHECK(whether we are using the GNU C Library,
      ac_cv_gnu_library,
      [AC_EGREP_CPP([Thanks for using GNU],
	[
#include <features.h>
#ifdef __GNU_LIBRARY__
  Thanks for using GNU
#endif
	],
	ac_cv_gnu_library=yes,
	ac_cv_gnu_library=no)
      ]
    )
    AC_CACHE_CHECK(for version 2 of the GNU C Library,
      ac_cv_glibc,
      [AC_EGREP_CPP([Thanks for using GNU too],
	[
#include <features.h>
#ifdef __GLIBC__
  Thanks for using GNU too
#endif
	],
	ac_cv_glibc=yes, ac_cv_glibc=no)
      ]
    )
  ]
)

#serial 1

AC_DEFUN(jm_AFS,
  AC_CHECKING(for AFS)
  test -d /afs \
    && AC_DEFINE(AFS, 1, [Define if you have the Andrew File System.])
)

#serial 2

# autoconf tests required for use of xstrtoumax.c

AC_DEFUN(jm_AC_PREREQ_XSTRTOUMAX,
[
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_HEADERS(stdlib.h)

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoumax as a macro],
    jm_cv_func_strtoumax_macro,
    AC_EGREP_CPP([inttypes_h_defines_strtoumax], [#include <inttypes.h>
#ifdef strtoumax
 inttypes_h_defines_strtoumax
#endif],
      jm_cv_func_strtoumax_macro=yes,
      jm_cv_func_strtoumax_macro=no))

  if test "$jm_cv_func_strtoumax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoumax)
  fi

  dnl We don't need (and can't compile) the replacement strtoull
  dnl unless the type `unsigned long long' exists.
  dnl Also, only the replacement strtoumax invokes strtoull,
  dnl so we need the replacement strtoull only if strtoumax does not exist.
  case "$ac_cv_type_unsigned_long_long,$jm_cv_func_strtoumax_macro,$ac_cv_func_strtoumax" in
    yes,no,no)
      AC_REPLACE_FUNCS(strtoull)
      ;;
  esac

])

#serial 2

dnl From Paul Eggert.

AC_DEFUN(jm_AC_TYPE_UNSIGNED_LONG_LONG,
[
  AC_CACHE_CHECK([for unsigned long long], ac_cv_type_unsigned_long_long,
  [AC_TRY_LINK([unsigned long long ull = 1; int i = 63;],
    [unsigned long long ullmax = (unsigned long long) -1;
     return ull << i | ull >> i | ullmax / ull | ullmax % ull;],
    ac_cv_type_unsigned_long_long=yes,
    ac_cv_type_unsigned_long_long=no)])
  if test $ac_cv_type_unsigned_long_long = yes; then
    AC_DEFINE(HAVE_UNSIGNED_LONG_LONG, 1,
      [Define if you have the unsigned long long type.])
  fi
])

#serial 3

dnl See if there's a working, system-supplied version of the getline function.
dnl We can't just do AC_REPLACE_FUNCS(getline) because some systems
dnl have a function by that name in -linet that doesn't have anything
dnl to do with the function we need.
AC_DEFUN(AM_FUNC_GETLINE,
[dnl
  am_getline_needs_run_time_check=no
  AC_CHECK_FUNC(getline,
		dnl Found it in some library.  Verify that it works.
		am_getline_needs_run_time_check=yes,
		am_cv_func_working_getline=no)
  if test $am_getline_needs_run_time_check = yes; then
    AC_CHECK_HEADERS(string.h)
    AC_CACHE_CHECK([for working getline function], am_cv_func_working_getline,
    [echo fooN |tr -d '\012'|tr N '\012' > conftest.data
    AC_TRY_RUN([
#    include <stdio.h>
#    include <sys/types.h>
#    if HAVE_STRING_H
#     include <string.h>
#    endif
    int main ()
    { /* Based on a test program from Karl Heuer.  */
      char *line = NULL;
      size_t siz = 0;
      int len;
      FILE *in = fopen ("./conftest.data", "r");
      if (!in)
	return 1;
      len = getline (&line, &siz, in);
      exit ((len == 4 && line && strcmp (line, "foo\n") == 0) ? 0 : 1);
    }
    ], am_cv_func_working_getline=yes dnl The library version works.
    , am_cv_func_working_getline=no dnl The library version does NOT work.
    , am_cv_func_working_getline=no dnl We're cross compiling.
    )])
  fi

  if test $am_cv_func_working_getline = no; then
    LIBOBJS="$LIBOBJS getline.$ac_objext"
    AC_SUBST(LIBOBJS)dnl
  fi
])

#serial 2

dnl Just like AC_CHECK_TYPE from autoconf-2.12, but also checks in unistd.h
dnl on systems that have it.  Fujitsu UXP/V needs this for ssize_t.
dnl Now, also uses the three-argument form of AC_DEFINE.

undefine([AC_CHECK_TYPE])
dnl AC_CHECK_TYPE(TYPE, DEFAULT)
AC_DEFUN(AC_CHECK_TYPE,
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_CHECK_HEADERS(unistd.h)
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(ac_cv_type_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif], ac_cv_type_$1=yes, ac_cv_type_$1=no)])dnl
AC_MSG_RESULT($ac_cv_type_$1)
if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1, $2,
    [  Define to \`$2' if certain system header files doesn't define it.])
fi
])

#serial 3

dnl From Paul Eggert.

AC_PREREQ(2.13)

# Define uintmax_t to `unsigned long' or `unsigned long long'
# if <inttypes.h> does not exist.

AC_DEFUN(jm_AC_TYPE_UINTMAX_T,
[
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  if test $jm_ac_cv_header_inttypes_h = no; then
    AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
    test $ac_cv_type_unsigned_long_long = yes \
      && ac_type='unsigned long long' \
      || ac_type='unsigned long'
    AC_DEFINE_UNQUOTED(uintmax_t, $ac_type,
[  Define to \`unsigned long' or \`unsigned long long'
   if <inttypes.h> doesn't define.])
  fi
])

#serial 2

dnl A replacement for autoconf's macro by the same name.  This version
dnl uses `ac_lib' rather than `i' for the loop variable, but more importantly
dnl moves the ACTION-IF-FOUND ($3) into the inner `if'-block so that it is
dnl run only if one of the listed libraries ends up being used (and not in
dnl the `none required' case.
dnl I hope it's only temporary while we wait for that version to be fixed.
undefine([AC_SEARCH_LIBS])

dnl AC_SEARCH_LIBS(FUNCTION, SEARCH-LIBS [, ACTION-IF-FOUND
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])
dnl Search for a library defining FUNC, if it's not already available.

AC_DEFUN(AC_SEARCH_LIBS,
[
  AC_PREREQ([2.13])
  AC_CACHE_CHECK([for library containing $1], [ac_cv_search_$1],
  [
    ac_func_search_save_LIBS="$LIBS"
    ac_cv_search_$1="no"
    AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="none required"])
    if test "$ac_cv_search_$1" = "no"; then
      for ac_lib in $2; do
	LIBS="-l$ac_lib $5 $ac_func_search_save_LIBS"
	AC_TRY_LINK_FUNC([$1],
	  [ac_cv_search_$1="-l$ac_lib"
	  break])
      done
    fi
    LIBS="$ac_func_search_save_LIBS"
  ])

  if test "$ac_cv_search_$1" = "no"; then :
    $4
  else
    if test "$ac_cv_search_$1" = "none required"; then :
      $4
    else
      LIBS="$ac_cv_search_$1 $LIBS"
      $3
    fi
  fi
])

#serial 1

dnl Written by Jim Meyering

AC_DEFUN(jm_FUNC_GROUP_MEMBER,
  [
    dnl Do this replacement check manually because I want the hyphen
    dnl (not the underscore) in the filename.
    AC_CHECK_FUNC(group_member, , [LIBOBJS="$LIBOBJS group-member.$ac_objext"])
    AC_SUBST(LIBOBJS)
  ]
)

#serial 3

dnl From Jim Meyering.
dnl
dnl Check whether putenv ("FOO") removes FOO from the environment.
dnl The putenv in libc on at least SunOS 4.1.4 does *not* do that.
dnl

AC_DEFUN(jm_FUNC_PUTENV,
[AC_CACHE_CHECK([for SVID conformant putenv], jm_cv_func_svid_putenv,
  [AC_TRY_RUN([
    int
    main ()
    {
      /* Put it in env.  */
      if (putenv ("CONFTEST_putenv=val"))
        exit (1);

      /* Try to remove it.  */
      if (putenv ("CONFTEST_putenv"))
        exit (1);

      /* Make sure it was deleted.  */
      if (getenv ("CONFTEST_putenv") != 0)
        exit (1);

      exit (0);
    }
	      ],
	     jm_cv_func_svid_putenv=yes,
	     jm_cv_func_svid_putenv=no,
	     dnl When crosscompiling, assume putenv is broken.
	     jm_cv_func_svid_putenv=no)
  ])
  if test $jm_cv_func_svid_putenv = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS putenv.$ac_objext"
    AC_DEFINE_UNQUOTED(putenv, rpl_putenv,
      [Define to rpl_putenv if the replacement function should be used.])
  fi
])

dnl From Jim Meyering.  Use this if you use the GNU error.[ch].
dnl FIXME: Migrate into libit

AC_DEFUN(AM_FUNC_ERROR_AT_LINE,
[AC_CACHE_CHECK([for error_at_line], am_cv_lib_error_at_line,
 [AC_TRY_LINK([],[error_at_line(0, 0, "", 0, "");],
              am_cv_lib_error_at_line=yes,
	      am_cv_lib_error_at_line=no)])
 if test $am_cv_lib_error_at_line = no; then
   LIBOBJS="$LIBOBJS error.$ac_objext"
 fi
 AC_SUBST(LIBOBJS)dnl
])

#serial 6

dnl This macro is intended to be used solely in this file.
dnl These are the prerequisite macros for GNU's strftime.c replacement.
dnl FIXME: the list is far from complete
AC_DEFUN(_jm_STRFTIME_PREREQS,
[
 dnl strftime.c uses localtime_r if it exists.  Check for it.
 AC_CHECK_FUNCS(localtime_r)
 dnl FIXME: add tests for everything in strftime.c: e.g., HAVE_BCOPY,
 dnl HAVE_TZNAME, HAVE_TZSET, HAVE_TM_ZONE, etc.
])

dnl Determine if the strftime function has all the features of the GNU one.
dnl
dnl From Jim Meyering.
dnl
AC_DEFUN(jm_FUNC_GNU_STRFTIME,
[AC_REQUIRE([AC_HEADER_TIME])dnl

 _jm_STRFTIME_PREREQS

 AC_REQUIRE([AC_C_CONST])dnl
 AC_REQUIRE([AC_HEADER_STDC])dnl
 AC_CHECK_HEADERS(sys/time.h)
 AC_CACHE_CHECK([for working GNU strftime], jm_cv_func_working_gnu_strftime,
  [AC_TRY_RUN(
changequote(<<, >>)dnl
<< /* Ulrich Drepper provided parts of the test program.  */
#if STDC_HEADERS
# include <stdlib.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

static int
compare (const char *fmt, const struct tm *tm, const char *expected)
{
  char buf[99];
  strftime (buf, 99, fmt, tm);
  if (strcmp (buf, expected))
    {
#ifdef SHOW_FAILURES
      printf ("fmt: \"%s\", expected \"%s\", got \"%s\"\n",
	      fmt, expected, buf);
#endif
      return 1;
    }
  return 0;
}

int
main ()
{
  int n_fail = 0;
  struct tm *tm;
  time_t t = 738367; /* Fri Jan  9 13:06:07 1970 */
  tm = gmtime (&t);

  /* This is necessary to make strftime give consistent zone strings and
     e.g., seconds since the epoch (%s).  */
  putenv ("TZ=GMT0");

#undef CMP
#define CMP(Fmt, Expected) n_fail += compare ((Fmt), tm, (Expected))

  CMP ("%-m", "1");		/* GNU */
  CMP ("%A", "Friday");
  CMP ("%^A", "FRIDAY");	/* The ^ is a GNU extension.  */
  CMP ("%B", "January");
  CMP ("%^B", "JANUARY");
  CMP ("%C", "19");		/* POSIX.2 */
  CMP ("%D", "01/09/70");	/* POSIX.2 */
  CMP ("%F", "1970-01-09");
  CMP ("%G", "1970");		/* GNU */
  CMP ("%H", "13");
  CMP ("%I", "01");
  CMP ("%M", "06");
  CMP ("%M", "06");
  CMP ("%R", "13:06");		/* POSIX.2 */
  CMP ("%S", "07");
  CMP ("%T", "13:06:07");	/* POSIX.2 */
  CMP ("%U", "01");
  CMP ("%V", "02");
  CMP ("%W", "01");
  CMP ("%X", "13:06:07");
  CMP ("%Y", "1970");
  CMP ("%Z", "GMT");
  CMP ("%_m", " 1");		/* GNU */
  CMP ("%a", "Fri");
  CMP ("%^a", "FRI");
  CMP ("%b", "Jan");
  CMP ("%^b", "JAN");
  CMP ("%c", "Fri Jan  9 13:06:07 1970");
  CMP ("%^c", "FRI JAN  9 13:06:07 1970");
  CMP ("%d", "09");
  CMP ("%e", " 9");		/* POSIX.2 */
  CMP ("%g", "70");		/* GNU */
  CMP ("%h", "Jan");		/* POSIX.2 */
  CMP ("%^h", "JAN");
  CMP ("%j", "009");
  CMP ("%k", "13");		/* GNU */
  CMP ("%l", " 1");		/* GNU */
  CMP ("%m", "01");
  CMP ("%n", "\n");		/* POSIX.2 */
  CMP ("%p", "PM");
  CMP ("%r", "01:06:07 PM");	/* POSIX.2 */
  CMP ("%s", "738367");		/* GNU */
  CMP ("%t", "\t");		/* POSIX.2 */
  CMP ("%u", "5");		/* POSIX.2 */
  CMP ("%w", "5");
  CMP ("%x", "01/09/70");
  CMP ("%y", "70");
  CMP ("%z", "+0000");		/* GNU */

  exit (n_fail ? 1 : 0);
}
	      >>,
changequote([, ])dnl
	     jm_cv_func_working_gnu_strftime=yes,
             jm_cv_func_working_gnu_strftime=no,
	     dnl When crosscompiling, assume strftime is missing or broken.
	     jm_cv_func_working_gnu_strftime=no)
  ])
  if test $jm_cv_func_working_gnu_strftime = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS strftime.$ac_objext"
    AC_DEFINE_UNQUOTED(strftime, gnu_strftime,
      [Define to gnu_strftime if the replacement function should be used.])
  fi
])

AC_DEFUN(jm_FUNC_STRFTIME,
[
  _jm_STRFTIME_PREREQS
  AC_REPLACE_FUNCS(strftime)
])

#serial 3

dnl From Jim Meyering.
dnl
dnl Invoking code should check $GETGROUPS_LIB something like this:
dnl  jm_FUNC_GETGROUPS
dnl  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"
dnl

AC_DEFUN(jm_FUNC_GETGROUPS,
[AC_REQUIRE([AC_TYPE_GETGROUPS])dnl
 AC_REQUIRE([AC_TYPE_SIZE_T])dnl
 AC_CHECK_FUNCS(getgroups)

 # If we don't yet have getgroups, see if it's in -lbsd.
 # This is reported to be necessary on an ITOS 3000WS running SEIUX 3.1.
 if test $ac_cv_func_getgroups = no; then
   jm_cv_sys_getgroups_saved_lib="$LIBS"
   AC_CHECK_LIB(bsd, getgroups, [GETGROUPS_LIB=-lbsd])
   LIBS="$jm_cv_sys_getgroups_saved_lib"
 fi

 # Run the program to test the functionality of the system-supplied
 # getgroups function only if there is such a function.
 if test $ac_cv_func_getgroups = yes; then
   AC_CACHE_CHECK([for working getgroups], jm_cv_func_working_getgroups,
    [AC_TRY_RUN([
      int
      main ()
      {
	/* On Ultrix 4.3, getgroups (0, 0) always fails.  */
	exit (getgroups (0, 0) == -1 ? 1 : 0);
      }
		],
	       jm_cv_func_working_getgroups=yes,
	       jm_cv_func_working_getgroups=no,
	       dnl When crosscompiling, assume getgroups is broken.
	       jm_cv_func_working_getgroups=no)
    ])
    if test $jm_cv_func_working_getgroups = no; then
      AC_SUBST(LIBOBJS)
      LIBOBJS="$LIBOBJS getgroups.$ac_objext"
      AC_DEFINE_UNQUOTED(getgroups, rpl_getgroups,
	[Define as rpl_getgroups if getgroups doesn't work right.])
    fi
  fi
])

#serial 4

AC_DEFUN(AM_FUNC_GETLOADAVG,
[ac_have_func=no # yes means we've found a way to get the load average.

am_cv_saved_LIBS="$LIBS"

# On HPUX9, an unprivileged user can get load averages through this function.
AC_CHECK_FUNCS(pstat_getdynamic)

# Solaris has libkstat which does not require root.
AC_CHECK_LIB(kstat, kstat_open)
if test $ac_cv_lib_kstat_kstat_open = yes ; then ac_have_func=yes ; fi

# Some systems with -lutil have (and need) -lkvm as well, some do not.
# On Solaris, -lkvm requires nlist from -lelf, so check that first
# to get the right answer into the cache.
# For kstat on solaris, we need libelf to force the definition of SVR4 below.
AC_CHECK_LIB(elf, elf_begin, LIBS="-lelf $LIBS")
if test $ac_have_func = no; then
  AC_CHECK_LIB(kvm, kvm_open, LIBS="-lkvm $LIBS")
  # Check for the 4.4BSD definition of getloadavg.
  AC_CHECK_LIB(util, getloadavg,
    [LIBS="-lutil $LIBS" ac_have_func=yes ac_cv_func_getloadavg_setgid=yes])
fi

if test $ac_have_func = no; then
  # There is a commonly available library for RS/6000 AIX.
  # Since it is not a standard part of AIX, it might be installed locally.
  ac_save_LIBS="$LIBS"
  LIBS="-L/usr/local/lib $LIBS"
  AC_CHECK_LIB(getloadavg, getloadavg,
    LIBS="-lgetloadavg $LIBS", LIBS="$ac_save_LIBS")
fi

# Make sure it is really in the library, if we think we found it.
AC_REPLACE_FUNCS(getloadavg)

if test $ac_cv_func_getloadavg = yes; then
  AC_DEFINE(HAVE_GETLOADAVG)
  ac_have_func=yes
else
  AC_DEFINE(C_GETLOADAVG, 1, [Define if using getloadavg.c.])
  # Figure out what our getloadavg.c needs.
  ac_have_func=no
  AC_CHECK_HEADER(sys/dg_sys_info.h,
  [ac_have_func=yes; AC_DEFINE(DGUX)
  AC_CHECK_LIB(dgc, dg_sys_info)])

  AC_CHECK_HEADER(locale.h)
  AC_CHECK_FUNCS(setlocale)

  # We cannot check for <dwarf.h>, because Solaris 2 does not use dwarf (it
  # uses stabs), but it is still SVR4.  We cannot check for <elf.h> because
  # Irix 4.0.5F has the header but not the library.
  if test $ac_have_func = no && test $ac_cv_lib_elf_elf_begin = yes; then
    ac_have_func=yes; AC_DEFINE(SVR4)
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADER(inq_stats/cpustats.h,
    [ac_have_func=yes; AC_DEFINE(UMAX)
    AC_DEFINE(UMAX4_3)])
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADER(sys/cpustats.h,
    [ac_have_func=yes; AC_DEFINE(UMAX)])
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADERS(mach/mach.h)
  fi

  AC_CHECK_HEADER(nlist.h,
  [AC_DEFINE(NLIST_STRUCT)
  AC_CACHE_CHECK([for n_un in struct nlist], ac_cv_struct_nlist_n_un,
  [AC_TRY_COMPILE([#include <nlist.h>],
  [struct nlist n; n.n_un.n_name = 0;],
  ac_cv_struct_nlist_n_un=yes, ac_cv_struct_nlist_n_un=no)])
  if test $ac_cv_struct_nlist_n_un = yes; then
    AC_DEFINE(NLIST_NAME_UNION)
  fi
  ])dnl
fi # Do not have getloadavg in system libraries.

# Some definitions of getloadavg require that the program be installed setgid.
dnl FIXME Don't hardwire the path of getloadavg.c in the top-level directory.
AC_CACHE_CHECK(whether getloadavg requires setgid,
  ac_cv_func_getloadavg_setgid,
[AC_EGREP_CPP([Yowza Am I SETGID yet],
[#include "$srcdir/lib/getloadavg.c"
#ifdef LDAV_PRIVILEGED
Yowza Am I SETGID yet
#endif],
  ac_cv_func_getloadavg_setgid=yes, ac_cv_func_getloadavg_setgid=no)])
if test $ac_cv_func_getloadavg_setgid = yes; then
  NEED_SETGID=true; AC_DEFINE(GETLOADAVG_PRIVILEGED)
else
  NEED_SETGID=false
fi
AC_SUBST(NEED_SETGID)dnl

if test $ac_cv_func_getloadavg_setgid = yes; then
  AC_CACHE_CHECK(group of /dev/kmem, ac_cv_group_kmem,
changequote(<<, >>)dnl
<<
  # On Solaris, /dev/kmem is a symlink.  Get info on the real file.
  ac_ls_output=`ls -lgL /dev/kmem 2>/dev/null`
  # If we got an error (system does not support symlinks), try without -L.
  test -z "$ac_ls_output" && ac_ls_output=`ls -lg /dev/kmem`
  ac_cv_group_kmem=`echo $ac_ls_output \
    | sed -ne 's/[ 	][ 	]*/ /g;
	       s/^.[sSrwx-]* *[0-9]* *\([^0-9]*\)  *.*/\1/;
	       / /s/.* //;p;'`
>>
changequote([, ])dnl
)
  KMEM_GROUP=$ac_cv_group_kmem
fi
AC_SUBST(KMEM_GROUP)dnl

if test x = "x$am_cv_saved_LIBS"; then
  GETLOADAVG_LIBS="$LIBS"
else
  GETLOADAVG_LIBS=`echo "$LIBS"|sed "s!$am_cv_saved_LIBS!!"`
fi
AC_SUBST(GETLOADAVG_LIBS)dnl
LIBS="$am_cv_saved_LIBS"
])

#serial 2

AC_PREREQ(2.13)

AC_DEFUN(jm_SYS_PROC_UPTIME,
[ dnl Require AC_PROG_CC to see if we're cross compiling.
  AC_REQUIRE([AC_PROG_CC])
  AC_CACHE_CHECK([for /proc/uptime], jm_cv_have_proc_uptime,
  [jm_cv_have_proc_uptime=no
    test -f /proc/uptime \
      && test $ac_cv_prog_cc_cross = no \
      && cat < /proc/uptime >/dev/null 2>/dev/null \
      && jm_cv_have_proc_uptime=yes])
  if test $jm_cv_have_proc_uptime = yes; then
    AC_DEFINE(HAVE_PROC_UPTIME, 1,
	      [  Define if your system has the /proc/uptime special file.])
  fi
])

dnl From Jim Meyering.

# serial 1

AC_DEFUN(AM_SYS_POSIX_TERMIOS,
[AC_CACHE_CHECK([POSIX termios], am_cv_sys_posix_termios,
  [AC_TRY_LINK([#include <sys/types.h>
#include <unistd.h>
#include <termios.h>],
  [/* SunOS 4.0.3 has termios.h but not the library calls.  */
   tcgetattr(0, 0);],
  am_cv_sys_posix_termios=yes,
  am_cv_sys_posix_termios=no)])
])

#serial 3

AC_DEFUN(jm_HEADER_TIOCGWINSZ_NEEDS_SYS_IOCTL,
[AC_REQUIRE([jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires sys/ioctl.h],
	        jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h,
  [jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=no

  if test $jm_cv_sys_tiocgwinsz_needs_termios_h = no; then
    AC_EGREP_CPP([yes],
    [#include <sys/types.h>
#     include <sys/ioctl.h>
#     ifdef TIOCGWINSZ
        yes
#     endif
    ], jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=yes)
  fi
  ])
  if test $jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h = yes; then
    AC_DEFINE(GWINSZ_IN_SYS_IOCTL, 1,
      [Define if your system defines TIOCGWINSZ in sys/ioctl.h.])
  fi
])

dnl From Jim Meyering.
#serial 2
AC_DEFUN(jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H,
[AC_REQUIRE([AM_SYS_POSIX_TERMIOS])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires termios.h],
	        jm_cv_sys_tiocgwinsz_needs_termios_h,
  [jm_cv_sys_tiocgwinsz_needs_termios_h=no

   if test $am_cv_sys_posix_termios = yes; then
     AC_EGREP_CPP([yes],
     [#include <sys/types.h>
#      include <termios.h>
#      ifdef TIOCGWINSZ
         yes
#      endif
     ], jm_cv_sys_tiocgwinsz_needs_termios_h=yes)
   fi
  ])
])

AC_DEFUN(jm_WINSIZE_IN_PTEM,
  [AC_CHECK_HEADER([sys/ptem.h],
		   AC_DEFINE(WINSIZE_IN_PTEM, 1,
      [Define if your system defines \`struct winsize' in sys/ptem.h.]))
  ]
)








AC_DEFUN(AM_FUNC_STRTOD,
[AC_CACHE_CHECK(for working strtod, am_cv_func_strtod,
[AC_TRY_RUN([
double strtod ();
int
main()
{
  {
    /* Some versions of Linux strtod mis-parse strings with leading '+'.  */
    char *string = " +69";
    char *term;
    double value;
    value = strtod (string, &term);
    if (value != 69 || term != (string + 4))
      exit (1);
  }

  {
    /* Under Solaris 2.4, strtod returns the wrong value for the
       terminating character under some conditions.  */
    char *string = "NaN";
    char *term;
    strtod (string, &term);
    if (term != string && *(term - 1) == 0)
      exit (1);
  }
  exit (0);
}
], am_cv_func_strtod=yes, am_cv_func_strtod=no, am_cv_func_strtod=no)])
test $am_cv_func_strtod = no && LIBOBJS="$LIBOBJS strtod.$ac_objext"
AC_SUBST(LIBOBJS)dnl
am_cv_func_strtod_needs_libm=no
if test $am_cv_func_strtod = no; then
  AC_CHECK_FUNCS(pow)
  if test $ac_cv_func_pow = no; then
    AC_CHECK_LIB(m, pow, [am_cv_func_strtod_needs_libm=yes],
		 [AC_MSG_WARN(can't find library containing definition of pow)])
  fi
fi
])

# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 107

AC_PREREQ(2.13)               dnl Minimum Autoconf version required.

AC_DEFUN(AM_WITH_NLS,
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      USE_NLS=$enableval, USE_NLS=yes)
    AC_MSG_RESULT($USE_NLS)
    AC_SUBST(USE_NLS)

    USE_INCLUDED_LIBINTL=no

    dnl If we use NLS figure out what method
    if test "$USE_NLS" = "yes"; then
      AC_DEFINE(ENABLE_NLS, 1, [Define to 1 if NLS is requested.])
      AC_MSG_CHECKING([whether included gettext is requested])
      AC_ARG_WITH(included-gettext,
        [  --with-included-gettext use the GNU gettext library included here],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
      if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If gettext or catgets are available (in this order) we
        dnl use this.  Else we have to fall back to GNU NLS library.
	dnl catgets is only used if permitted by option --with-catgets.
	nls_cv_header_intl=
	nls_cv_header_libgt=
	CATOBJEXT=NONE

	AC_CHECK_HEADER(libintl.h,
	  [AC_CACHE_CHECK([for gettext in libc], gt_cv_func_gettext_libc,
	    [AC_TRY_LINK([#include <libintl.h>], [return (int) gettext ("")],
	       gt_cv_func_gettext_libc=yes, gt_cv_func_gettext_libc=no)])

	   if test "$gt_cv_func_gettext_libc" != "yes"; then
	     AC_CHECK_LIB(intl, bindtextdomain,
	       [AC_CHECK_LIB(intl, gettext)])
	   fi

	   if test "$gt_cv_func_gettext_libc" = "yes" \
	      || test "$ac_cv_lib_intl_gettext" = "yes"; then
	      AC_DEFINE(HAVE_GETTEXT, 1,
	  [Define to 1 if you have gettext and don't want to use GNU gettext.])
	      AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
		[test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], no)dnl
	      if test "$MSGFMT" != "no"; then
		AC_CHECK_FUNCS(dcgettext)
		AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
		AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
		  [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
		AC_TRY_LINK(, [extern int _nl_msg_cat_cntr;
			       return _nl_msg_cat_cntr],
		  [CATOBJEXT=.gmo
		   DATADIRNAME=share],
		  [CATOBJEXT=.mo
		   DATADIRNAME=lib])
		INSTOBJEXT=.mo
	      fi
	    fi
	])

        if test "$CATOBJEXT" = "NONE"; then
	  AC_MSG_CHECKING([whether catgets can be used])
	  AC_ARG_WITH(catgets,
	    [  --with-catgets          use catgets functions if available],
	    nls_cv_use_catgets=$withval, nls_cv_use_catgets=no)
	  AC_MSG_RESULT($nls_cv_use_catgets)

	  if test "$nls_cv_use_catgets" = "yes"; then
	    dnl No gettext in C library.  Try catgets next.
	    AC_CHECK_LIB(i, main)
	    AC_CHECK_FUNC(catgets,
	      [AC_DEFINE(HAVE_CATGETS, 1,
			 [Define as 1 if you have catgets and don't want to use GNU gettext.])
	       INTLOBJS="\$(CATOBJS)"
	       AC_PATH_PROG(GENCAT, gencat, no)dnl
	       if test "$GENCAT" != "no"; then
		 AC_PATH_PROG(GMSGFMT, gmsgfmt, no)
		 if test "$GMSGFMT" = "no"; then
		   AM_PATH_PROG_WITH_TEST(GMSGFMT, msgfmt,
		    [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], no)
		 fi
		 AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
		   [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
		 USE_INCLUDED_LIBINTL=yes
		 CATOBJEXT=.cat
		 INSTOBJEXT=.cat
		 DATADIRNAME=lib
		 INTLDEPS='$(top_builddir)/intl/libintl.a'
		 INTLLIBS=$INTLDEPS
		 LIBS=`echo $LIBS | sed -e 's/-lintl//'`
		 nls_cv_header_intl=intl/libintl.h
		 nls_cv_header_libgt=intl/libgettext.h
	       fi])
	  fi
        fi

        if test "$CATOBJEXT" = "NONE"; then
	  dnl Neither gettext nor catgets in included in the C library.
	  dnl Fall back on GNU gettext library.
	  nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	  [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], msgfmt)
        AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
        AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	  [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
        AC_SUBST(MSGFMT)
	USE_INCLUDED_LIBINTL=yes
        CATOBJEXT=.gmo
        INSTOBJEXT=.mo
        DATADIRNAME=share
	INTLDEPS='$(top_builddir)/intl/libintl.a'
	INTLLIBS=$INTLDEPS
	LIBS=`echo $LIBS | sed -e 's/-lintl//'`
        nls_cv_header_intl=intl/libintl.h
        nls_cv_header_libgt=intl/libgettext.h
      fi

      dnl Test whether we really found GNU xgettext.
      if test "$XGETTEXT" != ":"; then
	dnl If it is no GNU xgettext we define it as : so that the
	dnl Makefiles still can work.
	if $XGETTEXT --omit-header /dev/null 2> /dev/null; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found xgettext program is not GNU xgettext; ignore it])
	  XGETTEXT=":"
	fi
      fi

      # We need to process the po/ directory.
      POSUB=po
    else
      DATADIRNAME=share
      nls_cv_header_intl=intl/libintl.h
      nls_cv_header_libgt=intl/libgettext.h
    fi
    if test -z "$nls_cv_header_intl"; then
      # Clean out junk possibly left behind by a previous configuration.
      rm -f intl/libintl.h
    fi
    AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)
    AC_OUTPUT_COMMANDS(
     [case "$CONFIG_FILES" in *po/Makefile.in*)
        sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile
      esac])


    # If this is used in GNU gettext we have to set USE_NLS to `yes'
    # because some of the sources are only built for this goal.
    if test "$PACKAGE" = gettext; then
      USE_NLS=yes
      USE_INCLUDED_LIBINTL=yes
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(USE_INCLUDED_LIBINTL)
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(DATADIRNAME)
    AC_SUBST(GMOFILES)
    AC_SUBST(INSTOBJEXT)
    AC_SUBST(INTLDEPS)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)
  ])

AC_DEFUN(AM_GNU_GETTEXT,
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_ISC_POSIX])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl

   AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h string.h \
unistd.h sys/param.h])
   AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp \
strdup __argz_count __argz_stringify __argz_next])

   if test "${ac_cv_func_stpcpy+set}" != "set"; then
     AC_CHECK_FUNCS(stpcpy)
   fi
   if test "${ac_cv_func_stpcpy}" = "yes"; then
     AC_DEFINE(HAVE_STPCPY, 1, [Define to 1 if you have the stpcpy function.])
   fi

   AM_LC_MESSAGES
   AM_WITH_NLS

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for lang in ${LINGUAS=$ALL_LINGUAS}; do
         case "$ALL_LINGUAS" in
          *$lang*) NEW_LINGUAS="$NEW_LINGUAS $lang" ;;
         esac
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl The reference to <locale.h> in the installed <libintl.h> file
   dnl must be resolved because we cannot expect the users of this
   dnl to define HAVE_LOCALE_H.
   if test $ac_cv_header_locale_h = yes; then
     INCLUDE_LOCALE_H="#include <locale.h>"
   else
     INCLUDE_LOCALE_H="\
/* The system does not provide the header <locale.h>.  Take care yourself.  */"
   fi
   AC_SUBST(INCLUDE_LOCALE_H)

   dnl Determine which catalog format we have (if any is needed)
   dnl For now we know about two different formats:
   dnl   Linux libc-5 and the normal X/Open format
   test -d intl || mkdir intl
   if test "$CATOBJEXT" = ".cat"; then
     AC_CHECK_HEADER(linux/version.h, msgformat=linux, msgformat=xopen)

     dnl Transform the SED scripts while copying because some dumb SEDs
     dnl cannot handle comments.
     sed -e '/^#/d' $srcdir/intl/$msgformat-msg.sed > intl/po2msg.sed
   fi
   dnl po2tbl.sed is always needed.
   sed -e '/^#.*[^\\]$/d' -e '/^#$/d' \
     $srcdir/intl/po2tbl.sed.in > intl/po2tbl.sed

   dnl In the intl/Makefile.in we have a special dependency which makes
   dnl only sense for gettext.  We comment this out for non-gettext
   dnl packages.
   if test "$PACKAGE" = "gettext"; then
     GT_NO="#NO#"
     GT_YES=
   else
     GT_NO=
     GT_YES="#YES#"
   fi
   AC_SUBST(GT_NO)
   AC_SUBST(GT_YES)

   dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
   dnl find the mkinstalldirs script in another subdir but ($top_srcdir).
   dnl Try to locate is.
   MKINSTALLDIRS=
   if test -n "$ac_aux_dir"; then
     MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
   fi
   if test -z "$MKINSTALLDIRS"; then
     MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
   fi
   AC_SUBST(MKINSTALLDIRS)

   dnl *** For now the libtool support in intl/Makefile is not for real.
   l=
   AC_SUBST(l)

   dnl Generate list of files to be processed by xgettext which will
   dnl be included in po/Makefile.
   test -d po || mkdir po
   changequote(, )dnl
   case "$srcdir" in
   .) 
     posrcprefix="../" ;;
   /* | [A-Za-z]:*)
     posrcprefix="$srcdir/" ;;
   *)
     posrcprefix="../$srcdir/" ;;
   esac
   changequote([, ])dnl
   rm -f po/POTFILES
   sed -e "/^#/d" -e "/^\$/d" -e "s,.*,	$posrcprefix& \\\\," -e "\$s/\(.*\) \\\\/\1/" \
	< $srcdir/po/POTFILES.in > po/POTFILES
  ])

# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 1

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN(AM_PATH_PROG_WITH_TEST,
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
	ac_cv_path_$1="$ac_dir/$ac_word"
	break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test -n "[$]$1"; then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 2

AC_PREREQ(2.13)               dnl Minimum Autoconf version required.

AC_DEFUN(AM_LC_MESSAGES,
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES, 1,
		[Define if your locale.h file contains LC_MESSAGES.])
    fi
  fi])

