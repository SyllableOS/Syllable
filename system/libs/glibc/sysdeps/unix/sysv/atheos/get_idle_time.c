#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/time.h>


bigtime_t __get_idle_time( int nProcessor )
{
  bigtime_t nTime;

  INLINE_SYSCALL( get_raw_idle_time, 2, &nTime, nProcessor );
  return( nTime );
}

weak_alias( __get_idle_time, get_idle_time )
