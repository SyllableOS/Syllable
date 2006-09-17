/* Initialization code run first thing by the ELF startup code.  Syllable version.
   Copyright (C) 1995-1999,2000,01,02 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sysdep.h>
#include <fpu_control.h>
#include <sys/param.h>
#include <sys/types.h>
#include <libc-internal.h>

#ifdef SHARED
#include <ldsodefs.h>
#else
#include <fpu_control.h>
unsigned long int _dl_hwcap;
unsigned long int _dl_hwcap_mask;
fpu_control_t _dl_fpu_control;
#endif

/* Pull in __libc_init_secure () for static library */
#ifndef SHARED
#include <elf/enbl-secure.c>
#endif

/* The function is called from assembly stubs the compiler can't see.  */
static void init (int) __attribute__ ((unused));

/* Set nonzero if we have to be prepared for more then one libc being
   used in the process.  Safe assumption if initializer never runs.  */
int __libc_multiple_libcs attribute_hidden = 1;

#ifndef HAVE_INLINED_SYSCALLS
/* Nonzero during startup.  */
int _dl_starting_up = 1;
#endif

static void
init (int dummy)
{
  extern int __libc_argc;
  extern char **__libc_argv;

#ifdef USE_NONOPTION_FLAGS
  extern void __getopt_clean_environment (char **);
#endif

  __libc_multiple_libcs = &_dl_starting_up && !_dl_starting_up;

  /* Make sure we don't initialize twice.  */
  if (!__libc_multiple_libcs)
    {
      /* Set the FPU control word to the proper default value if the
	 kernel would use a different value.  (In a static program we
	 don't have this information.)  */
#ifdef SHARED
      if (__fpu_control != GLRO(dl_fpu_control))
#endif
	__setfpucw (__fpu_control);
    }

#ifndef SHARED
  __libc_init_secure ();
#endif

  __init_misc (__libc_argc, __libc_argv, environ);

#ifdef SHARED
  __libc_global_ctors ();
#endif
}

#ifdef SHARED

strong_alias (init, _init);

extern void __libc_init_first (void);

void
__libc_init_first (void)
{
}

#else
extern void __libc_init_first (int argc, char **argv, char **envp);

void
__libc_init_first (int argc, char **argv, char **envp)
{
  init (0);
}
#endif


