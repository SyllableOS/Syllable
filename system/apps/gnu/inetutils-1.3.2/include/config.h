/* include/config.h.  Generated automatically by configure.  */
/* headers/config.h.in.  Generated automatically from configure.in by autoheader.  */
/* inetutils-specific things to put in config.h.in

  Copyright (C) 1996, 1997 Free Software Foundation, Inc.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Defining this causes gnu libc to act more like BSD.  */
#define _BSD_SOURCE


/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if you have <vfork.h>.  */
/* #undef HAVE_VFORK_H */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the setvbuf function takes the buffering type as its second
   argument and the buffer pointer as the third, as on System V
   before release 3.  */
/* #undef SETVBUF_REVERSED */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define vfork as fork if vfork does not work.  */
/* #undef vfork */

/* Define this symbol if authentication is enabled.  */
/* #undef AUTHENTICATION */

/* Define this symbol if connection encryption is enabled.  */
/* #undef ENCRYPTION */

/* Define this symbol if connection DES encryption is enabled.  */
/* #undef DES_ENCRYPTION */

/* Define this symbol if `struct osockaddr' is defined by <osockaddr.h>.  */
/* #undef HAVE_OSOCKADDR_H */

/* Define this symbol if `struct lastlog' is defined in <utmp.h>.  */
#define HAVE_STRUCT_LASTLOG 1

/* Define this this symbol if struct sockaddr_in has a sin_len field.  */
/* #undef HAVE_SOCKADDR_IN_SIN_LEN */

/* Define this symbol if in addition to the normal time fields in struct stat
   (st_mtime &c), there are additional fields `st_mtime_usec' &c.  */
/* #undef HAVE_STAT_ST_MTIME_USEC */

/* Define this if struct utmp has a ut_type field.  */
#define HAVE_UTMP_UT_TYPE 1
/* Define this if struct utmp has a ut_host field.  */
#define HAVE_UTMP_UT_HOST 1
/* Define this if struct utmp has a ut_tv field.  */
#define HAVE_UTMP_UT_TV 1
/* Define this if struct utmpx has a ut_tv field.  */
#define HAVE_UTMPX_UT_TV 1

/* Define this if using Kerberos version 4.  */
/* #undef KRB4 */

/* Define this to be `setpgrp' if on a BSD system that doesn't have setpgid. */
/* #undef setpgid */

/* Define this if __P is defined in <stdlib.h>.  */
#define HAVE___P 1

/* Define this if a definition of sys_errlist is available.  */
/* #undef HAVE_SYS_ERRLIST */

/* Define this if sys_errlist is declared in <stdio.h> or <errno.h>.  */
/* #undef HAVE_SYS_ERRLIST_DECL */

/* Define this if errno is declared by <errno.h>.  */
#define HAVE_ERRNO_DECL 1

/* Define this if ENVIRON is declared by <unistd.h> or <stdlib.h>.  */
/* #undef HAVE_ENVIRON_DECL */

/* Define this if hstrerror is declared in <netdb.h>.  */
#define HAVE_HSTRERROR_DECL 1

/* Define this if H_ERRLIST is declared in <netdb.h>  */
/* #undef HAVE_H_ERRLIST_DECL */

/* Define this if a definition of h_errno is available after including <netdb.h>.  */
#define HAVE_H_ERRNO 1

/* Define this if H_ERRNO is declared in <netdb.h>  */
#define HAVE_H_ERRNO_DECL 1

/* Define this if the system supplies the __PROGNAME variable.  */
#define HAVE___PROGNAME 1

/* Define this if the system defines snprintf.  */
#define HAVE_SNPRINTF 1

/* Define this if the system defines vsnprintf. */
#define HAVE_VSNPRINTF 1

/* Define this if sig_t is declared by including <sys/types.h> & <signal.h> */
#define HAVE_SIG_T 1

/* Define this to be `int' if sig_atomic_t isn't declared by including
   <sys/types.h> & <signal.h> */
/* #undef sig_atomic_t */
/* Define this to be `unsigned long' if sigset_t isn't declared by including
   <sys/types.h> & <signal.h> */
