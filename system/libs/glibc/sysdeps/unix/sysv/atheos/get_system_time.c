#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/time.h>

bigtime_t __get_system_time( void )
{
  bigtime_t	nTime;

  INLINE_SYSCALL( get_raw_system_time, 1, &nTime );
  return( nTime );
}

weak_alias( __get_system_time, get_system_time )
