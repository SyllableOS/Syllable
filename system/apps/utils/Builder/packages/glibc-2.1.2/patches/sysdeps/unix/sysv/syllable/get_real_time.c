#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/time.h>

bigtime_t __get_real_time( void )
{
  bigtime_t nTime;

  INLINE_SYSCALL( get_raw_real_time, 1, &nTime );
  return( nTime );
}

weak_alias( __get_real_time, get_real_time )
