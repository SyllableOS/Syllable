#include <atheos/types.h>
#include <atheos/string.h>
#include <atheos/elf.h>
#include <posix/unistd.h>

#include <posix/errno.h>

#include <macros.h>


#include "stage2.h"

void *	AllocSecData( int lSize, uint32 lFlags );
void *	AllocImageData( int lSize, uint32 lFlags );

#define	dbprintf printf
#define	printk printf

int load_kernel_image( int nFile, void** ppEntry, int* pnKernelSize )
{
  Elf32_ElfHdr_s sElfHeader;
  int		 nError;
  uint32     	 nBssStartAddress = -1;
  uint32     	 nBssEndAddress = 0;
  uint32     	 nStartAddress = -1;
  uint32     	 nEndAddress = 0;
  int	     	 nStartOffset = 0;
  int	     	 nTextSize = 0;
  int	     	 nBssSize = 0;
  int	     	 nKernelSize;
  int	     	 i;
	
  nError = read( nFile, &sElfHeader, sizeof( sElfHeader ) );

  if ( sizeof( sElfHeader ) != nError ) {
    printk( "failed to read elf header\n" );
    return( (nError < 0 ) ? nError : -ENOEXEC );
  }
/*
  if ( AELFMAG0 != sElfHeader.e_anIdent[ AEI_MAG0 ] || AELFMAG1 != sElfHeader.e_anIdent[ AEI_MAG1 ] ||
       AELFMAG2 != sElfHeader.e_anIdent[ AEI_MAG2 ] || AELFMAG3 != sElfHeader.e_anIdent[ AEI_MAG3 ] )
  {
    printk( "ERROR : Elf file has bad magic\n" );
    return( -ENOEXEC );
  }
  */
  if ( ELFCLASS32 != sElfHeader.e_anIdent[ EI_CLASS ] || ELFDATALSB != sElfHeader.e_anIdent[ EI_DATA ] ) {
    printk( "ERROR : elf file has wrong bitsize, or endian type\n" );
//    return( -ENOEXEC );
  }

  if ( sElfHeader.e_nSecHdrSize != sizeof( Elf32_SectionHeader_s ) )
  {
    printk( "ERROR : elf file has invalid section header size %d\n", sElfHeader.e_nSecHdrSize );
//    return( -ENOEXEC );
  }

  lseek( nFile, sElfHeader.e_nSecHdrOff, SEEK_SET );
	
  for ( i = 0 ; i < sElfHeader.e_nSecHdrCount ; ++i )
  {
    Elf32_SectionHeader_s sSection;

    nError = read( nFile, &sSection, sElfHeader.e_nSecHdrSize );
    
    if ( sSection.sh_nType == SHT_NULL ) {
      continue;
    }
    if ( sSection.sh_nType == SHT_NOBITS ) {
      if ( nBssStartAddress == -1 ) {
	nBssStartAddress = sSection.sh_nAddress;
      }
      nBssEndAddress = sSection.sh_nAddress + sSection.sh_nSize;
    }
    if (sSection.sh_nType != SHT_NULL ) {
      if ( nStartAddress == -1 ) {
	nStartAddress = sSection.sh_nAddress;
	nStartOffset  = sSection.sh_nOffset;
      }
    }
    if ( sSection.sh_nType == SHT_PROGBITS ) {
      if ( sSection.sh_nAddress + sSection.sh_nSize > nEndAddress ) {
	nEndAddress = sSection.sh_nAddress + sSection.sh_nSize;
      }
    }
  }
/*
  for ( i = 0 ; i < sElfHeader.e_nSecHdrCount ; ++i )
  {
    Elf32_SectionHeader_s sSection;

    nError = read( nFile, &sSection, sElfHeader.e_nSecHdrSize );
    
    if ( sSection.sh_nType == SHT_NULL ) {
      continue;
    }
    if ( sSection.sh_nType == SHT_NOBITS ) {
      if ( nBssStartAddress == -1 ) {
	nBssStartAddress = sSection.sh_nAddress;
      }
      nBssEndAddress = sSection.sh_nAddress + sSection.sh_nSize;
    } else if (sSection.sh_nType == SHT_PROGBITS || sSection.sh_nType == SHT_REL ) {
      if ( nStartAddress == -1 ) {
	nStartAddress = sSection.sh_nAddress;
	nStartOffset  = sSection.sh_nOffset;
      }
      if ( sSection.sh_nAddress + sSection.sh_nSize > nEndAddress ) {
	nEndAddress = sSection.sh_nAddress + sSection.sh_nSize;
      }
    }
  }
*/
  nStartOffset -= nStartAddress & ~PAGE_MASK;
  nStartAddress &= PAGE_MASK;

  if ( (nStartOffset & ~PAGE_MASK) != 0 ) {
    printk( "Error: Unaligned file offset segment %08x\n", nStartOffset );
//    return( -EINVAL );
  }
  if ( (nBssStartAddress & ~PAGE_MASK) != 0 ) {
    printk( "Error: Unaligned bss segment %08x\n", nBssStartAddress );
//    return( -EINVAL );
  }
  

  
  if ( -1 != nStartAddress )
  {
    uint32 nTextAddress;
    nTextAddress = nStartAddress;

    nTextSize = nEndAddress - nStartAddress;
  } else {
    printk( "Error: No text segment\n" );
  }
  if ( -1 != nBssStartAddress ) {
    nBssSize  = nBssEndAddress - nBssStartAddress;
  }

  nKernelSize = nBssEndAddress - 1024 * 1024;
  memset( PHYS_TO_LIN( nStartAddress ), 0, nKernelSize );
  
  lseek( nFile, nStartOffset, SEEK_SET );
  read( nFile, PHYS_TO_LIN( nStartAddress ), nTextSize );

  *ppEntry = (void*) sElfHeader.e_nEntry;
  *pnKernelSize = nKernelSize;
  
  return( nError );
}
