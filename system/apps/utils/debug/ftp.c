/*
 *  rfs - small utility to copy files over a AtheOS debug channel
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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <atheos/kdebug.h>
#include <atheos/kernel.h>
#include <atheos/time.h>

#include <macros.h>

int g_nPort = 7;

#define PACKET_SIZE 512 * 2

bool g_bServerDebug = false;
bool g_bDumpTerm;

enum
{
  CMD_INVAL,
  CMD_OPEN,
  CMD_OPEN_REPLY,
  CMD_CLOSE,
  CMD_CLOSE_REPLY,
  CMD_WRITE,
  CMD_WRITE_REPLY
};
typedef struct
{
  int   or_nCommand;
  int	or_nOpenMode;
  char	or_zPath[PACKET_SIZE - 8];
} OpenReq_s;

typedef struct
{
  int 	   or_nCommand;
  int 	   or_nHandle;
  fs_off_t or_nFileSize;
} OpenReply_s;

typedef struct
{
  int	   rr_nCommand;
  int 	   rr_nHandle;
  fs_off_t rr_nOffset;
  int	   rr_nLength;
  int	   rr_nCRC;
} ReadReq_s;

typedef struct
{
  int	   rr_nCommand;
  int	   rr_nHandle;
  fs_off_t rr_nOffset;
  int	   rr_nLength;
} ReadReply_s;

typedef struct
{
  int	   wr_nCommand;
  int	   wr_nCRC;
  int	   wr_nHandle;
  fs_off_t wr_nOffset;
  int	   wr_nLength;
} WriteReply_s;

int calc_crc( const char* pBuffer, int nSize )
{
  int i;
  int nSum = 0;
  
  for ( i = 0 ; i < nSize ; ++i ) {
    nSum += pBuffer[i] * 325413;
  }
  return( nSum );
}

int server()
{
  bool	bNoWait = false;
  fs_off_t nPrevPos = 0;
  for (;;)
  {
    char anBuffer[PACKET_SIZE];
    int nSize = socket_read( g_nPort, anBuffer, PACKET_SIZE );

//    if ( nSize > 0 ) {
//      printf( "Got packet of %d bytes\n", nSize );
//    }
    if ( nSize == PACKET_SIZE )
    {
      switch( *((int*)anBuffer) )
      {
	case CMD_OPEN:
	{
	  OpenReq_s*   psReq = (OpenReq_s*) anBuffer;
	  OpenReply_s* psReply = (OpenReply_s*) anBuffer;
	  int nFile;

	  if ( g_bServerDebug ) {
	    printf( "Open '%s' with mode %d\n", psReq->or_zPath, psReq->or_nOpenMode );
	  }
	  nFile = open( psReq->or_zPath, psReq->or_nOpenMode, 0777 );
	  psReply->or_nCommand = CMD_OPEN_REPLY;
	  psReply->or_nHandle = nFile;
	  socket_write( g_nPort, (char*)psReply, PACKET_SIZE );
	  bNoWait = true;
	  nPrevPos = 0;
	  break;
	}
	case CMD_CLOSE:
	{
	  OpenReply_s* psReq = (OpenReply_s*) anBuffer;
	  if ( g_bServerDebug ) {
	    printf( "\nClose %d", psReq->or_nHandle );
	  }
	  close( psReq->or_nHandle );
	  *((int*)anBuffer) = CMD_CLOSE_REPLY;
	  socket_write( g_nPort, anBuffer, PACKET_SIZE );
	  bNoWait = false;
	  break;
	}
	case CMD_WRITE:
	{
	  ReadReq_s* psReq = (ReadReq_s*) anBuffer;
	  int nCRC = psReq->rr_nCRC;
	  
	  psReq->rr_nCRC = 0;
	  if ( nCRC != calc_crc( anBuffer, PACKET_SIZE ) ) {
	    dbprintf( "Wrong CRC on write packet\n" );
	    break;
	  }
	  if ( g_bServerDebug ) {
	    printf( "Write %d bytes to %d att offset %Ld\r", psReq->rr_nLength, psReq->rr_nHandle, psReq->rr_nOffset );
	  }
	  fflush( stdout );
	  if ( psReq->rr_nOffset != nPrevPos ) {
	    dbprintf( "Write is skipping %d bytes\n", (int)(psReq->rr_nOffset - nPrevPos) );
	  }
	  lseek( psReq->rr_nHandle, psReq->rr_nOffset, SEEK_SET );
	  write( psReq->rr_nHandle, anBuffer + sizeof(ReadReq_s), psReq->rr_nLength );
	  nPrevPos = psReq->rr_nOffset + psReq->rr_nLength;
	  
	  ((int*)anBuffer)[0] = CMD_WRITE_REPLY;
	  ((int*)anBuffer)[1] = nCRC;
	  socket_write( g_nPort, anBuffer, 8 );
	  break;
	}
      }
    }
    else
    {
      if ( bNoWait == false ) {
	Delay( 1000 );
      }
    }
  }
}


int open_remote_file( const char* pzPath )
{
  char	       anBuffer[PACKET_SIZE];
  OpenReq_s*   psOpenReq   = (OpenReq_s*)   anBuffer;
  OpenReply_s* psOpenReply = (OpenReply_s*) anBuffer;
  int	       i;
  
  psOpenReq->or_nCommand = CMD_OPEN;
  psOpenReq->or_nOpenMode = O_WRONLY | O_CREAT | O_TRUNC;
  
  strcpy( psOpenReq->or_zPath, pzPath );

//  debug_write( g_nPort, "Hello world\n", PACKET_SIZE );
  socket_write( g_nPort, anBuffer, PACKET_SIZE );
  printf( "Open remote file %s\n", pzPath );
  
  for ( i = 0 ; i < 100 ; ++i )
  {
    if ( (i % 5) == 0 ) {
      printf( "." );
      fflush( stdout );
    }
    if ( socket_read( g_nPort, anBuffer, PACKET_SIZE ) == PACKET_SIZE )
    {
      if ( psOpenReply->or_nCommand == CMD_OPEN_REPLY ) {
	printf( "\nGot reply on open %d\n", psOpenReply->or_nHandle );
	return( psOpenReply->or_nHandle );
      }
    }
    Delay( 100000 );
  }
  printf( "\nGot timeout on open\n" );
  return( -1 );
}

int close_remote_file( int nHandle )
{
  char	       anBuffer[PACKET_SIZE];
  OpenReply_s* psCloseReq = (OpenReply_s*) anBuffer;
  int	       i;
  
  for ( i = 0 ; i < 10 ; ++i )
  {
    int	j;
    
    psCloseReq->or_nCommand = CMD_CLOSE;
    psCloseReq->or_nHandle  = nHandle;
  
    socket_write( g_nPort, anBuffer, PACKET_SIZE );
    for ( j = 0 ; j < 100 ; ++j )
    {
      if ( socket_read( g_nPort, anBuffer, PACKET_SIZE ) == PACKET_SIZE )
      {
	if ( *((int*)anBuffer) == CMD_CLOSE_REPLY ) {
	  printf( "Got reply on close\n" );
	  return(0);
	}
      }
      Delay( 100000 );
    }
    printf( "Got timeout on close %d\n", i );
  }
  return( -1 );
}

int remote_write( int nHandle, const char* pBuffer, off_t nPos, int nSize )
{
  char	       anBuffer[PACKET_SIZE];
  ReadReq_s*   psWriteReq  = (ReadReq_s*)   anBuffer;
  int	       nBlkSize = PACKET_SIZE - sizeof(ReadReq_s);
  off_t	       nCurPos = nPos;
  int	       j;
  int          nLoopCnt = 0;
  
  while( nSize > 0 )
  {
    int nCurSize = min( nBlkSize, nSize );
    int	nCRC;
    
    
    psWriteReq->rr_nCommand= CMD_WRITE;
    psWriteReq->rr_nHandle = nHandle;
    psWriteReq->rr_nOffset = nCurPos;
    psWriteReq->rr_nLength = nCurSize;
    psWriteReq->rr_nCRC    = 0;
    memcpy( anBuffer + sizeof(ReadReq_s), pBuffer, nCurSize );
    
    nCRC = calc_crc( anBuffer, PACKET_SIZE );
    psWriteReq->rr_nCRC    = nCRC;

//    printf( "Send %d bytes to remote machine\n", nSize );
    socket_write( g_nPort, anBuffer, PACKET_SIZE );

    for ( j = 0 ; j < 40000 ; ++j )
    {
      memset( anBuffer, 0, 8 );
      if ( socket_read( g_nPort, anBuffer, 8 ) >= 8 ) {
	WriteReply_s* psReply = (WriteReply_s*) anBuffer;
	if ( psReply->wr_nCommand == CMD_WRITE_REPLY ) {
	  if ( psReply->wr_nCRC != nCRC ) {
	    dbprintf( "\nReply had wrong CRC (%d) should be %d\n", psReply->wr_nCRC, nCRC );
	    continue;
	  }
//	  printf( "\nGot reply on write" );
	  break;
	}
      } else {
	Delay( /*1000*/ 0  );
      }
    }
    if ( j == 40000 ) {
      printf( "\nTimeout on write" );
    } else {
      nCurPos += nCurSize;
      nSize   -= nCurSize;
      pBuffer += nCurSize;

      if ( ++nLoopCnt > 1 ) {
	printf( "\nLoop %d times", nLoopCnt );
      }
    }
  }
  return( 0 );
}

