#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <dirstream.h>

#include <atheos/kernel.h>

int __rewind_indexdir( DIR* pDir )
{
    int		 nError;

    nError = INLINE_SYSCALL( rewind_indexdir, 1, pDir->fd );
    return( nError );
}

weak_alias( __rewind_indexdir, rewind_indexdir )
