/* Copyright (C) 1993,1996,1997,1998,2002,2003 Free Software Foundation, Inc.
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
#include <termios.h>
#include <sysdep.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <atheos/types.h>

/* Set the state of FD to *TERMIOS_P.  */
int
tcsetattr (fd, optional_actions, termios_p)
     int fd;
     int optional_actions;
     const struct termios *termios_p;
{
	int ret;

	if ( NULL == termios_p )
	{
		__set_errno ( EINVAL );
		dbprintf ( "tcsetattr() called with termios_p == NULL\n" );
		return ( -1 );
	}

	switch (optional_actions)
	{
		case TCSANOW:
			ret = ioctl ( fd, TCSETA, termios_p );
			break;
		case TCSADRAIN:
			ret = ioctl ( fd, TCSETAW, termios_p );
			break;
		case TCSAFLUSH:
			ret = ioctl ( fd, TCSETAF, termios_p );
			break;
		default:
		{
			dbprintf ( "tcsetattr() called with invalid argument 0x%2x\n", optional_actions );
			__set_errno (EINVAL);
			ret = -1;
		}
	}
	return( ret );
}
libc_hidden_def (tcsetattr)
