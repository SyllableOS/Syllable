/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if your system has a working fnmatch function.  */
#define HAVE_FNMATCH 1

/* Define if you have a working `mmap' system call.  */
/* #undef HAVE_MMAP */

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
/* #undef MAJOR_IN_MKDEV */

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
/* #undef MAJOR_IN_SYSMACROS */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* This is unconditionally defined for setting a GNU environment.  */
#define _GNU_SOURCE 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef daddr_t */

/* Define to a string giving the full name of the default archive file.  */
#define DEFAULT_ARCHIVE "-"

/* Define to a number giving the default blocking size for archives.  */
#define DEFAULT_BLOCKING 20

/* Define to 1 if density may be indicated by [lmh] at end of device.  */
/* #undef DENSITY_LETTER */

/* Define to a string giving the prefix of the default device, without the
   part specifying the unit and density.  */
/* #undef DEVICE_PREFIX */

/* Define to 1 if you lack a 3-argument version of open, and want to
   emulate it with system calls you do have.  */
/* #undef EMUL_OPEN3 */

/* Define to 1 if you have getgrgid(3).  */
#define HAVE_GETGRGID 1

/* Define to 1 if you have getpwuid(3).  */
#define HAVE_GETPWUID 1

/* Define to 1 if mknod function is available.  */
#define HAVE_MKNOD 1

/* Define if struct stat has a char st_fstype[] member.  */
/* #undef HAVE_ST_FSTYPE_STRING */

/* Define if `union wait' is the type of the first arg to wait functions.  */
/* #undef HAVE_UNION_WAIT */

/* Define to 1 if utime.h exists and declares struct utimbuf.  */
#define HAVE_UTIME_H 1

/* Define to `int' if <sys/types.h> doesn't define.  */
#define major_t int

/* Define to `int' if <sys/types.h> doesn't define.  */
#define minor_t int

/* Define to mt_model (v.g., for DG/UX), else to mt_type.  */
#define MTIO_CHECK_FIELD mt_type

/* Define to the full path of your rsh, if any.  */
/* #undef REMOTE_SHELL */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef ssize_t */

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* The number of bytes in a unsigned long.  */
#define SIZEOF_UNSIGNED_LONG 4

/* Define if you have the __argz_count function.  */
#define HAVE___ARGZ_COUNT 1

/* Define if you have the __argz_next function.  */
#define HAVE___ARGZ_NEXT 1

/* Define if you have the __argz_stringify function.  */
#define HAVE___ARGZ_STRINGIFY 1

/* Define if you have the alarm function.  */
#define HAVE_ALARM 1

/* Define if you have the dcgettext function.  */
/* #undef HAVE_DCGETTEXT */

/* Define if you have the execlp function.  */
#define HAVE_EXECLP 1

/* Define if you have the fsync function.  */
#define HAVE_FSYNC 1

/* Define if you have the ftime function.  */
#define HAVE_FTIME 1

/* Define if you have the ftruncate function.  */
/* #undef HAVE_FTRUNCATE */

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the isascii function.  */
#define HAVE_ISASCII 1

/* Define if you have the lchown function.  */
/* #undef HAVE_LCHOWN */

/* Define if you have the localtime_r function.  */
#define HAVE_LOCALTIME_R 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the mkfifo function.  */
#define HAVE_MKFIFO 1

/* Define if you have the munmap function.  */
/* #undef HAVE_MUNMAP */

/* Define if you have the nap function.  */
/* #undef HAVE_NAP */

/* Define if you have the napms function.  */
/* #undef HAVE_NAPMS */

/* Define if you have the poll function.  */
#define HAVE_POLL 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the rename function.  */
#define HAVE_RENAME 1

/* Define if you have the rmdir function.  */
#define HAVE_RMDIR 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the stpcpy function.  */
#define HAVE_STPCPY 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the strtoull function.  */
/* #undef HAVE_STRTOULL */

/* Define if you have the strtoumax function.  */
#define HAVE_STRTOUMAX 1

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the <argz.h> header file.  */
#define HAVE_ARGZ_H 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <linux/fd.h> header file.  */
/* #undef HAVE_LINUX_FD_H */

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <net/errno.h> header file.  */
/* #undef HAVE_NET_ERRNO_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <nl_types.h> header file.  */
#define HAVE_NL_TYPES_H 1

/* Define if you have the <poll.h> header file.  */
#define HAVE_POLL_H 1

/* Define if you have the <sgtty.h> header file.  */
#define HAVE_SGTTY_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <stropts.h> header file.  */
#define HAVE_STROPTS_H 1

/* Define if you have the <sys/buf.h> header file.  */
/* #undef HAVE_SYS_BUF_H */

/* Define if you have the <sys/device.h> header file.  */
/* #undef HAVE_SYS_DEVICE_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/gentape.h> header file.  */
/* #undef HAVE_SYS_GENTAPE_H */

/* Define if you have the <sys/inet.h> header file.  */
/* #undef HAVE_SYS_INET_H */

/* Define if you have the <sys/io/trioctl.h> header file.  */
/* #undef HAVE_SYS_IO_TRIOCTL_H */

/* Define if you have the <sys/ioccom.h> header file.  */
/* #undef HAVE_SYS_IOCCOM_H */

/* Define if you have the <sys/mtio.h> header file.  */
#define HAVE_SYS_MTIO_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/tape.h> header file.  */
/* #undef HAVE_SYS_TAPE_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1

/* Define if you have the <sys/tprintf.h> header file.  */
/* #undef HAVE_SYS_TPRINTF_H */

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the i library (-li).  */
/* #undef HAVE_LIBI */

/* Define if you have the intl library (-lintl).  */
/* #undef HAVE_LIBINTL */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Name of package */
#define PACKAGE "tar"

/* Version number of package */
#define VERSION "1.13"

/* Number of bits in a file offset, on hosts where this is settable.
       case  in
	# HP-UX 10.20 and later
	hpux10.2-90-9* | hpux11-9* | hpux2-90-9*)
	  ac_cv_sys_file_offset_bits=64 ;;
	esac */
/* #undef _FILE_OFFSET_BITS */

/* Define to make fseeko etc. visible, on some hosts. */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define if compiler has function prototypes */
#define PROTOTYPES 1

/* Define if <inttypes.h> exists, doesn't clash with <sys/types.h>,
   and declares uintmax_t.  */
#define HAVE_INTTYPES_H 1

/* Define if there is a member named d_ino in the struct describing
   directory headers. */
#define D_INO_IN_DIRENT 1

/* Define if you have the unsigned long long type. */
#define HAVE_UNSIGNED_LONG_LONG 1

/*   Define to `unsigned long' or `unsigned long long'
   if <inttypes.h> doesn't define. */
/* #undef uintmax_t */

/* Define if the malloc check has been performed.  */
#define HAVE_DONE_WORKING_MALLOC_CHECK 1

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to rpl_mktime if the replacement function should be used. */
/* #undef mktime */

/* Define if the realloc check has been performed.  */
#define HAVE_DONE_WORKING_REALLOC_CHECK 1

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define if using the dmalloc debugging malloc package */
/* #undef WITH_DMALLOC */

/* Define to 1 if you have the stpcpy function. */
#define HAVE_STPCPY 1

/* Define if your locale.h file contains LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if NLS is requested. */
#define ENABLE_NLS 1

/* Define to 1 if you have gettext and don't want to use GNU gettext. */
#define HAVE_GETTEXT 1

/* Define as 1 if you have catgets and don't want to use GNU gettext. */
/* #undef HAVE_CATGETS */

