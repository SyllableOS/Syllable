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

int __close_indexdir( DIR* pDir )
{
  int 		 nError;

  assert( NULL != pDir );
  
  nError = close( pDir->fd );
  free( pDir );
  
  return( nError );
}

weak_alias( __close_indexdir, close_indexdir )
