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

#include <errno.h>
#include <assert.h>
#include <sysdep.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/image.h>
#include <sys/syscall.h>
#include <sys/debug.h>

#include <atheos/atomic.h>

#define MAX_OPEN_LIBS	1024

static atomic_t open_libs[MAX_OPEN_LIBS];

static void __do_init_library( image_info *info, uint8* opened_counts )
{
  int image_id				= info->ii_image_id;
  int sub_image_count		= info->ii_sub_image_count;
  int open_count			= info->ii_open_count;
  image_init* image_init	= info->ii_init;

  assert( image_id >= 0 && image_id < MAX_OPEN_LIBS );
  opened_counts[ image_id ]++;

  /* Initialise each sub-image */
  for( int i = 0; i < sub_image_count; i++ )
    if( get_image_info( image_id, i, info ) == 0 )
      __do_init_library( info, opened_counts );

  /* Call any init functions */
  if( opened_counts[ image_id ] == open_count && NULL != image_init );
	image_init( image_id );
}

int __init_library( int lib )
{
  image_info info;
  uint8 opened_counts[MAX_OPEN_LIBS];

  memset( opened_counts, 0, MAX_OPEN_LIBS );

  if( get_image_info( lib, -1, &info ) == 0 )
  {
    __do_init_library( &info, opened_counts );
    return 0;
  }
  else
  {
    dbprintf("%s unable to get image information for library %i\n", __FUNCTION__, lib );
    return -1;
  }
}

weak_alias( __init_library, init_library )

static void __do_deinit_library( image_info *info, uint8* opened_counts )
{
  int image_id			= info->ii_image_id;
  int sub_image_count		= info->ii_sub_image_count;
  int open_count		= info->ii_open_count;

  assert( image_id >= 0 && image_id < MAX_OPEN_LIBS );
  opened_counts[ image_id ]++;

  /* Call any fini functions */
  if( opened_counts[ image_id ] == open_count && NULL != info->ii_fini );
    info->ii_fini( image_id );

  /* Uninitialise each sub-image */
  for( int i = 0; i < sub_image_count; i++ )
    if( get_image_info( image_id, i, info ) == 0 )
      __do_deinit_library( info, opened_counts );

}

int __deinit_library( int lib )
{
  image_info info;
  uint8 opened_counts[MAX_OPEN_LIBS];

  memset( opened_counts, 0, MAX_OPEN_LIBS );

  if( get_image_info( lib, -1, &info ) == 0 )
  {
    __do_deinit_library( &info, opened_counts );
    return 0;
  }
  else
  {
    dbprintf("%s unable to get image information for library %i\n", __FUNCTION__, lib );
    return -1;
  }
}

weak_alias( __deinit_library, deinit_library )

int __load_library( const char* path, uint32 flags )
{
  int lib = INLINE_SYSCALL( load_library, 3, path, flags, getenv("DLL_PATH") );
  if( lib > 0 && lib < MAX_OPEN_LIBS )
  {
    atomic_add( &open_libs[lib], 1 );
    __init_library( lib );
  }
  return( lib );
}

weak_alias( __load_library, load_library )

int __unload_library( int lib )
{
  int error;

  assert( lib > 0 && lib < MAX_OPEN_LIBS );

  /* Unmark the library slot as used */
  atomic_add( &open_libs[lib], -1 );

  /* Unload the library */
  __deinit_library( lib );
  error = INLINE_SYSCALL( unload_library, 1, lib );
  return error;
}

weak_alias( __unload_library, unload_library )

int __get_image_id( void )
{
  image_info* info = NULL;

  if( get_image_info( 0, -1, info ) == 0)
  {
    return info->ii_image_id;
  }

  dbprintf("%s unable to get image information for main image\n", __FUNCTION__);
  return -1;
}

weak_alias( __get_image_id, get_image_id )

void __init_main_image( void )
{
  /* Mark first available library slot as used */
  atomic_add( &open_libs[0], 1 );

  /* Initialise the main image */
  if( __init_library( 0 ) )
    dbprintf("%s failed to initialise main image\n", __FUNCTION__ );

}
libc_hidden_def(__init_main_image)

void __deinit_main_image( void )
{
  /* Unload each library */
  for( int i = 1; i < MAX_OPEN_LIBS; i++ )
  {
    while( open_libs[i] > 0 )
      __unload_library( i );
  }

  /* Unload the main image */
  __deinit_library( 0 );
}
libc_hidden_def(__deinit_main_image)
