/* Copyright (C) 1999 Free Software Foundation, Inc.
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
#include <unistd.h>

#include <sysdep.h>
#include <sys/syscall.h>
#include <sys/debug.h>

#include <ctype.h>
#include <malloc.h>
#include <fcntl.h>
#include <atheos/types.h>
//extern void __pthread_kill_other_threads_np __P ((void));
//weak_extern (__pthread_kill_other_threads_np)




int __execve( const char* pzPath, char* const * argv, char* const * envv )
{
  char	zLineBuf[1024];
  int	nError;
  int	nFile;
  int   nLen;
  nFile = open( pzPath, O_RDONLY );

  if ( nFile == -1 ) {
    return( -1 );
  }
  nLen = read( nFile, zLineBuf, 1024 );
  close( nFile );

  if ( nLen > 3 && '#' == zLineBuf[0] && '!' == zLineBuf[1] )
  {
    int		argc;
    int		nStart; /* Start of interpreter name */
    int i;
    char*	pzArgCopy;
    int		nCurPos = 0;
    char*	apzArgs[5];
    int		nArgLen;
    bool	bFoundEnd = false;
    nArgLen = strlen( pzPath ) + 1;
    
    for ( i = 2 ; i < nLen ; ++i ) {
      if ( '\n' == zLineBuf[i] || '\r' == zLineBuf[i] || '\0' == zLineBuf[i] ) {
	bFoundEnd = true;
	zLineBuf[i] = '\0';
	break;
      }
      nArgLen++;
    }
    if ( bFoundEnd == false ) {
      dbprintf( "Interpreter arguments to long\n" );
      errno = E2BIG;
      return( -1 );
    }
    for ( nStart = 2 ; isspace( zLineBuf[nStart] ) ; nStart++ )
	/*** EMPTY ***/;

    for ( argc = 1 ; NULL != argv[argc] ; argc++ ) {
      nArgLen += strlen( argv[argc] ) + 2 + 1;
    }
    pzArgCopy = malloc( nArgLen + 1 );
    if ( pzArgCopy == NULL ) {
      errno = ENOMEM;
      return( -1 );
    }
    strcpy( pzArgCopy, zLineBuf + nStart );
    
    nCurPos = strlen( pzArgCopy );

    pzArgCopy[nCurPos++] = ' ';
    memcpy( pzArgCopy + nCurPos, pzPath, strlen( pzPath ) );
    nCurPos += strlen( pzPath );
    
    for ( argc = 1 ; NULL != argv[argc] ; argc++ ) {
      int nCurLen = strlen( argv[argc] );
      
      pzArgCopy[nCurPos++] = ' ';
      pzArgCopy[nCurPos++] = '"';
      memcpy( pzArgCopy + nCurPos, argv[argc], nCurLen );
      nCurPos += nCurLen;
      pzArgCopy[nCurPos++] = '"';
    }
    pzArgCopy[nCurPos++] = '\0';

    argc = 0;
    apzArgs[ argc++ ] = "/bin/sh";
    apzArgs[ argc++ ] = "-c";
    apzArgs[ argc++ ] = pzArgCopy;
    apzArgs[ argc++ ] = NULL;

//    dbprintf( "Exec interpreter %s (%s)\n", pzArgCopy, pzPath );
    nError = __execve( apzArgs[0], apzArgs, envv );
    
//    dbprintf( "Exec failed\n" );
    free( pzArgCopy );
    return( nError );
  }
  else
  {
    return INLINE_SYSCALL (execve, 3, pzPath, argv, envv);
  }
  return( -1 );
}

#if 0
int
__execve (file, argv, envp)
     const char *file;
     char *const argv[];
     char *const envp[];
{
  /* If this is a threaded application kill all other threads.  */
//  if (__pthread_kill_other_threads_np)
//    __pthread_kill_other_threads_np ();

  return INLINE_SYSCALL (execve, 3, file, argv, envp);
}
#endif
weak_alias (__execve, execve)
