#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>


ssize_t __recv(int nFile, void* pBuffer, size_t nSize, int nFlags)
{
  struct sockaddr sAddr; // FIXME: Should be removed
  int	nAddrLen = sizeof( sAddr );
  
  return( recvfrom( nFile, pBuffer, nSize, nFlags, &sAddr, &nAddrLen ) );
}

weak_alias( __recv, recv )
