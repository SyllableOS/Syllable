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

DIR* __open_attrdir( int nFile )
{
    DIR* pDir = malloc( sizeof( DIR ) + sizeof( struct dirent ) );
    int  nAttrDir;
  
    if ( NULL == pDir ) {
	return( NULL );
    }
    pDir->data = (char*) (pDir + 1);
    nAttrDir = INLINE_SYSCALL( open_attrdir, 1, nFile );

    if ( nAttrDir < 0 ) {
	free( pDir );
	return( NULL );
    } else {
	pDir->fd = nAttrDir;
	return( pDir );
    }
}

weak_alias( __open_attrdir, open_attrdir )
