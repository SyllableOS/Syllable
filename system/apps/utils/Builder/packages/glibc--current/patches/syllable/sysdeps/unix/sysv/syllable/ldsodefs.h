/* Run-time dynamic linker data structures for loaded ELF shared objects.
   Heavily stripped down Syllable version.
   Copyright (C) 1995-2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef	_LDSODEFS_H
#define	_LDSODEFS_H	1

#include <features.h>

#include <stdbool.h>
#define __need_size_t
#define __need_NULL
#include <stddef.h>
#include <string.h>

#include <elf.h>
#include <dlfcn.h>
#include <fpu_control.h>
#include <sys/mman.h>
#include <link.h>
#include <dl-lookupcfg.h>
#include <dl-sysdep.h>
#include <bits/libc-lock.h>
#include <hp-timing.h>
#include <tls.h>

__BEGIN_DECLS

/* Internal functions of the run-time dynamic linker. */

#ifndef SHARED
# define EXTERN extern
# define GLRO(name) _##name
#else
# define EXTERN
# ifdef IS_IN_rtld
#  define GLRO(name) _rtld_local_ro._##name
# else
#  define GLRO(name) _rtld_global_ro._##name
# endif
struct rtld_global_ro
{
#endif
  /* Mask for hardware capabilities that are available.  */
  EXTERN unsigned long int _dl_hwcap;

  /* Mask for important hardware capabilities we honour. */
  EXTERN unsigned long int _dl_hwcap_mask;

  /* Default floating-point control word.  */
  EXTERN fpu_control_t _dl_fpu_control;

  /* Get architecture specific definitions.  */
#define PROCINFO_DECL
#ifndef PROCINFO_CLASS
# define PROCINFO_CLASS EXTERN
#endif
#include <dl-procinfo.c>

#ifdef SHARED
};
extern struct rtld_global_ro _rtld_local_ro;
extern struct rtld_global_ro _rtld_global_ro;
# else
/* We cheat a bit here.  We declare the variable as as const even
   though it is at startup.  */
extern const struct rtld_global_ro _rtld_global_ro;
#endif

/* Flag set at startup and cleared when the last initializer has run.  */
extern int _dl_starting_up;
weak_extern (_dl_starting_up)

__END_DECLS

#endif /* ldsodefs.h */



