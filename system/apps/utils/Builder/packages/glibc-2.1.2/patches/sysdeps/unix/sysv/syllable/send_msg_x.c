#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>


#include <atheos/msgport.h>

status_t __send_msg_x( port_id hPort, uint32 nCode, const void* pBuffer, int nSize, bigtime_t nTimeOut )
{
  return( INLINE_SYSCALL( raw_send_msg_x, 5, hPort, nCode, pBuffer, nSize, &nTimeOut ) );
}


weak_alias( __send_msg_x, send_msg_x )
