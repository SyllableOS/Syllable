/* Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


/* Prefix for master pseudo terminal nodes.  */
#define _PATH_PTY "/dev/pty/mst/pty"


/* Open a master pseudo terminal and return its file descriptor.  */
int
__getpt (void)
{
  char buf[sizeof (_PATH_PTY) + 4];
  int i;
  char *s;

  s = __mempcpy (buf, _PATH_PTY, sizeof (_PATH_PTY) - 1);

  for (i = 0; i < 10000; ++i)
    {
      int fd;

      sprintf(s, "%d", i);

      fd = __open (buf, O_RDWR|O_CREAT);
      if (fd != -1)
	return fd;

      if (errno == ENOENT)
	return -1;
    }

  __set_errno (ENOENT);
  return -1;
}

#undef __getpt
weak_alias (__getpt, getpt)

#ifndef HAVE_POSIX_OPENPT
/* We cannot define posix_openpt in general for BSD systems.  */
int
__posix_openpt (int oflag)
{
  __set_errno (ENOSYS);
  return -1;
}
weak_alias (__posix_openpt, posix_openpt)

stub_warning (posix_openpt)
# include <stub-tag.h>
#endif
