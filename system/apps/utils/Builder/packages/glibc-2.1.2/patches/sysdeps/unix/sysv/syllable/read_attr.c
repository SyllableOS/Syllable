#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/filesystem.h>

ssize_t __read_attr( int nFile, const char *pzAttrName, int nType, void* pBuffer, off_t nPos, size_t nLen )
{
  WriteAttrParams_s sParams;

  sParams.wa_nFile   = nFile;
  sParams.wa_pzName  = pzAttrName;
  sParams.wa_nFlags  = 0;
  sParams.wa_nType   = nType;
  sParams.wa_pBuffer = pBuffer;
  sParams.wa_nPos    = nPos;
  sParams.wa_nLen    = nLen;

  return( INLINE_SYSCALL( read_attr, 1, &sParams ) );
}

weak_alias( __read_attr, read_attr )
