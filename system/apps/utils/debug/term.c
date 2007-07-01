/*
 *  dbterm - small utility to send/recive data over a AtheOS debug channel
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <atheos/kdebug.h>
#include <atheos/threads.h>
#include <atheos/kernel.h>

int g_nInPort;
int g_nOutPort;

int read_thread( void* pData )
{
  for (;;)
  {
    char anBuffer[512];
    int nSize = read( 0, anBuffer, 512 );

    if ( nSize > 0 ) {
      debug_write( g_nOutPort, anBuffer, nSize );
    } else {
      snooze( 10000 );
    }
  }
  return( 0 );
}

int main( int argc, char** argv )
{
  thread_id hThread;

  if ( argc == 2 ) {
    if ( strcmp( argv[1], "-d" ) == 0 ) {
      g_nOutPort = 1;
      g_nInPort  = 2;
    } else if ( strcmp( argv[1], "-l" ) == 0 ) {
      g_nOutPort = 9;
      g_nInPort  = 10;
    } else {
      g_nInPort = atoi( argv[1] );
      g_nOutPort = g_nInPort;
    }
  } else {
    g_nInPort = 0;
    g_nOutPort = 0;
  }
  printf( "Connect to port %d\n", g_nInPort );
  if( g_nInPort != g_nOutPort )
  {  
    hThread = spawn_thread( "read", read_thread, 0, 0, NULL );
    resume_thread( hThread );
  }
  for (;;)
  {
    char anBuffer[512];
    int nSize = debug_read( g_nInPort, anBuffer, 512 );

    if ( nSize > 0 ) {
      write( 1, anBuffer, nSize );
    } else {
      snooze( 1000 );
    }
  }
  return( 0 );
}
