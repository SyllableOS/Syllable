/* Copyright (C) 1991, 1992, 1996, 1997 Free Software Foundation, Inc.
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
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <atheos/kernel.h>

/* Put information about the system in NAME.  */
int uname( struct utsname *name )
{
    system_info	sSysInfo;
    int save;

    if (name == NULL)
    {
	__set_errno (EINVAL);
	return -1;
    }

    save = errno;
    if ( __gethostname( name->nodename, sizeof(name->nodename) ) < 0 ) {
	if (errno == ENOSYS) {
	      /* Hostname is meaningless for this machine.  */
	    name->nodename[0] = '\0';
	    __set_errno (save);
	}
	else if (errno == ENAMETOOLONG)
	      /* The name was truncated.  */
	    __set_errno (save);
	else
	    return -1;
    }
    if ( get_system_info( &sSysInfo ) < 0 ) {
	return -1;
    }
    strcpy( name->sysname, "atheos" );
    strcpy( name->release, "13" );

    if ( ((sSysInfo.nKernelVersion >> 48) & 0xffff) > 0 && ((sSysInfo.nKernelVersion >> 48) & 0xffff) < 26 ) {
	sprintf( name->version, "%d.%d.%d%c",
		 (int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		 (int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
		 (int)(sSysInfo.nKernelVersion & 0xffff),
		 'a' + (int)((sSysInfo.nKernelVersion >> 48) & 0xffff) );
    } else {
	sprintf( name->version, "%d.%d.%d",
		 (int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
		 (int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
		 (int)(sSysInfo.nKernelVersion & 0xffff) );
    }
    strcpy( name->machine, "i386" );

    return 0;
}
