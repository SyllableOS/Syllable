#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>


int __recvfrom( int nFile, void* pBuffer, size_t nSize, int nFlags,
	      struct sockaddr* psFrom, socklen_t* pnFromLen )
{
  struct msghdr sMsg;
  struct iovec	sIov;
  int		nError;
  
  sIov.iov_base = pBuffer;
  sIov.iov_len  = nSize;
  
  sMsg.msg_iov     = &sIov;
  sMsg.msg_iovlen  = 1;
  sMsg.msg_name    = psFrom;
  sMsg.msg_namelen = *pnFromLen;
  nError = recvmsg( nFile, &sMsg, nFlags );
  *pnFromLen = sMsg.msg_namelen;
  return( nError );
}

weak_alias( __recvfrom, recvfrom )
