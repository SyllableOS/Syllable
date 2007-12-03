/* Copyright (C) 1991,92,93,1996-2001,2002 Free Software Foundation, Inc.
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

#if 0
/* Is this used anywhere?  It is not exported.  */
char *__ttyname;
#endif

static char *getttyname (const char *dev, dev_t mydev,
			 ino64_t myino, int save)
     internal_function;


libc_freeres_ptr (static char *getttyname_name);

static char *
internal_function
getttyname (const char *dev, dev_t mydev, ino64_t myino, int save)
{
  static size_t namelen;
  struct stat64 st;
  DIR *dirstream;
  struct dirent *d;
  size_t devlen = strlen (dev) + 1;

  dirstream = __opendir (dev);
  if (dirstream == NULL)
    {
      return NULL;
    }

  while ((d = __readdir (dirstream)) != NULL)
    if (d->d_fileno == myino)
      {
	size_t dlen = _D_ALLOC_NAMLEN (d);
	if (devlen + dlen > namelen)
	  {
	    free (getttyname_name);
	    namelen = 2 * (devlen + dlen); /* Big enough.  */
	    getttyname_name = malloc (namelen);
	    if (! getttyname_name)
	      {
		/* Perhaps it helps to free the directory stream buffer.  */
		(void) __closedir (dirstream);
		return NULL;
	      }
	  }
	/* Always recopy dev since it may be master or slave.  */
	*((char *) __mempcpy (getttyname_name, dev, devlen - 1)) = '/';

	memcpy (&getttyname_name[devlen], d->d_name, dlen);
	if (__xstat64 (_STAT_VER, getttyname_name, &st) == 0
#ifdef _STATBUF_ST_RDEV
	    && S_ISCHR (st.st_mode) && st.st_rdev == mydev
#else
	    && d->d_fileno == myino && st.st_dev == mydev
#endif
	   )
	  {
	    (void) __closedir (dirstream);
#if 0
	    __ttyname = getttyname_name;
#endif
	    __set_errno (save);
	    return getttyname_name;
	  }
      }

  (void) __closedir (dirstream);
  __set_errno (save);
  return NULL;
}


/* Static buffer in `ttyname'.  */
libc_freeres_ptr (static char *ttyname_buf);


/* Return the pathname of the terminal FD is open on, or NULL on errors.
   The returned storage is good only until the next call to this function.  */
char *
ttyname (int fd)
{
  struct stat64 st;
  char *name;
  int save = errno;

  if (!__isatty (fd))
    return NULL;

  if (__fxstat64 (_STAT_VER, fd, &st) < 0)
    return NULL;

  name = getttyname ("/dev/pty/slv", st.st_dev, st.st_ino, save);

  if (!name)
    {
      name = getttyname ("/dev/pty/mst", st.st_dev, st.st_ino, save);
    }

  return name;
}
