#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <dirstream.h>

#include <atheos/kernel.h>

struct dirent* __read_indexdir( DIR* pDir )
{
    struct kernel_dirent sKDirEnt;
    int		 nError;

    assert( NULL != pDir );
    nError = INLINE_SYSCALL( read_indexdir, 3, pDir->fd, &sKDirEnt, sizeof(sKDirEnt) );
    if ( nError == 1 ) {
	struct dirent* psEntry = (struct dirent*) pDir->data;

	psEntry->d_ino    = sKDirEnt.d_ino;
	psEntry->d_off    = 0; //kd.d_off;
	psEntry->d_namlen = sKDirEnt.d_namlen;
	psEntry->d_reclen = sizeof(*psEntry);
	psEntry->d_type   = DT_UNKNOWN;
	memcpy( psEntry->d_name, sKDirEnt.d_name, sKDirEnt.d_namlen );
	psEntry->d_name[sKDirEnt.d_namlen] = '\0';
	
	return( psEntry );
    } else {
	if ( nError == 0 ) {
	    __set_errno( EOK );
	}
	return( NULL );
    }
}

weak_alias( __read_indexdir, read_indexdir )