/* #undef sigset_t */

/* Define this if weak references of any sort are supported.  */
#define HAVE_WEAK_REFS 1
/* Define this if gcc-style weak references work: ... __attribute__ ((weak)) */
/* #undef HAVE_ATTR_WEAK_REFS */
/* Define this if #pragma weak references work: #pragma weak foo */
#define HAVE_PRAGMA_WEAK_REFS 1
/* Define this if gnu-as weak references work: asm (".weak foo") */
/* #undef HAVE_ASM_WEAK_REFS */

/* Define this if crypt is declared by including <unistd.h>.  */
/* #undef HAVE_CRYPT_DECL */

/* Define this if fclose is declared by including <stdio.h>.  */
#define HAVE_FCLOSE_DECL 1

/* Define this if pclose is declared by including <stdio.h>.  */
#define HAVE_PCLOSE_DECL 1

/* Define this if getpass is declared by including <stdlib.h>.  */
#define HAVE_GETPASS_DECL 1

/* Define this if getusershell is declared by including <stdlib.h>.  */
#define HAVE_GETUSERSHELL_DECL 1

/* Define this if strchr is declared by including <stdlib.h> & <string.h>.  */
#define HAVE_STRCHR_DECL 1

/* Define this if strerror is declared by including <stdlib.h> & <string.h>.  */
#define HAVE_STRERROR_DECL 1

/* Define this if htons is declared by including <sys/types.h>,
   <sys/param.h>, & <netinet/in.h>.  */
#define HAVE_HTONS_DECL 1

/* Define this if <paths.h> exists.  */
#define HAVE_PATHS_H 1

/* Define this if getcwd (0, 0) works.  */
#define HAVE_GETCWD_ZERO_SIZE 1

/* If these aren't defined by <unistd.h>, define them here. */
/* #undef SEEK_SET */
/* #undef SEEK_CUR */
/* #undef SEEK_END */

/* If the fd_set macros (FD_ZERO &c) are defined by including <sys/time.h> (a
   truly bizarre place), define this.  */
/* #undef HAVE_FD_SET_MACROS_IN_SYS_TIME_H */

/* If <syslog.h> declares special internal stuff when SYSLOG_NAMES is
   defined, define this.  */
#define HAVE_SYSLOG_INTERNAL 1

/* If EWOULDBLOCK isn't defined by <errno.h>, define it here.  */
/* #undef EWOULDBLOCK */

/* Define if you have the bcopy function.  */
#define HAVE_BCOPY 1

/* Define if you have the bzero function.  */
#define HAVE_BZERO 1

/* Define if you have the cfsetspeed function.  */
#define HAVE_CFSETSPEED 1

/* Define if you have the cgetent function.  */
/* #undef HAVE_CGETENT */

/* Define if you have the crypt function.  */
#define HAVE_CRYPT 1

/* Define if you have the daemon function.  */
#define HAVE_DAEMON 1

/* Define if you have the flock function.  */
#define HAVE_FLOCK 1

/* Define if you have the forkpty function.  */
#define HAVE_FORKPTY 1

/* Define if you have the fpathconf function.  */
#define HAVE_FPATHCONF 1

/* Define if you have the ftruncate function.  */
/* #undef HAVE_FTRUNCATE */

/* Define if you have the getusershell function.  */
#define HAVE_GETUSERSHELL 1

/* Define if you have the herror function.  */
#define HAVE_HERROR 1

/* Define if you have the iruserok function.  */
#define HAVE_IRUSEROK 1

/* Define if you have the killpg function.  */
#define HAVE_KILLPG 1

/* Define if you have the login function.  */
#define HAVE_LOGIN 1

/* Define if you have the login_tty function.  */
#define HAVE_LOGIN_TTY 1

/* Define if you have the logout function.  */
#define HAVE_LOGOUT 1

/* Define if you have the logwtmp function.  */
#define HAVE_LOGWTMP 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the openpty function.  */
#define HAVE_OPENPTY 1

/* Define if you have the revoke function.  */
/* #undef HAVE_REVOKE */

