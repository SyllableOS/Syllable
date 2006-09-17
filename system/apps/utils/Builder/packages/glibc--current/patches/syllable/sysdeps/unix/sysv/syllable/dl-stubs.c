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

#include <dlfcn.h>
#include <ldsodefs.h>
#include <errno.h>
#include <atheos/types.h>

/* Do not emit a stub warning for either of these functions as they
   may be called tens, if not hundreds of times at startup. */
int
internal_function
_dl_addr (const void *address, Dl_info *info,
	     struct link_map **mapp, const ElfW(Sym) **symbolp)
{
  __set_errno(ENOSYS);
  return 0;
}
libc_hidden_def(_dl_addr);

/* Replacement functions for elf/dl-profstub.c */

void
_dl_mcount_wrapper_check (void *selfpc)
{
  __set_errno(ENOSYS);
  return;
}
libc_hidden_def (_dl_mcount_wrapper_check)

