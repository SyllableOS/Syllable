#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <dirstream.h>

#include <atheos/kernel.h>

void __rewind_attrdir( DIR* pDir )
{
  assert( NULL != pDir );
  INLINE_SYSCALL( rewind_attrdir, 1, pDir->fd );
}

weak_alias( __rewind_attrdir, rewind_attrdir )
