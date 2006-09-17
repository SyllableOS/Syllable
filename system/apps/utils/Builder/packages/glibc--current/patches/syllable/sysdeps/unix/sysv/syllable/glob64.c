#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>

#define dirent dirent64
#define __readdir(dirp) __readdir64 (dirp)

#define glob_t glob64_t
#define glob(pattern, flags, errfunc, pglob) \
  __glob64 (pattern, flags, errfunc, pglob)
#define globfree(pglob) globfree64 (pglob)

#undef stat
#define stat stat64
#undef __stat
#define __stat(file, buf) __xstat64 (_STAT_VER, file, buf)

#define NO_GLOB_PATTERN_P 1

#define COMPILE_GLOB64	1

#include <posix/glob.c>

#include "shlib-compat.h"

libc_hidden_def (globfree64)

versioned_symbol (libc, __glob64, glob64, GLIBC_2_2);
libc_hidden_ver (__glob64, glob64)