/* Define if you have the setegid function.  */
#define HAVE_SETEGID 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the seteuid function.  */
#define HAVE_SETEUID 1

/* Define if you have the setlinebuf function.  */
#define HAVE_SETLINEBUF 1

/* Define if you have the setregid function.  */
#define HAVE_SETREGID 1

/* Define if you have the setresgid function.  */
#define HAVE_SETRESGID 1

/* Define if you have the setresuid function.  */
#define HAVE_SETRESUID 1

/* Define if you have the setreuid function.  */
#define HAVE_SETREUID 1

/* Define if you have the setsid function.  */
#define HAVE_SETSID 1

/* Define if you have the setutent_r function.  */
/* #undef HAVE_SETUTENT_R */

/* Define if you have the sigaction function.  */
#define HAVE_SIGACTION 1

/* Define if you have the sigvec function.  */
#define HAVE_SIGVEC 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the tcgetattr function.  */
#define HAVE_TCGETATTR 1

/* Define if you have the uname function.  */
#define HAVE_UNAME 1

/* Define if you have the utime function.  */
#define HAVE_UTIME 1

/* Define if you have the utimes function.  */
#define HAVE_UTIMES 1

/* Define if you have the wait3 function.  */
#define HAVE_WAIT3 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the xstrdup function.  */
/* #undef HAVE_XSTRDUP */

/* Define if you have the <arpa/nameser.h> header file.  */
#define HAVE_ARPA_NAMESER_H 1

/* Define if you have the <des.h> header file.  */
/* #undef HAVE_DES_H */

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <glob.h> header file.  */
#define HAVE_GLOB_H 1

/* Define if you have the <krb.h> header file.  */
/* #undef HAVE_KRB_H */

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <netinet/in_systm.h> header file.  */
#define HAVE_NETINET_IN_SYSTM_H 1

/* Define if you have the <netinet/ip.h> header file.  */
#define HAVE_NETINET_IP_H 1

/* Define if you have the <netinet/ip_icmp.h> header file.  */
#define HAVE_NETINET_IP_ICMP_H 1

/* Define if you have the <netinet/ip_var.h> header file.  */
/* #undef HAVE_NETINET_IP_VAR_H */

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/cdefs.h> header file.  */
#define HAVE_SYS_CDEFS_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/filio.h> header file.  */
/* #undef HAVE_SYS_FILIO_H */

/* Define if you have the <sys/ioctl_compat.h> header file.  */
/* #undef HAVE_SYS_IOCTL_COMPAT_H */

/* Define if you have the <sys/msgbuf.h> header file.  */
/* #undef HAVE_SYS_MSGBUF_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/proc.h> header file.  */
/* #undef HAVE_SYS_PROC_H */

/* Define if you have the <sys/ptyvar.h> header file.  */
/* #undef HAVE_SYS_PTYVAR_H */

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/sockio.h> header file.  */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define if you have the <sys/stream.h> header file.  */
/* #undef HAVE_SYS_STREAM_H */

/* Define if you have the <sys/sysmacros.h> header file.  */
#define HAVE_SYS_SYSMACROS_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/tty.h> header file.  */
/* #undef HAVE_SYS_TTY_H */

/* Define if you have the <sys/utsname.h> header file.  */
#define HAVE_SYS_UTSNAME_H 1

/* Define if you have the <termio.h> header file.  */
#define HAVE_TERMIO_H 1

/* Define if you have the <termios.h> header file.  */
#define HAVE_TERMIOS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <utmp.h> header file.  */
#define HAVE_UTMP_H 1

/* Define if you have the <utmpx.h> header file.  */
#define HAVE_UTMPX_H 1

/* Define if you have the <vis.h> header file.  */
/* #undef HAVE_VIS_H */

/* Define if you have the bsd library (-lbsd).  */
/* #undef HAVE_LIBBSD */

/* Define if you have the nsl library (-lnsl).  */
#define HAVE_LIBNSL 1

/* Define if you have the resolv library (-lresolv).  */
#define HAVE_LIBRESOLV 1

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

