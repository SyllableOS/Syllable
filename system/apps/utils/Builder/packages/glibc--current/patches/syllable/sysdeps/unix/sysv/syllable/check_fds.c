/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
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
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>
#include <sys/stat.h>

/* Try to get a machine dependent instruction which will make the
   program crash.  This is used in case everything else fails.  */
#include <abort-instr.h>
#ifndef ABORT_INSTRUCTION
/* No such instruction is available.  */
# define ABORT_INSTRUCTION
#endif

static void
check_one_fd (int fd, int mode)
{
  if (__fcntl (fd, F_GETFD) == -1 && errno == EBADF)
    {
      /* Something is wrong with this descriptor, it's probably not
	 opened.  Open /dev/null so that the SUID program we are
	 about to start does not accidently use this descriptor.  */
      int nullfd = __open (_PATH_DEVNULL, mode);
      if (nullfd == -1)
	/* We cannot even given an error message here since it would
	   run into the same problems.  */
        ABORT_INSTRUCTION;
    }
}

void
check_standard_fds (void)
{
/* Check all three standard file descriptors.  */
  check_one_fd (STDIN_FILENO, O_RDONLY);
  check_one_fd (STDOUT_FILENO, O_RDWR);
  check_one_fd (STDERR_FILENO, O_RDWR);
}

