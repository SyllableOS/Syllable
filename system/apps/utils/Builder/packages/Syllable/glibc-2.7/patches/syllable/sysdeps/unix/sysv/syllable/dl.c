/* Copyright (C) 1999, 2000, 2001, 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.columbia.edu>, 1999.

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
#include <stdlib.h>
#include <ldsodefs.h>
#include <malloc.h>

#include <atheos/image.h>

static int
internal_function
dlerror_run (void (*operate) (void *), void *args)
{
  return -1;
}

/* This code is to support __libc_dlopen from __libc_dlopen'ed shared
   libraries.  We need to ensure the statically linked __libc_dlopen
   etc. functions are used instead of the dynamically loaded.  */
struct dl_open_hook
{
  void *(*dlopen_mode) (const char *name, int mode);
  void *(*dlsym) (void *map, const char *name);
  int (*dlclose) (void *map);
};

#ifdef SHARED
extern struct dl_open_hook *_dl_open_hook;
libc_hidden_proto (_dl_open_hook);
struct dl_open_hook *_dl_open_hook __attribute__((nocommon));
libc_hidden_data_def (_dl_open_hook);
#else
static struct dl_open_hook _dl_open_hook =
  {
    .dlopen_mode = __libc_dlopen_mode,
    .dlsym = __libc_dlsym,
    .dlclose = __libc_dlclose
  };
#endif

/* ... and these functions call dlerror_run. */

void *
__libc_dlopen_mode (const char *name, int mode)
{
  int *dl, kernmode = 0;

  if( name == NULL )
    return NULL;

  /* We can ignore the RTLD_LAZY and RTLD_NOW flags.  Syllable always does the
     equivilent of RTLD_NOW and the spec allows RTLD_LAZY to be implementation
     dependent; so we let it be RTLD_NOW too */

  /* Note that RTLD_GLOBAL and RTLD_LOCAL arn't exactly correct either.  They're
     probably close enough for now though. */

  if( mode & RTLD_GLOBAL )
    kernmode |= IM_APP_SPACE;

  if( mode & RTLD_LOCAL )
    kernmode |= IM_LIBRARY_SPACE;

  dl = (int*)malloc( sizeof( int ) );
  if( dl != NULL )
    *dl = load_library( name, kernmode );

  return (void*)dl;
}
libc_hidden_def (__libc_dlopen_mode)

void *
__libc_dlsym (void *map, const char *name)
{
	int error = 0;
	int *dl = (int*)map;
	void* symbol;

	if( name == NULL || dl == NULL || *dl == RTLD_NEXT )
		return NULL;

	error = get_symbol_address( *dl, name, -1, (void**) &symbol );
	if( error < 0 )
		return NULL;

	return symbol;
}
libc_hidden_def (__libc_dlsym)

int
__libc_dlclose (void *map)
{
	int *dl = (int*)map;

	if( dl == NULL )
		return EINVAL;

	unload_library( *dl );
	free( map );

	return 0;
}
libc_hidden_def (__libc_dlclose)

