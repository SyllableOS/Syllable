/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <sysdep.h>

/* Because the Linux version is in fact i386/ELF and the start.? file
   for this system (sysdeps/i386/elf/start.S) is also used by The Hurd
   and therefore this files must not contain the definition of the
   `errno' variable (I don't know why, ask Roland), we have to define
   it somewhere else.

   ...and this place is here.  */
	.bss
	.globl	errno
	.type errno,@object
	.size errno,4
errno:
	.space	4
weak_alias (errno, _errno)

/* The following code is only used in the shared library when we
   compile the reentrant version.  Otherwise each system call defines
   each own version.  */

#ifndef PIC

/* The syscall stubs jump here when they detect an error.
   The code for Linux is almost identical to the canonical Unix/i386
   code, except that the error number in %eax is negated.  */

#undef CALL_MCOUNT
#define CALL_MCOUNT /* Don't insert the profiling call, it clobbers %eax.  */

	.text
ENTRY (__syscall_error)
	negl %eax

#define __syscall_error __syscall_error_1
#include <sysdeps/unix/i386/sysdep.S>

#endif	/* !PIC */