void send_file( const char* pzLocalPath, const char* pzRemotePath )
{
  char	       anBuffer[PACKET_SIZE * 2];
  int	       nLocalHandle;
  int	       nRemoteHandle     = -1;
  int	       nBlkSize = PACKET_SIZE - sizeof(ReadReq_s);
  fs_off_t     nCurPos  = 0;
  int	       i = 0;
  bigtime_t    nStartTime;
  
  nLocalHandle = open( pzLocalPath, O_RDONLY );

  if ( nLocalHandle < 0 ) {
    printf( "Failed to open source %s\n", pzLocalPath );
    return;
  }

  nRemoteHandle = open_remote_file( pzRemotePath );

  if ( nRemoteHandle < 0 ) {
    printf( "Failed to open dest %s\n", pzRemotePath );
    goto error1;
  }
//  for ( i = 0 ; i < 100 ; ++i )
  nStartTime = get_real_time();
  for (;;)
  {
//    ReadReq_s*   psWriteReq  = (ReadReq_s*)   anBuffer;
    int nSize;
//    int	j;
    
    nSize = read( nLocalHandle, anBuffer, nBlkSize );
    if ( nSize <= 0 ) {
      printf( "\nDone sending file\n" );
      break;
    }
    remote_write( nRemoteHandle, anBuffer, nCurPos, nSize );
    nCurPos += nSize;
    
    if ( (i++ % 25) == 0 || nSize != nBlkSize )
    {   
      printf( "%Ld bytes written in %5.2fs (%Ld/s)",
	      nCurPos,
	      (float)(get_real_time() - nStartTime) / 1000000.0f,
	      (nCurPos * 1000000LL) / (get_real_time() - nStartTime) );
      if ( g_bDumpTerm ) {
	printf( "\n" );
      } else {
	printf( "\r" );
	fflush( stdout );
      }
    }
  }
  close_remote_file( nRemoteHandle );
error1:
  close( nLocalHandle );
}

int main( int argc, char** argv )
{
  char* pzTerm = getenv( "TERM" );

  if ( pzTerm != NULL && strncmp( pzTerm, "xterm", 5 ) == 0 ) {
    g_bDumpTerm = false;
  } else {
    g_bDumpTerm = true;
  }
  
  if ( argc != 3 ) {
    printf( "Start server at port %d\n", g_nPort );
    server();
  } else {
    send_file( argv[1], argv[2] );
  }
  return( 0 );
}
