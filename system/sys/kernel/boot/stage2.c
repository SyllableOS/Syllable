#include <atheos/types.h>
#include <atheos/v86.h>
#include <atheos/isa_io.h>
#include <atheos/string.h>
#include <posix/fcntl.h>
#include <posix/unistd.h>

#include <macros.h>

#include "stage2.h"


char*	RealPage = (char*)0xdeadbabe; // Avoid using the bss segment.

extern	realint( int , struct RMREGS * );


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int realint( int nIntNum, struct RMREGS* psParams )
{
  int	nError;
  
  __asm__ volatile ("push %%ebx; mov %%ecx,%%ebx; int $0x30; pop %%ebx;" :
		    "=a" (nError) : "0" (nIntNum), "c" ((int)psParams) );
  
  return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void debug_write( const char* pBuffer, int nSize )
{
  int	i;
  
  for ( i = 0 ; i < nSize ; ++i )
  {
    int j = 0;
		
    while( (inb_p( 0x2fd ) & 0x40) == 0 && j < 100000 )
      j++;
		
    outb_p( pBuffer[i], 0x2f8 );

    if ( pBuffer[i] == '\n' )
    {
      while( (inb_p( 0x2fd ) & 0x40) == 0 && j < 100000 )
	j++;
		
      outb_p( '\r', 0x2f8 );
    }
  }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int parse_init_script( const char* pBuffer, int nBufferSize, char* pzKernelPath )
{
  int nEnvCnt = 0;
  int nStart = 0;
  int i;

  for ( i = 0 ; i < nBufferSize ; ++i )
  {
    if ( '\n' == pBuffer[i] || '\r' == pBuffer[i] )
    {
      int nEnd = i;
      if ( nStart < nEnd )
      {
	if ( strncmp( "KERNEL_PATH", &pBuffer[nStart], 11 ) == 0 )
	{
	  nStart += 12;	/* Skip command + "=" */

	  if ( nStart < nEnd )
	  {
	    memcpy( pzKernelPath, &pBuffer[nStart], nEnd - nStart );
	    pzKernelPath[ nEnd - nStart ] = '\0';
	  }
	}
#if 0
	if ( strncmp( "SYS_PATH", &pBuffer[nStart], 8 ) == 0 )
	{
	  nStart += 9;	/* Skip command + "=" */

	  if ( nStart < nEnd )
	  {
	    g_pzSysPath = AllocVec( nEnd - nStart + 1, MEMF_KERNEL );

	    if ( NULL != g_pzSysPath )
	    {
	      memcpy( g_pzSysPath, &pBuffer[nStart], nEnd - nStart );
	      g_pzSysPath[ nEnd - nStart ] = '\0';

	      dbprintf( "Set system path = '%s'\n", g_pzSysPath );
	    }
	  }
	}

	if ( strncmp( "SET", &pBuffer[nStart], 3 ) == 0 )
	{
	  nStart += 4;	/* Skip command + "=" */

	  if ( nStart < nEnd )
	  {
	    apzEnviron[ nEnvCnt ] = AllocVec( nEnd - nStart + 1, MEMF_KERNEL );
	    memcpy( apzEnviron[ nEnvCnt ], &pBuffer[nStart], nEnd - nStart );
	    apzEnviron[ nEnvCnt ][ nEnd - nStart ] = '\0';
	    dbprintf( "Set environment '%s'\n", apzEnviron[ nEnvCnt ] );
	    nEnvCnt++;
	  }
	}
#endif
      }
      nStart = nEnd + 1;
    }
  }
  return( nEnvCnt );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void parse_args( char* pzScriptPath )
{
  int	i,j;
  
  for ( i = 0 ; loader_args[i] == ' ' && loader_args[i] != '\0' ; ++i )
      /*** EMPTY ***/;
  for ( j = 0 ; loader_args[i] != ' ' && loader_args[i] != '\0' ; ++i, ++j ) {
    pzScriptPath[j] = loader_args[i];
  }
  pzScriptPath[j] = '\0';
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int main()
{
  char 	zKernelPath[ 256 ];
  char	zScriptPath[256];
  char	zBuffer[256];
  char*	pInitScript;
  void* pEntry;
  int	nKernelSize;
  int	nFile;
  int	nScriptFile;
  int	nScriptSize;
  int	nError;

  RealPage = first_free_addr;
  
  parse_args( zScriptPath );

  printf( "Load init script '%s'\n", zScriptPath );

  nScriptFile = open( zScriptPath, O_RDONLY );
  
  if ( nScriptFile < 0 ) {
    printf( "Failed to open init script!\n" );
    return( 1 );
  }
  
  nScriptSize = lseek( nScriptFile, 0, SEEK_END );
  lseek( nScriptFile, 0, SEEK_SET );

  pInitScript = RealPage;
  RealPage += (nScriptSize + 15) & ~15;

  if ( read( nScriptFile, pInitScript, nScriptSize ) != nScriptSize ) {
    printf( "Error: failed to read init script!\n" );
  }

  close( nScriptFile );

  parse_init_script( pInitScript, nScriptSize, zKernelPath );

  printf( "Load kernel '%s'\n", zKernelPath );
  
  nFile = open( zKernelPath, O_RDONLY );

  if ( nFile < 0 ) {
    printf( "Error: failed to open kernel image.\n" );
  }
  nError = load_kernel_image( nFile, &pEntry, &nKernelSize );
  close( nFile );
  
  if ( nError < 0 ) {
    printf( "Error: failed to load kernel!\n" );
  }
  printf( "Enter kernel %x\n", pEntry );
  start_kernel( pEntry, ((uint32)LIN_TO_PHYS( first_free_addr ) + 15) & ~15,
		nKernelSize, LIN_TO_PHYS( pInitScript ), nScriptSize );
  return( 0 );
}
