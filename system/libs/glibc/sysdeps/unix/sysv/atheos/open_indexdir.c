#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <dirstream.h>

#include <atheos/kernel.h>

DIR* __open_indexdir( const char* pzVolume )
{
  DIR* pDir = malloc( sizeof( DIR ) + sizeof( struct dirent ) );
  int  nFile;
  
  assert( NULL != pzVolume );
  
  if ( NULL == pDir ) {
    return( NULL );
  }
  pDir->data = (char*) (pDir + 1);
  nFile = INLINE_SYSCALL( open_indexdir, 1, pzVolume );

  if ( nFile < 0 )
  {
    free( pDir );
    return( NULL );
  }
  else
  {
    pDir->fd = nFile;
    return( pDir );
  }
}

weak_alias( __open_indexdir, open_indexdir )
