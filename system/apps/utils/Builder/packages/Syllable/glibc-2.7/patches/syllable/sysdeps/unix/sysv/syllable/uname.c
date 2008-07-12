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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

/* Put information about the system in NAME.  */
int __uname( struct utsname *name )
{
	system_info	sysinfo;
	int save;

	if( name == NULL )
	{
		__set_errno (EINVAL);
		return -1;
	}

	save = errno;
	if ( __gethostname( name->nodename, sizeof(name->nodename) ) < 0 )
	{
		if (errno == ENOSYS)
		{
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

	if ( get_system_info_v( &sysinfo, SYS_INFO_VERSION ) < 0 )
		return -1;

	strcpy( name->sysname, sysinfo.zKernelSystem );
	strcpy( name->machine, sysinfo.zKernelCpuArch );

	sprintf( name->version, "%d.%d",
	(int)((sysinfo.nKernelVersion >> 32) & 0xffff),
	(int)((sysinfo.nKernelVersion >> 16) & 0xffff) );

	sprintf( name->release, "%d", (int)(sysinfo.nKernelVersion & 0xffff) );

	return 0;
}
weak_alias (__uname, uname)
libc_hidden_def (__uname)
libc_hidden_def (uname)
