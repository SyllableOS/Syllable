/* Copyright (C) 1991,92,93,1995-2001, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <stdio-common/_itoa.h>

static int getttyname_r (char *buf, size_t buflen,
			 dev_t mydev, ino64_t myino,
			 int save) internal_function;

static int
internal_function
getttyname_r (char *buf, size_t buflen, dev_t mydev, ino64_t myino, int save)
{
  struct stat64 st;
  DIR *dirstream;
  struct dirent *d;
  size_t devlen = strlen (buf);

  dirstream = __opendir (buf);
  if (dirstream == NULL)
    {
      return errno;
    }

  while ((d = __readdir (dirstream)) != NULL)
    if (d->d_fileno == myino)
      {
	char *cp;
	size_t needed = _D_EXACT_NAMLEN (d) + 1;

	if (needed > buflen)
	  {
	    (void) __closedir (dirstream);
	    __set_errno (ERANGE);
	    return ERANGE;
	  }

	cp = __stpncpy (buf + devlen, d->d_name, needed);
	cp[0] = '\0';

	if (__xstat64 (_STAT_VER, buf, &st) == 0
#ifdef _STATBUF_ST_RDEV
	    && S_ISCHR (st.st_mode) && st.st_rdev == mydev
#else
	    && d->d_fileno == myino && st.st_dev == mydev
#endif
	   )
	  {
	    (void) __closedir (dirstream);
	    __set_errno (save);
	    return 0;
	  }
      }

  (void) __closedir (dirstream);
  __set_errno (save);
  /* It is not clear what to return in this case.  `isatty' says FD
     refers to a TTY but no entry in /dev has this inode.  */
  return ENOTTY;
}

/* Store at most BUFLEN character of the pathname of the terminal FD is
   open on in BUF.  Return 0 on success,  otherwise an error number.  */
int
__ttyname_r (int fd, char *buf, size_t buflen)
{
  struct stat64 st;
  int save = errno;
  int ret;

  if (buflen < sizeof ("/dev/pty/slv/"))
    {
      __set_errno (ERANGE);
      return ERANGE;
    }

  if (!__isatty (fd))
    {
      __set_errno (ENOTTY);
      return ENOTTY;
    }

  if (__fxstat64 (_STAT_VER, fd, &st) < 0)
    return errno;

  /* Prepare the result buffer.  */
  memcpy (buf, "/dev/pty/slv/", sizeof ("/dev/pty/slv/"));
  buflen -= sizeof ("/dev/pty/slv/") - 1;

  ret = getttyname_r (buf, buflen, st.st_dev, st.st_ino, save);

  if (ret)
    {
      memcpy (buf, "/dev/pty/mst/", sizeof ("/dev/pty/mst/"));
      ret = getttyname_r (buf, buflen, st.st_dev, st.st_ino, save);
    }

  return ret;
}

weak_alias (__ttyname_r, ttyname_r)
