/* Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Zack Weinberg <zack@rabi.phys.columbia.edu>, 1998.

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
#include <limits.h>
/*#include <pty.h>*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sys/types.h>



/* Create pseudo tty master slave pair and set terminal attributes
   according to TERMP and WINP.  Return handles for both ends in
   AMASTER and ASLAVE, and return the name of the slave end in NAME.  */
int openpty (int *amaster, int *aslave, char *name, struct termios *termp,
	     struct winsize *winp)
{
    int i;
    int master = -1;
    int slave = -1;
    int result = 0;
    char line[256];
  
    for ( i = 0 ; i < 1000 ; ++i )
    {
	struct stat sStat;
		
	sprintf( line, "/dev/pty/mst/lcp%d", i );

//    if ( stat( line, &sStat ) == -1 )
	{
	    master = open( line, O_RDWR | O_CREAT );
	    if ( master < 0 ) {
		continue;
	    }
	    sprintf( line, "/dev/pty/slv/lcp%d", i );
	    break;
	}
    }
    if ( master >= 0 ) {
	slave = open( line, O_RDWR );
	if ( slave < 0 ) {
	    dbprintf( "Error: libc.so::openpty() failed to open slave %s\n", line );
	    close( master );
	    result = -1;
	} else {
	    *amaster = master;
	    *aslave = slave;
      
	    if ( name != NULL ) {
		strcpy( name, line );
	    }
	    if(termp) {
		tcsetattr (slave, TCSAFLUSH, termp);
	    }
	    if (winp) {
		__ioctl (slave, TIOCSWINSZ, winp);
	    }
	}
    } else {
	dbprintf( "Error: libc.so::openpty() failed to create master pty\n" );
	result = -1;
    }
    return( result );
}
