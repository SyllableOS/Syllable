#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>


int __sendto( int nFile, const __ptr_t pBuffer, size_t nSize, int nFlags,
	    const struct sockaddr* psFrom, socklen_t pnFromLen )
{
  struct msghdr sMsg;
  struct iovec	sIov;
  int		nError;
  
  sIov.iov_base = (void*)pBuffer;
  sIov.iov_len  = nSize;
  
  sMsg.msg_iov     = &sIov;
  sMsg.msg_iovlen  = 1;
  sMsg.msg_name    = (void*)psFrom;
  sMsg.msg_namelen = pnFromLen;
  nError = sendmsg( nFile, &sMsg, nFlags );
  return( nError );
}

weak_alias( __sendto, sendto )
