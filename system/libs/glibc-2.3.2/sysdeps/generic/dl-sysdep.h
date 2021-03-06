/* System-specific settings for dynamic linker code.  Generic version.
   Copyright (C) 2002 Free Software Foundation, Inc.
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

/* This macro must be defined to either 0 or 1.

   If 1, then an errno global variable hidden in ld.so will work right with
   all the errno-using libc code compiled for ld.so, and there is never a
   need to share the errno location with libc.  This is appropriate only if
   all the libc functions that ld.so uses are called without PLT and always
   get the versions linked into ld.so rather than the libc ones.  */

#ifdef IS_IN_rtld
# define RTLD_PRIVATE_ERRNO 1
#else
# define RTLD_PRIVATE_ERRNO 0
#endif
