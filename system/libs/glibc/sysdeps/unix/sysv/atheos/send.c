#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>


ssize_t __send(int nFile, const void* pBuffer, size_t nSize, int nFlags)
{
  return( sendto( nFile, pBuffer, nSize, nFlags, NULL, 0 ) );
}

weak_alias( __send, send )
