/* Copyright (C) 1993, 1996, 1997, 1998 Free Software Foundation, Inc.
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
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>


int __tcsetattr( int nFile, int nOptionActions, const struct termios* psTermios )
{
  if ( NULL == psTermios ) {
    errno = EINVAL;
    dbprintf( "tcsetattr() called whith psTermios == NULL\n" );
    return( -1 );
  }

/*
  CTL's used in linux
  switch (optional_actions)
    {
    case TCSANOW:
      cmd = TCSETS;
      break;
    case TCSADRAIN:
      cmd = TCSETSW;
      break;
    case TCSAFLUSH:
      cmd = TCSETSF;
      break;
    default:
      __set_errno (EINVAL);
      return -1;
    }
  
 */  
  switch( nOptionActions )
  {
    case TCSANOW: 	return( ioctl( nFile, TCSETA, psTermios ) );
    case TCSADRAIN:	return( ioctl( nFile, TCSETAW, psTermios ) );
    case TCSAFLUSH:	return( ioctl( nFile, TCSETAF, psTermios ) );
    default:
      dbprintf( "tcsetattr() Invalid argument in nOptionActions (%d)\n", nOptionActions );
      errno = EINVAL;
      return( - 1 );
  }
}

weak_alias( __tcsetattr, tcsetattr )
