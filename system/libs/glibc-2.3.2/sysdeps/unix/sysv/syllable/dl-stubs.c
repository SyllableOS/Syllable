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
#include <sys/dlstubs.h>
#include <atheos/types.h>

/* Taken from elf/dl-error.c */
const char _dl_out_of_memory[] = "out of memory";
INTVARDEF(_dl_out_of_memory)

void *
internal_function
_dl_open (const char *name, int mode, const void *caller)
{
  DL_STUB_WARNING
  return(NULL);
}
libc_hidden_def(_dl_open);

void
internal_function
_dl_close (void *map)
{
  DL_STUB_WARNING
  return;
}
libc_hidden_def(_dl_close);

int
internal_function
_dl_addr (const void *address, Dl_info *info)
{
  DL_STUB_WARNING
  return(ENOSYS);
}
libc_hidden_def(_dl_addr);

int
internal_function
_dl_catch_error (const char **objname, const char **errstring,
			    void (*operate) (void *),
			    void *args)
{
  *errstring = NULL;
  DL_STUB_WARNING
  return(ENOSYS);
}

lookup_t
internal_function
_dl_lookup_symbol (const char *undef,
				   struct link_map *undef_map,
				   const ElfW(Sym) **sym,
				   struct r_scope_elem *symbol_scope[],
				   int type_class, int flags)
{
  DL_STUB_WARNING
  return(ENOSYS);  /* May not be correct */
}

/* Replacement functions for elf/dl-profstub.c */

void
_dl_mcount_wrapper (void *selfpc)
{
  DL_STUB_WARNING
  return;
}

void
_dl_mcount_wrapper_check (void *selfpc)
{
  DL_STUB_WARNING
  return;
}
libc_hidden_def (_dl_mcount_wrapper_check)

