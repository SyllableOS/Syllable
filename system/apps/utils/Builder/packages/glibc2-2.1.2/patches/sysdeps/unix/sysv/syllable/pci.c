#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/types.h>
#include <atheos/pci.h>

uint32 __read_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize )
{
  uint32 nValue;
  int	 nError;

  INLINE_SYSCALL( raw_read_pci_config, 5, nBusNum, (nDevNum << 16) | nFncNum, nOffset, nSize, &nValue );
  return( nValue );
}


status_t __write_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue )
{
  int			nError;
  nError = INLINE_SYSCALL( raw_write_pci_config, 5, nBusNum, (nDevNum << 16) | nFncNum, nOffset, nSize, nValue );
  return( nError );
}

weak_alias( __read_pci_config, read_pci_config )
weak_alias( __write_pci_config, write_pci_config )
