#include <errno.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <sys/debug.h>

int __dbprintf( const char* pzFmt, ... )
{
  INLINE_SYSCALL( DebugPrint, 2, pzFmt, ((char**)(&pzFmt)) + 1 );
  return( 0 );
}

weak_alias( __dbprintf, dbprintf )
