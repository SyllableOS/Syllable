#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/semaphore.h>


status_t __lock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut )
{
  return( INLINE_SYSCALL( raw_lock_semaphore_x, 4, hSema, nCount, nFlags, &nTimeOut ) );
}

weak_alias ( __lock_semaphore_x, lock_semaphore_x )
