/* Close a handle opened by `dlopen'.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#ifndef __ATHEOS__

#include <dlfcn.h>
#include <elf/ldsodefs.h>

static void
dlclose_doit (void *handle)
{
  _dl_close (handle);
}

int
dlclose (void *handle)
{
  return _dlerror_run (dlclose_doit, handle) ? -1 : 0;
}

#else	/* __ATHEOS__ */

#include <dlfcn.h>
#include <stdlib.h>
#include <atheos/image.h>

int
dlclose(void *handle)
{
	int ret;
	int* lib_handle;

	lib_handle = (int*)handle;

	ret = unload_library( *lib_handle );
	free( *lib_handle );

	if( ret < 0 )
		return( NULL );	/* FIXME: We do not set an error for dlerror() */

	return( ret );
}

#endif	/* __ATHEOS__ */