#ifdef HAVE___P
/* The system defines __P; we tested for it in <sys/cdefs.h>, so include that
   if we can.  */
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#else /* !HAVE___P */
/* If the system includes don't seem to define __P, do it here instead.  */

#if defined (__GNUC__) || (defined (__STDC__) && __STDC__) || defined (__cplusplus)
#define	__P(args)	args	/* Use prototypes.  */
#else
#define	__P(args)	()	/* No prototypes.  */
#endif

#endif /* HAVE___P */

#ifndef HAVE_SIG_T
typedef RETSIGTYPE (*sig_t) ();
#endif

/* Define memory frobbing ops.  memmove is always defined, even if we have to
   do so ourselves, as is memset.  bcopy, memcpy, & bzero are defined in
   terms of those if necessary (since that can always be done with a macro). */

#ifndef HAVE_BCOPY
#define bcopy(f,t,z) memmove(t,f,z)
#endif
#ifndef HAVE_BZERO
#define bzero(x,z) memset(x,0,z)
#endif
#ifndef HAVE_MEMCPY
#define memcpy memmove
#endif

#ifndef HAVE_KILLPG
#define killpg(pid, sig) kill(-(pid), (sig))
#endif

#ifndef HAVE_SETEUID
#ifdef HAVE_SETREUID
#define seteuid(uid) setreuid(-1, (uid))
#else /* !HAVE_SETREUID */
#ifdef HAVE_SETRESUID
#define seteuid(uid) setresuid(-1, (uid), -1)
#endif /* HAVE_SETRESUID */
#endif /* HAVE_SETREUID */
#endif /* ! HAVE_SETEUID */

#ifndef HAVE_SETEGID
#ifdef HAVE_SETREGID
#define setegid(gid) setregid(-1, (gid))
#else /* !HAVE_SETREGID */
#ifdef HAVE_SETRESGID
#define setegid(gid) setresgid(-1, (gid), -1)
#endif /* HAVE_SETRESGID */
#endif /* HAVE_SETREGID */
#endif /* ! HAVE_SETEGID */

#if !defined(HAVE_MEMMOVE) || !defined(HAVE_MEMSET)
/* Make sure size_t is defined */
#include <sys/types.h>
#endif
#ifndef HAVE_MEMMOVE
/* Declare our own silly version.  */
extern void *memmove __P ((void *to, const void *from, size_t sz));
#endif
#ifndef HAVE_MEMSET
/* Declare our own silly version.  */
extern void memset __P ((void *mem, int val, size_t sz));
#endif

/* Define string index ops. */
#ifndef HAVE_STRCHR_DECL
extern char *strchr __P ((char *str, int ch));
extern char *strrchr __P ((char *str, int ch));
#endif
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif

#ifndef HAVE_VSNPRINTF
#include <sys/types.h>
#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
extern int vsnprintf __P ((char *, size_t, const char *, va_list));
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/types.h>
#include <sys/param.h>
#endif
/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#if !defined(DEV_BSIZE) && defined(BSIZE)
#define DEV_BSIZE BSIZE
#endif
#if !defined(DEV_BSIZE) && defined(BBSIZE) /* SGI */
#define DEV_BSIZE BBSIZE
#endif
#ifndef DEV_BSIZE
#define DEV_BSIZE 4096
#endif

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Optimal I/O blocksize for the file, in bytes. */
#ifndef HAVE_ST_BLKSIZE
# define ST_BLKSIZE(statbuf) DEV_BSIZE
#else /* HAVE_ST_BLKSIZE */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
                              ? (statbuf).st_blksize : DEV_BSIZE)
#endif /* HAVE_ST_BLKSIZE */

#ifndef HAVE_GETPASS_DECL
extern char *getpass __P((const char *));
#endif

#ifndef HAVE_HSTRERROR_DECL
extern const char *hstrerror __P ((int));
#endif

#ifndef HAVE_STRERROR_DECL
extern const char *strerror __P ((int));
#endif

/* Defaults for PATH_ variables.  */
#include <confpaths.h>
