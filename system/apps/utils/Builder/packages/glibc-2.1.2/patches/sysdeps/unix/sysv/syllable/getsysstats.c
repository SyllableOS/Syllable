/* Determine various system internal values, Linux version.
   Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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
#include <stdio.h>
#include <atheos/kernel.h>


#define get_system_info( info ) __xget_system_info( info, SYS_INFO_VERSION )

int __get_nprocs()
{
    system_info sInfo;
    if ( get_system_info( &sInfo ) >= 0 ) {
	return( sInfo.nCPUCount );
    } else {
	return( 1 );
    }
}
weak_alias(__get_nprocs, get_nprocs)


strong_alias( __get_nprocs, __get_nprocs_conf )
weak_alias( __get_nprocs_conf, get_nprocs_conf )

/* Return the number of pages of physical memory in the system. */
int __get_phys_pages()
{
    system_info sInfo;
    if ( get_system_info( &sInfo ) >= 0 ) {
	return( sInfo.nMaxPages );
    } else {
	return( -1 );
    }
}
weak_alias (__get_phys_pages, get_phys_pages)


/* Return the number of available pages of physical memory in the system. */
int __get_avphys_pages()
{
    system_info sInfo;
    if ( get_system_info( &sInfo ) >= 0 ) {
	return( sInfo.nFreePages );
    } else {
	return( -1 );
    }
}
weak_alias( __get_avphys_pages, get_avphys_pages )
