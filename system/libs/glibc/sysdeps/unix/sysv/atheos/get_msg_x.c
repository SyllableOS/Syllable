#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>


#include <atheos/msgport.h>

status_t __get_msg_x(  port_id hPort, uint32* pnCode, void* pBuffer, int nSize, bigtime_t nTimeOut )
{
  return( INLINE_SYSCALL( raw_get_msg_x, 5, hPort, pnCode, pBuffer, nSize, &nTimeOut ) );
}

weak_alias( __get_msg_x, get_msg_x )
