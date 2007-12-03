/* Copyright (C) 1998, 1999 Free Software Foundation, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/image.h>
#include <bits/libc-tsd.h>

#include <ldsodefs.h>

/* This is a bad hack but required by the way the LOCALE TLD is used. */
#include "../../../../locale/localeinfo.h"

extern void __libc_init (int, char **, char **);
extern void __libc_init_first (int argc, char **argv, char **envp);
extern void __getopt_clean_environment (char **);

static void deinit_images( void )
{
  __deinit_main_image();
}

extern int _dl_starting_up;
weak_extern (_dl_starting_up)
extern int __libc_multiple_libcs;
void *__libc_stack_end;	/* Normally defined in dl-sysdeps.c but we had better declare it now */
/* Prototype for local function.  */
static void check_standard_fds (void);

/* Remember the command line argument and enviroment contents for
   later calls of initializers for dynamic libraries.  */
int __libc_argc;
char **__libc_argv;

#ifdef SHARED
/* This is usually part of the rtld but we'll hack it into existence here */
int __libc_enable_secure;
#endif

int
__libc_start_main (int (*main) (int, char **, char **), char** argv,
		   char **envv, void (*init) (void), void (*fini) (void),
		   void *stack_end)
{
  int argc, tld;
#ifndef PIC
  /* The next variable is only here to work around a bug in gcc <= 2.7.2.2.
     If the address would be taken inside the expression the optimizer
     would try to be too smart and throws it away.  Grrr.  */
  int *dummy_addr = &_dl_starting_up;

  __libc_multiple_libcs = dummy_addr && !_dl_starting_up;
#endif

  for ( argc = 0 ; argv[argc] != NULL ; argc++ )
      /*** EMPTY ***/;

  /* Store the lowest stack address.  */
  __libc_stack_end = stack_end;

  /* Set the global _environ variable correctly.  */
  __environ = envv;

  /* Some security at this point.  Prevent starting a SUID binary where
     the standard file descriptors are not opened.  */
  if (__libc_enable_secure)
    check_standard_fds ();

  __libc_tsd_set(LOCALE,&_nl_global_locale);

  /* Call the initializer of the libc.  This is only needed here if we
     are compiling for the static library in which case we haven't
     run the constructors in `_dl_start_user'.  */
#ifndef SHARED
  __libc_init_first (argc, argv, __environ);
#endif

  /* Register the destructor of the program, if any.  */
  if (fini)
    __cxa_atexit ((void (*) (void *)) fini, NULL, NULL);

  /* Store the arguments for main() and initialise the ELF linker */
  __libc_argc = argc;
  __libc_argv = argv;

  __init_main_image();

  /* Call the initialisor of the program, if any */
  if (init)
    (*init) ();

  /* Register our own destructor to allow us to properly tear down the image */
  __cxa_atexit((void (*) (void *)) deinit_images, NULL, NULL );

  exit ((*main) (argc, argv, __environ));
}

