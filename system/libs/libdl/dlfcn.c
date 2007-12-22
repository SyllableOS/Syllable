/* libdl for Syllable - Dynamic Library handling                             */
/*                                                                           */
/* Copyright (C) 2003 Kristian Van Der Vliet (vanders@liqwyd.com)            */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#include <dlfcn.h>

#include <atheos/image.h>

#include <posix/errno.h>
#include <malloc.h>

static int __dl_errno = 0;

static inline void __dl_set_errno( int errno )
{
	__dl_errno = errno;
}

static inline int __dl_get_errno( void )
{
	return( __dl_errno );
}

void *dlopen(const char *file, int mode)
{
	int dl, kernmode = 0;
	void *ret = NULL;

	if( file == 0 )
	{
		__dl_set_errno( _DL_ENOGLOBAL );
		return( ret );
	}

	/* We can ignore the RTLD_LAZY and RTLD_NOW flags.  Syllable always does the
	   equivilent of RTLD_NOW and the spec allows RTLD_LAZY to be implementation
	   dependent; so we let it be RTLD_NOW too */

	/* Note that RTLD_GLOBAL and RTLD_LOCAL arn't exactly correct either.  They're
	   probably close enough for now though. */

	if( mode & RTLD_GLOBAL )
		kernmode |= IM_APP_SPACE;

	if( mode & RTLD_LOCAL )
		kernmode |= IM_LIBRARY_SPACE;

	dl = load_library( file, kernmode );
	if( dl >= 0 )
		ret = (void*)dl;
	else
		__dl_set_errno( _DL_EBADLIBRARY );

	return( ret );
}

void *dlsym(void *handle, const char *name)
{
	int error = 0;
	int dl = (int)handle;
	void* symbol;

	if( dl == 0 )
	{
		__dl_set_errno( _DL_EBADHANDLE );
		return( NULL );
	}

	if( name == NULL )
	{
		__dl_set_errno( _DL_EBADSYMBOL );
		return( NULL );
	}

	if( dl == RTLD_NEXT )
	{
		__dl_set_errno( _DL_EBADHANDLE );
		return( NULL );
	}

	error = get_symbol_address( dl, name, -1, (void**) &symbol );
	if( error < 0 )
	{
		__dl_set_errno( _DL_ENOSYM );
		return( NULL );
	}

	return( symbol );
}

int dlclose(void *handle)
{
	int dl = (int)handle;

	if( dl == 0 )
	{
		__dl_set_errno( _DL_EBADHANDLE );
		return( EINVAL );
	}

	unload_library( dl );

	return( 0 );
}

char *dlerror(void)
{
	char *estr;
	int errno = __dl_get_errno();

	__dl_set_errno( _DL_ENONE );

	switch( errno )
	{
		case _DL_ENOGLOBAL:
		{
			estr = "Global symbol lookups not supported";
			break;
		}

		case _DL_EBADHANDLE:
		{
			estr = "Invalid handle";
			break;
		}

		case _DL_EBADSYMBOL:
		{
			estr = "Bad symbol name";
			break;
		}

		case _DL_ENOSYM:
		{
			estr = "Symbol not found";
			break;
		}

		case _DL_EBADLIBRARY:
		{
			estr = "Failed to load library";
			break;
		}

		case _DL_ENONE:
		{
			estr = NULL;
			break;
		}

		default:
		{
			estr = "Unknown error";
			break;
		}
	}

	return( estr );
}


