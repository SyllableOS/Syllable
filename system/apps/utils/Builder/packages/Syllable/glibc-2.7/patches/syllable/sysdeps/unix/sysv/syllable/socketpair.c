/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
   Copyright (C) 1999 Philippe De Muyter
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>

socketpair( int af, int type, int protocol, int fd[2] )
{
	int listen_socket;
	struct sockaddr_in sin[2];
	int len;

	/* The following is only valid if type == SOCK_STREAM */
	if( type != SOCK_STREAM )
		return -1;

	/* Create a temporary listen socket; temporary, so any port is good */
	listen_socket = socket( af, type, protocol );
	if( listen_socket < 0 )
	{
		perror( "creating listen_socket" );
		return -1;
	}
	sin[0].sin_family = af;
	sin[0].sin_port = 0;	/* Use any port number */
	sin[0].sin_addr.s_addr = inet_makeaddr( INADDR_ANY, 0 );
	if( bind( listen_socket, (struct sockaddr *)&sin[0], sizeof( sin[0] ) ) < 0 )
	{
		perror( "bind" );
		return -1;
	}
	len = sizeof( sin[0] );

	/* Read the port number we got, so that our client can connect to it */
	if( getsockname( listen_socket, (struct sockaddr *)&sin[0], &len ) < 0 )
	{
		perror( "getsockname" );
		return -1;
	}

	/* Put the listen socket in listening mode */
	if( listen( listen_socket, 5 ) < 0 )
	{
		perror( "listen" );
		return -1;
	}

	/* Create the client socket */
	fd[1] = socket( af, type, protocol );
	if( fd[1] < 0 )
	{
		perror( "creating client_socket" );
		return -1;
	}

	/* Put the client socket in non-blocking connecting mode */
	fcntl( fd[1], F_SETFL, fcntl( fd[1], F_GETFL, 0 ) | O_NDELAY );
	if( connect( fd[1], (struct sockaddr *)&sin[0], sizeof( sin[0] ) ) < 0 )
	{
		perror( "connect" );
		return -1;
	}

	/* At the listen-side, accept the incoming connection we generated */
	len = sizeof( sin[1] );
	if( ( fd[0] = accept( listen_socket, (struct sockaddr *)&sin[1], &len ) ) < 0 )
	{
		perror( "accept" );
		return -1;
	}

	/* Reset the client socket to blocking mode */
	fcntl( fd[1], F_SETFL, fcntl( fd[1], F_GETFL, 0 ) & ~O_NDELAY );

	close( listen_socket );

	return 0;
}

