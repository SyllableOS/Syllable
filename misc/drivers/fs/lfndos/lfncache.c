/*
 *  lfndos - AtheOS filesystem that use MS-DOS to store/retrive files
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <atheos/ctype.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>
#include <posix/errno.h>
#include <posix/unistd.h>
#include <posix/stat.h>

#include <atheos/types.h>

#include <atheos/filesystem.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include "lfndos.h"
#include "dos.h"

#include <macros.h>

static	CacheBuffer_s*	g_psFirstBuffer = NULL;
static	CacheBuffer_s*	g_psLastBuffer	=	NULL;


struct
{
  FileEntry_s*	psEntry;
  int		nFileDesc;
} g_apsOpenedEntries[ LFD_NUM_DOS_HANDLES ];

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_AssertValidBuffer( CacheBuffer_s* psBuffer )
{
  if ( NULL == psBuffer ) {
    printk( "WARNING lfd_AssertValidBuffer() calles with NULL pointer\n" );
    return( -1 );
  }

  if ( 0x12345678 != psBuffer->cb_nMagic1 ) {
    printk( "WARNING : cb_nMagic1 overwritten! New value = %x\n", psBuffer->cb_nMagic1 );
    return( -1 );
  }

  if ( 0x87654321 != psBuffer->cb_nMagic2 ) {
    printk( "WARNING : cb_nMagic2 overwritten! New value = %x\n", psBuffer->cb_nMagic2 );
    return( -1 );
  }

  if ( FALSE != psBuffer->cb_bDirty && TRUE != psBuffer->cb_bDirty ) {
    printk( "Invalid value %d in cb_bDirty!!!\n", psBuffer->cb_bDirty );
    return( -1 );
  }

  if ( FALSE != psBuffer->cb_bDirty && NULL == psBuffer->cb_psFileEntry ) {
    printk( "WARNING : Cache buffer marked as dirty, but does not point to a valid file entry!!!\n" );
    return( -1 );
  }

  if ( psBuffer->cb_nBytesUsed < 0 ) {
    printk( "WARNING : negative size %d marked as used cache buffer!!!\n",
	      psBuffer->cb_nBytesUsed );
    return( -1 );
  }

  if ( psBuffer->cb_nBytesUsed > CACHE_BLOCK_SIZE ) {
    printk( "WARNING : %d bytes marked as used in a %d byte large cache buffer!!!\n",
	      psBuffer->cb_nBytesUsed, CACHE_BLOCK_SIZE );
    return( -1 );
  }


  if ( NULL != psBuffer->cb_psNext ) {
    if ( psBuffer->cb_psNext->cb_psPrev != psBuffer ) {
      printk( "WARNING : Cache buffer pointed to by cb_psNext does not point back to this buffer!!!\n" );
      return( -1 );
    }
  }

  if ( NULL != psBuffer->cb_psPrev) {
    if ( psBuffer->cb_psPrev->cb_psNext != psBuffer ) {
      printk( "WARNING : Cache buffer pointed to by cb_psPrev does not point back to this buffer!!!\n" );
      return( -1 );
    }
  }
  return( 0 );
}

/** Get the DOS file descriptor for a file node
 * \par Description:
 *	Scan the list of open DOS file for the file descriptor. If not found
 *	we check how many files are open, and if necesary close the oldest
 *	file, before opening the file described by the node.
 *
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

static int lfd_get_entry_file_handle( FileEntry_s* psEntry )
{
  char	zFullPath[ 300 ];
  int		nPathLen;
  int		i;

    /* Check if already opened */

  for ( i = 0 ; i < LFD_NUM_DOS_HANDLES ; ++i ) {
    if ( g_apsOpenedEntries[ i ].psEntry == psEntry ) {
      return( g_apsOpenedEntries[ i ].nFileDesc );
    }
  }

    /* Flush last entry */

  if ( -1 != g_apsOpenedEntries[ LFD_NUM_DOS_HANDLES - 1 ].nFileDesc ) {
    dos_close( g_apsOpenedEntries[ LFD_NUM_DOS_HANDLES - 1 ].nFileDesc );
  }
  memmove( &g_apsOpenedEntries[ 1 ],
	   &g_apsOpenedEntries[ 0 ],
	   sizeof( g_apsOpenedEntries[0] ) * (LFD_NUM_DOS_HANDLES - 1)  );

    /* Open file */

  nPathLen = lfd_GetFullPath( psEntry, zFullPath );

  if ( -1 != nPathLen )
  {
//    if ( psEntry->fe_nFlags & FE_FILE_EXISTS )
//    {
      g_apsOpenedEntries[ 0 ].nFileDesc = dos_open( zFullPath, O_RDWR, S_IRWXU );
//    }
//    else
//    {
//      g_apsOpenedEntries[ 0 ].nFileDesc = dos_create( zFullPath );
//      psEntry->fe_nFlags |= FE_FILE_EXISTS | FE_FILE_SIZE_VALID;
//    }

    if ( -1 != g_apsOpenedEntries[ 0 ].nFileDesc ) {
      g_apsOpenedEntries[ 0 ].psEntry = psEntry;
      return( g_apsOpenedEntries[ 0 ].nFileDesc );
    } else {
      printk( "ERROR : lfd_get_entry_file_handle() failed to open '%s'\n", zFullPath );
    }
  }
  return( -1 );
}

/** Get rid of a DOS file handle
 * \par Description:
 *	If psEntry is NULL all open files is closed else
 *	the file pointed at by psEntry is closed.
 * \param psEntry - Pointer to the file to close, or NULL to close the oldes open file
 * \sa lfd_get_entry_file_handle()
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

void lfd_close_dos_handle( FileEntry_s* psEntry )
{
  int		i;


  if ( NULL == psEntry )
  {
    for ( i = 0 ; i < LFD_NUM_DOS_HANDLES ; ++i ) {
      if ( -1 != g_apsOpenedEntries[ i ].nFileDesc ) {
	dos_close( g_apsOpenedEntries[ i ].nFileDesc );
	g_apsOpenedEntries[ i ].nFileDesc = -1;
	g_apsOpenedEntries[ i ].psEntry  = NULL;
      }
    }
  }
  else
  {
//    if ( psEntry->fe_nFlags & FE_FILE_EXISTS )
    {
      for ( i = 0 ; i < LFD_NUM_DOS_HANDLES ; ++i )
      {
	if ( g_apsOpenedEntries[ i ].psEntry == psEntry )
	{
	  dos_close( g_apsOpenedEntries[ i ].nFileDesc );
	  g_apsOpenedEntries[ i ].nFileDesc = -1;
	  g_apsOpenedEntries[ i ].psEntry 	= NULL;
	  break;
	}
      }
    }
  }
}

/** Unlink a cache buffer from the global list.
 * \param psBuffer - The buffer to unlink
 * \sa lfd_append_buffer
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

static void lfd_remove_buffer( CacheBuffer_s* psBuffer )
{
  if ( psBuffer == g_psFirstBuffer ) {
    g_psFirstBuffer = psBuffer->cb_psNext;
  }
  if ( psBuffer == g_psLastBuffer ) {
    g_psLastBuffer = psBuffer->cb_psPrev;
  }

  if ( NULL != psBuffer->cb_psNext )
  {
    kassertw( psBuffer->cb_psNext->cb_psPrev == psBuffer );
    psBuffer->cb_psNext->cb_psPrev = psBuffer->cb_psPrev;
  }

  if ( NULL != psBuffer->cb_psPrev )
  {
    kassertw( psBuffer->cb_psPrev->cb_psNext == psBuffer );
    psBuffer->cb_psPrev->cb_psNext = psBuffer->cb_psNext;
  }
  psBuffer->cb_psNext = psBuffer->cb_psPrev = NULL;
}

/****** msfs.filesystem/ ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static void lfd_append_buffer( CacheBuffer_s* psBuffer )
{
  if ( lfd_AssertValidBuffer( psBuffer ) != 0 ) {
    printk( "Warning: lfd_append_buffer() -> Invalide cache buffer at %p!!!!\n", psBuffer );
    return;
  }

  if ( NULL != g_psLastBuffer )
  {
    g_psLastBuffer->cb_psNext	= psBuffer;
    psBuffer->cb_psPrev 	= g_psLastBuffer;
    g_psLastBuffer		= psBuffer;
    psBuffer->cb_psNext		= NULL;
  }
  else
  {
    g_psFirstBuffer = g_psLastBuffer = psBuffer;
    psBuffer->cb_psNext = psBuffer->cb_psPrev = NULL;
  }
}

/** Find the cache buffer covering a given offset in a given file
 * \par Description:
 *	Scan the list of cache buffers to find the one that contain the byte
 *	at nOffset in file psEntry.
 *
 * \param psEntry - The file to search
 * \param nOffset - The file offset to match
 * \return Pointer to the cache buffer, or null if the area was not cached
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

static CacheBuffer_s* lfd_find_buffer( FileEntry_s* psEntry, int nOffset )
{
  CacheBuffer_s* psBuffer;

  kassertw( (nOffset & (CACHE_BLOCK_SIZE -1)) == 0 );

  for ( psBuffer = g_psLastBuffer ; NULL != psBuffer ; psBuffer = psBuffer->cb_psPrev )
  {
    if ( psBuffer->cb_nOffset == nOffset && psBuffer->cb_psFileEntry == psEntry ) {

      if ( lfd_AssertValidBuffer( psBuffer ) != 0 ) {
	printk( "WARNING : FindBuffer() -> Invalide cache buffer at %p!!!!\n", psBuffer );
	return( NULL );
      }
      return( psBuffer );
    }
  }
  return( NULL );
}

/** Write a dirty cache buffer to disk.
 * \par Description:
 *	If the buffer is dirty it is written to the file it belongs to and the
 *	dirty flag is cleared. If the buffer is clean alread this is a no-op.
 * \param psBuffer - The buffer to flush
 * \return 0 if noting whent wrong else a negative errorcode is returned.
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////


static int lfd_flush_buffer( CacheBuffer_s* psBuffer )
{
  int	nError;
  int	hFileHandle = -1;

  if ( NULL == psBuffer ) {
    printk( "ERROR : FlushBuffer() called with psBuffer == NULL!\n" );
    return( -EINVAL );
  }

  if ( lfd_AssertValidBuffer( psBuffer ) != 0 ) {
    printk( "ERROR : Invalide cache buffer at %p!!!!\n", psBuffer );
    return( -EINVAL );
  }

  if ( psBuffer->cb_bDirty )
  {
/*
  printk( "Flush %ld bytes of '%s' at %ld\n",
  psBuffer->cb_nBytesUsed, psBuffer->cb_psFileEntry->fe_zShortName,
  psBuffer->cb_nOffset / CACHE_BLOCK_SIZE );
  */
    psBuffer->cb_bDirty	= false;

    hFileHandle = lfd_get_entry_file_handle( psBuffer->cb_psFileEntry );


    if ( -1 == hFileHandle ) {
      printk( "ERROR : lfd, Unable to open '%s' for flushing!!\n",
		psBuffer->cb_psFileEntry->fe_zLongName );
      return( -1 );
    }

    nError = dos_lseek( hFileHandle, psBuffer->cb_nOffset, SEEK_SET );

    if ( nError == -1)	{
      printk( "ERROR : Unable to seek to %d in '%s'\n",
		psBuffer->cb_nOffset, psBuffer->cb_psFileEntry->fe_zLongName );
      return( -1 );
    }

    nError = dos_write( hFileHandle, psBuffer->cb_aData, psBuffer->cb_nBytesUsed );
    dos_setftime( hFileHandle, psBuffer->cb_psFileEntry->fe_nMTime );
    Schedule();

    if ( nError != psBuffer->cb_nBytesUsed )
    {
      printk( "ERROR : Unable to write %d bytes from '%s'\n",
		psBuffer->cb_nBytesUsed, psBuffer->cb_psFileEntry->fe_zLongName );
      return( -1 );
    }
  }
  return( 0 );
}

/** Write all dirty cache buffers for a file to disk.
 * \par Description:
 *	Scan through all cache buffers writing all dirty buffers belonging to
 *	the given file to disk.
 *
 *	If bDelete is true, no buffers will actually be written to disk.
 *	if bRelease is true, it will disassociate the buffers from the file
 *
 * \param psEntry - The file whos buffers should be flushed.
 * \param bRelease - Should be true if the buffers should be released
 * \param bDelete - True if the file is scheduled for deletion, to avoid spendin
 *	      	    time writing stuff to a doomed file.
 * \return
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

static void lfd_flush_node_buffers( FileEntry_s* psEntry, bool bRelease, bool bDelete )
{
  CacheBuffer_s*	psBuffer;

#if 0  
  if ( bDelete == false && (psEntry->fe_nFlags & FE_FILE_EXISTS) == 0 )
  {
    char	zFullPath[ 300 ];
    int		nFileDesc;
		
    lfd_GetFullPath( psEntry, zFullPath );
		
    nFileDesc = dos_create( zFullPath );	// FIX ME FIX ME : Check error code!!!!!
    dos_close( nFileDesc );
    psEntry->fe_nFlags |= FE_FILE_EXISTS;
  }
#endif	
	
  for ( psBuffer = g_psFirstBuffer ; NULL != psBuffer ; psBuffer = psBuffer->cb_psNext )
  {
    if ( psBuffer->cb_psFileEntry == psEntry )
    {
      if ( bDelete == false ) {
	lfd_flush_buffer( psBuffer );
      }

      if ( bRelease ) {
	psBuffer->cb_psFileEntry = NULL;
	psBuffer->cb_bDirty	 = false;
      }
    }
  }
}

static int cmp_blocks( const void* pBlk1, const void* pBlk2 )
{
  const CacheBuffer_s* const * psBlk1 = pBlk1;
  const CacheBuffer_s* const * psBlk2 = pBlk2;

  if ( (*psBlk1)->cb_psFileEntry == (*psBlk2)->cb_psFileEntry ) {
    return( (*psBlk1)->cb_nOffset - (*psBlk2)->cb_nOffset );
  }
  return( ((int)(*psBlk1)->cb_psFileEntry) - ((int)(*psBlk2)->cb_psFileEntry) );
}

/** Write the 8 least resently buffers to disk
 * \return
 *	True if any blocks was written to disk.
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

bool lfd_flush_some_buffers()
{
  CacheBuffer_s* apsBuffers[8];
  CacheBuffer_s* psBuffer;
  bool		 bFoundDirty = false;
  int i = 0;

  for ( psBuffer = g_psFirstBuffer ; psBuffer != NULL && i < 8 ; psBuffer = psBuffer->cb_psNext ) {
    if ( psBuffer->cb_bDirty ) {
      apsBuffers[i++] = psBuffer;
      bFoundDirty = true;
    }
  }
  
  if ( bFoundDirty ) {
    int j;

    qsort( apsBuffers, i, sizeof( CacheBuffer_s* ), cmp_blocks );
    
    for ( j = 0 ; j < i ; ++j ) {
      if ( apsBuffers[j]->cb_psFileEntry != NULL ) {
	lfd_flush_buffer( apsBuffers[j] );
      } else {
	printk( "Panic: lfd_flush_buffer() found dirty buffer with no file entry\n" );
	apsBuffers[j]->cb_bDirty = false;
      }
    }
  }
  return( bFoundDirty );
}

/****** msfs.filesystem/ ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static int lfd_read_buffer( CacheBuffer_s* psBuffer )
{
  int	nError;
  int	nFileHandle;

  if ( NULL == psBuffer ) {
    printk( "Error: ReadBuffer() called with psBuffer == NULL!\n" );
    return( -1 );
  }

  if ( lfd_AssertValidBuffer( psBuffer ) != 0 ) {
    printk( "Warning: ReadBuffer() -> Invalide cache buffer at %p!!!!\n", psBuffer );
    return( -1 );
  }


  nFileHandle = lfd_get_entry_file_handle( psBuffer->cb_psFileEntry );

  if ( -1 == nFileHandle ) {
    printk( "Error: lfd, unable to reopen '%s'\n", psBuffer->cb_psFileEntry->fe_zLongName );
    return( -1 );
  }

  nError = dos_lseek( nFileHandle, psBuffer->cb_nOffset, SEEK_SET );
  if ( -1 != nError ) {
    nError = dos_read( nFileHandle, &psBuffer->cb_aData[0], psBuffer->cb_nBytesUsed );
  } else {
    printk( "Error: ReadBuffer() Unable to seek to %d in '%s'\n",
	      psBuffer->cb_nOffset, psBuffer->cb_psFileEntry->fe_zLongName );
  }
  Schedule();
/*
  if ( nError == -1)	{
  printk( "ERROR : Unable to read %ld bytes from '%s'\n", psBuffer->cb_nBytesUsed, psBuffer->cb_psFileEntry->fe_zLongName );
  return( -1 );
  }

  if ( nError != psBuffer->cb_nBytesUsed ) {
  printk( "ERROR : Unable to read %ld bytes from '%s'\n", psBuffer->cb_nBytesUsed, psBuffer->cb_psFileEntry->fe_zLongName );
  return( -1 );
  }
  */
  return( 0 );
}

/****** msfs.filesystem/ ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

void	lfd_TouchBuffer( CacheBuffer_s* psBuffer )
{
  if ( lfd_AssertValidBuffer( psBuffer ) != 0 ) {
    printk( "Warning: TouchBuffer() -> Invalide cache buffer at %p!!!!\n", psBuffer );
    return;
  }

  lfd_remove_buffer( psBuffer );
  lfd_append_buffer( psBuffer );
}

/****** msfs.filesystem/ ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

CacheBuffer_s*	lfd_AllocBuffer( FileEntry_s* psEntry )
{
  CacheBuffer_s*	psBuffer = g_psFirstBuffer;

  kassertw( NULL != psBuffer );

  lfd_TouchBuffer( psBuffer );	/* Make it most reacent	used */

  if ( NULL != psBuffer->cb_psFileEntry )
  {
    if ( psBuffer->cb_bDirty ) {
      lfd_flush_buffer( psBuffer );
    }
    psBuffer->cb_psFileEntry = psEntry;
  }
  return( psBuffer );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_GetFullPath( FileEntry_s* psNode, char* pzBuffer )
{
  if ( NULL != psNode->fe_psParent )
  {
    int	nLen;

    nLen = lfd_GetFullPath( psNode->fe_psParent, pzBuffer );

    if ( -1 != nLen )
    {
      pzBuffer[ nLen ]	=	'\\';
      nLen++;
      strcpy( &pzBuffer[ nLen ], psNode->fe_zShortName );

      return( nLen + strlen( psNode->fe_zShortName ) );
    }
    else
    {
      return( -1 );
    }
  }
  else
  {
    strcpy( pzBuffer, psNode->fe_zShortName );
    return( strlen( psNode->fe_zShortName ) );
  }
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static int lfd_load_old_directory_info( FileEntry_s* psParent, char* pzPath, int nFileHandle )
{
  OldFileNodeInfo_s*	psInodeInfo;
  int			nNumEntries;
  int			nError = 0;
  int			nBytesRead;

  nBytesRead = dos_read( nFileHandle, &nNumEntries, sizeof( nNumEntries ) );

  psInodeInfo = kmalloc( sizeof( OldFileNodeInfo_s ) * 100, MEMF_KERNEL );

  if ( NULL != psInodeInfo )
  {
    if ( sizeof( nNumEntries ) == nBytesRead )
    {
      int	i;
      int	nNumInBuf = 0;


      for ( i = 0 ; i < nNumEntries ; ++i )
      {
	if ( nNumInBuf == 0 )
	{
	  nNumInBuf = min( nNumEntries - i, 100 );

	  nBytesRead = dos_read( nFileHandle, psInodeInfo, sizeof( OldFileNodeInfo_s ) * nNumInBuf );
	  nError = ( nBytesRead == sizeof( OldFileNodeInfo_s ) * nNumInBuf ) ? 0 : -1;

	  if ( 0 != nError ) {
	    printk( "ERROR : LoadDirectoryData() could only read %d of %d entries\n",
		      i, nNumEntries );
	    printk( "Could only read %d of %d bytes\n",
		      nBytesRead, sizeof( OldFileNodeInfo_s ) * nNumInBuf );
	    break;
	  }
	}
	nNumInBuf--;
	{
	  FileEntry_s*	psEntry;

	  psEntry = kmalloc( sizeof( FileEntry_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

	  if ( NULL != psEntry )
	  {
	    psEntry->fe_nFlags = FE_FILE_EXISTS;

	    if ( DOS_S_ISDIR( psInodeInfo[nNumInBuf].nMode ) ) {
	      psEntry->fe_u.psFirstChild = NULL;
	    } else {
	      psEntry->fe_u.nSize = psInodeInfo[nNumInBuf].nSize;
	    }

	    strcpy( psEntry->fe_zShortName, psInodeInfo[nNumInBuf].zShortName );
	    strcpy( psEntry->fe_zLongName, psInodeInfo[nNumInBuf].zLongName );

	    psEntry->fe_nMode	= psInodeInfo[nNumInBuf].nMode;
	    psEntry->fe_nATime	= psInodeInfo[nNumInBuf].nATime;
	    psEntry->fe_nMTime	= psInodeInfo[nNumInBuf].nMTime;
	    psEntry->fe_nCTime	= psInodeInfo[nNumInBuf].nCTime;

	    psEntry->fe_psNext	= psParent->fe_u.psFirstChild;
	    psParent->fe_u.psFirstChild	=	psEntry;
	    psEntry->fe_psParent	=	psParent;
	  }
	  else
	  {
	    nError = -ENOMEM;
	    printk( "ERROR : lfd_load_old_directory_info() Could not alloc memory for new entry!\n" );
	  }
	}
      }
    }
    else
    {
      nError = -EINVAL;
    }
    kfree( psInodeInfo );
  }
  else
  {
    nError = -ENOMEM;
    printk( "ERROR : lfd_load_old_directory_info() could not alloc memory for disk buffer\n" );
  }
  return( nError );
}

static int lfd_load_directory_info( FileEntry_s* psParent, char* pzPath, int nFileHandle )
{
  FileInfoHeader_s	sHeader;
  FileNodeInfo_s*	psInodeInfo;
//  int			nNumEntries;
  int			nError = 0;
  int			nBytesRead;
  int			nNumInBuf = 0;
  int			i;

  nBytesRead = dos_read( nFileHandle, &sHeader, sizeof( sHeader ) );

  if ( sizeof( sHeader ) != nBytesRead ) {
    printk( "Error: lfd_load_directory_info() failed to read table header\n" );
    return( -EIO );
  }

  if ( sHeader.nVersion != 1 ) {
    printk( "Error: lfd_load_directory_info() Inode table has unknown version number %d! Current version is %d\n",
	    sHeader.nVersion, LFD_CUR_INODE_FILE_VERSION );
    return( -EINVAL );
  }
  
  psInodeInfo = kmalloc( sizeof( FileNodeInfo_s ) * 100, MEMF_KERNEL );

  if ( psInodeInfo == NULL ) {
    printk( "ERROR : lfd_load_directory_info() could not alloc memory for disk buffer\n" );
    return( -ENOMEM );
  }
  
  for ( i = 0 ; i < sHeader.nNumEntries ; ++i )
  {
    FileEntry_s* psEntry;
    if ( nNumInBuf == 0 ) {
      nNumInBuf = min( sHeader.nNumEntries - i, 100 );

      nBytesRead = dos_read( nFileHandle, psInodeInfo, sizeof( FileNodeInfo_s ) * nNumInBuf );
      nError = ( nBytesRead == sizeof( FileNodeInfo_s ) * nNumInBuf ) ? 0 : -1;

      if ( 0 != nError ) {
	printk( "ERROR : LoadDirectoryData() could only read %d of %d entries\n",
		i, sHeader.nNumEntries );
	printk( "Could only read %d of %d bytes\n",
		nBytesRead, sizeof( FileNodeInfo_s ) * nNumInBuf );
	break;
      }
    }
    nNumInBuf--;

    psEntry = kmalloc( sizeof( FileEntry_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

    if ( psEntry == NULL ) {
      nError = -ENOMEM;
      printk( "ERROR : lfd_load_directory_info() Could not alloc memory for new entry!\n" );
      break;
    }
    psEntry->fe_nFlags = FE_FILE_EXISTS;

    if ( DOS_S_ISDIR( psInodeInfo[nNumInBuf].nMode ) ) {
      psEntry->fe_u.psFirstChild = NULL;
    } else {
      psEntry->fe_u.nSize = psInodeInfo[nNumInBuf].nSize;
    }

    strcpy( psEntry->fe_zShortName, psInodeInfo[nNumInBuf].zShortName );
    strcpy( psEntry->fe_zLongName, psInodeInfo[nNumInBuf].zLongName );

    psEntry->fe_nMode	= psInodeInfo[nNumInBuf].nMode;
    psEntry->fe_nUID	= psInodeInfo[nNumInBuf].nUID;
    psEntry->fe_nGID	= psInodeInfo[nNumInBuf].nGID;
    psEntry->fe_nATime  = psInodeInfo[nNumInBuf].nATime;
    psEntry->fe_nMTime  = psInodeInfo[nNumInBuf].nMTime;
    psEntry->fe_nCTime  = psInodeInfo[nNumInBuf].nCTime;

    psEntry->fe_psNext	  = psParent->fe_u.psFirstChild;
    psParent->fe_u.psFirstChild = psEntry;
    psEntry->fe_psParent	  = psParent;
  }

  kfree( psInodeInfo );
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_write_directory_info( FileEntry_s* psParent )
{
  FileInfoHeader_s sHeader;
  char		zPath[ 300 ];
  int		nLen = lfd_GetFullPath( psParent, zPath );
  int		nError = 0;
  FileEntry_s*	psTabEntry;
  FileEntry_s*	psEntry;
  int		nFilePos = 0;
  int		nBytesWritten;
  
  strcat( zPath, "\\" );
  nLen++;

  strcpy( zPath + nLen, LFD_INODE_FILE_NAME );

  nError = lfd_LookupFile( psParent, LFD_INODE_FILE_NAME, strlen( LFD_INODE_FILE_NAME ),
			   &psTabEntry );

  if ( 0 != nError ) {
    nError = lfd_create_file_entry( psParent, LFD_INODE_FILE_NAME, strlen( LFD_INODE_FILE_NAME ),
				    DOS_S_IFREG, &psTabEntry );
  }
  if ( nError != 0 ) {
    printk( "ERROR : lfd_write_directory_info() could not open info file!\n" );
    return( -ENOENT );
  }


  strcpy( sHeader.zWarning, LFD_INODE_WARNING );
  sHeader.nVersion = LFD_CUR_INODE_FILE_VERSION;
  
  for ( sHeader.nNumEntries = 0 , psEntry = psParent->fe_u.psFirstChild ; NULL != psEntry ; psEntry = psEntry->fe_psNext ) {
    sHeader.nNumEntries++;
  }
  nBytesWritten = lfd_CachedWrite( psTabEntry, &sHeader, nFilePos, sizeof( sHeader ) );
  nFilePos += sizeof( sHeader );

  if ( sizeof( sHeader ) != nBytesWritten ) {
    printk( "lfd_write_directory_info() was not able to write entry count to file!\n" );
    return( -EIO );
  }
  
  for ( psEntry = psParent->fe_u.psFirstChild ;
	NULL != psEntry ;
	psEntry = psEntry->fe_psNext )
  {
    FileNodeInfo_s	sInodeInfo;

    strcpy( sInodeInfo.zShortName, psEntry->fe_zShortName );
    strcpy( sInodeInfo.zLongName, psEntry->fe_zLongName );


    if ( DOS_S_ISDIR( psEntry->fe_nMode ) ) {
      sInodeInfo.nSize  = 0;
    } else {
      sInodeInfo.nSize  = psEntry->fe_u.nSize;
    }

    sInodeInfo.nMode  = psEntry->fe_nMode;
    sInodeInfo.nUID   = psEntry->fe_nUID;
    sInodeInfo.nGID   = psEntry->fe_nGID;
    sInodeInfo.nATime = psEntry->fe_nATime;
    sInodeInfo.nMTime = psEntry->fe_nMTime;
    sInodeInfo.nCTime = psEntry->fe_nCTime;

    lfd_CachedWrite( psTabEntry, &sInodeInfo, nFilePos, sizeof( sInodeInfo ) );
    nFilePos += sizeof( sInodeInfo );
  }
  
  return( nError );
}

#if 0 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int lfd_write_directory_info( FileEntry_s* psParent )
{
  char		zPath[ 300 ];
  int		nLen = lfd_GetFullPath( psParent, zPath );
  int		nError = 0;
  FileEntry_s*	psTabEntry;

  strcat( zPath, "\\" );
  nLen++;

  strcpy( zPath + nLen, LFD_INODE_FILE_NAME );

  nError = lfd_LookupFile( psParent, LFD_INODE_FILE_NAME, strlen( LFD_INODE_FILE_NAME ),
			   &psTabEntry );

  if ( 0 != nError ) {
    nError = lfd_create_file_entry( psParent, LFD_INODE_FILE_NAME, strlen( LFD_INODE_FILE_NAME ),
				    DOS_S_IFREG, &psTabEntry );
  }

  if ( 0 == nError )
  {
    FileEntry_s*	psEntry;
    int	nNumEntries;
    int	nFilePos = 0;
    int	nBytesWritten;

    for ( nNumEntries = 0 , psEntry = psParent->fe_u.psFirstChild ;
	  NULL != psEntry ;
	  psEntry = psEntry->fe_psNext )
    {
      nNumEntries++;
    }

    nBytesWritten = lfd_CachedWrite( psTabEntry, &nNumEntries, nFilePos, sizeof( nNumEntries ) );
    nFilePos += sizeof( nNumEntries );

    if ( sizeof( nNumEntries ) == nBytesWritten )
    {
      for ( psEntry = psParent->fe_u.psFirstChild ;
	    NULL != psEntry ;
	    psEntry = psEntry->fe_psNext )
      {
	FileNodeInfo_s	sInodeInfo;

	strcpy( sInodeInfo.zShortName, psEntry->fe_zShortName );
	strcpy( sInodeInfo.zLongName, psEntry->fe_zLongName );


	if ( DOS_S_ISDIR( psEntry->fe_nMode ) ) {
	  sInodeInfo.nSize  = 0;
	} else {
	  sInodeInfo.nSize  = psEntry->fe_u.nSize;
	}

	sInodeInfo.nMode  = psEntry->fe_nMode;
	sInodeInfo.nATime = psEntry->fe_nATime;
	sInodeInfo.nMTime = psEntry->fe_nMTime;
	sInodeInfo.nCTime = psEntry->fe_nCTime;

	lfd_CachedWrite( psTabEntry, &sInodeInfo, nFilePos, sizeof( sInodeInfo ) );
	nFilePos += sizeof( sInodeInfo );
      }
    }
    else
    {
      printk( "lfd_write_directory_info() was not able to write entry count to file!\n" );
    }
  }
  else
  {
    printk( "ERROR : lfd_write_directory_info() could not open info file!\n" );
    nError = -ENOENT;
  }
  return( nError );
}
#endif ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_ReadDirectoryInfo( FileEntry_s* psParent )
{
    DOS_DIR*		pDir;
    struct kernel_dirent*	psDirEnt;
    char			zPath[ 300 ];
    int			nLen = lfd_GetFullPath( psParent, zPath );
    int			nError = -1;
    int			nFileHandle;
  
    strcat( zPath, "\\" );
    nLen++;
    strcpy( zPath + nLen, LFD_INODE_FILE_NAME );

    nFileHandle = dos_open( zPath, O_RDONLY );

    zPath[ nLen ] = 0;

    if ( -1 != nFileHandle ) {
	nError = lfd_load_directory_info( psParent, zPath, nFileHandle );
	dos_close( nFileHandle );
    } else {
	strcpy( zPath + nLen, LFD_OLD_INODE_FILE_NAME );
	nFileHandle = dos_open( zPath, O_RDONLY );
	zPath[ nLen ] = 0;
	if ( -1 != nFileHandle ) {
	    nError = lfd_load_old_directory_info( psParent, zPath, nFileHandle );
	    dos_close( nFileHandle );
	}
    }

    if ( nError == 0 )  {
	psParent->fe_nFlags |= FE_FILE_INFO_LOADED;
	return( 0 );
    }
    nError = 0;

//    printk( "Warning: Directory '%s' have no inode table\n", zPath );
    
    pDir = dos_opendir( zPath );

    if ( pDir == NULL )  {
	printk( "WARNING : lfd_ReadDirectoryInfo() Could not open '%s'\n", zPath );
	return( -ENOENT );
    }
    
    while( (psDirEnt = dos_readdir( pDir )) )
    {
	FileEntry_s* psEntry;
	struct stat	 sStat;
	char	 zFilePath[ 300 ];
	int		 i;
	    
	if ( '.' == psDirEnt->d_name[0] &&
	     (psDirEnt->d_namlen == 1 || (psDirEnt->d_namlen == 2 && '.' == psDirEnt->d_name[1]) ) ) {
	    continue;
	}

	strcpy( zFilePath, zPath );
	strcat( zFilePath, psDirEnt->d_name );
	
	if ( dos_stat( zFilePath, &sStat ) != 0 ) {
	    nError = -ENOENT;
	    printk( "WARNING : lfd_ReadDirectoryInfo() Could not stat '%s'\n", zFilePath );
	    continue;
	}
		
	psEntry = kmalloc( sizeof( FileEntry_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psEntry == NULL ) {
	    nError = -ENOMEM;
	    printk( "ERROR : lfd_ReadDirectoryInfo() Could not allocate memory for new entry\n" );
	    break;
	}

	for ( i = 0 ; i <= psDirEnt->d_namlen ; ++i ) {
	    psEntry->fe_zShortName[ i ] = psDirEnt->d_name[ i ];
	    psEntry->fe_zLongName[ i ] = tolower( psDirEnt->d_name[ i ] );
	}

	if ( DOS_S_ISDIR( sStat.st_mode ) ) {
	    psEntry->fe_u.psFirstChild = NULL;
//	    printk( "Add DIR:  '%s'\n", psEntry->fe_zShortName );
	} else {
	    psEntry->fe_u.nSize = sStat.st_size;
//	    printk( "Add file: '%s'\n", psEntry->fe_zShortName );
	}

	psEntry->fe_nFlags |= FE_FILE_SIZE_VALID | FE_FILE_EXISTS;
	psEntry->fe_nMode	= sStat.st_mode | S_IRWXU | S_IRWXG;
	psEntry->fe_nATime	= sStat.st_atime;
	psEntry->fe_nMTime	= sStat.st_atime;
	psEntry->fe_nCTime	= sStat.st_atime;

	psEntry->fe_psNext	= psParent->fe_u.psFirstChild;
	psParent->fe_u.psFirstChild	= psEntry;
	psEntry->fe_psParent	= psParent;
    }
    dos_closedir( pDir );

    if ( 0 == nError ) {
	psParent->fe_nFlags |= FE_FILE_INFO_LOADED;
    }
    return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static int lfd_ValidateFileAttribs( FileEntry_s* psEntry )
{
  int	nError = 0;

  if ( DOS_S_ISDIR( psEntry->fe_nMode ) == 0 &&
       (psEntry->fe_nFlags & FE_FILE_SIZE_VALID) == 0 /*&& (psEntry->fe_nFlags & FE_FILE_EXISTS)*/ )
  {
    struct	stat	sStat;
    char					zFilePath[ 300 ];

    lfd_GetFullPath( psEntry, zFilePath );

    if ( dos_stat( zFilePath, &sStat ) == 0 )
    {
      if ( DOS_S_ISDIR( sStat.st_mode ) ) {
	printk( "WARNING : Inode file out of sync! entry %s marked as file turn into a dir!\n",
		  zFilePath );
	nError = -EISDIR;
      } else {
	psEntry->fe_u.nSize = sStat.st_size;
	psEntry->fe_nFlags |= FE_FILE_SIZE_VALID;
      }
    }
    else
    {
/*      
      if ( 0 == psEntry->fe_u.nSize )
      {
	psEntry->fe_nFlags |= FE_FILE_SIZE_VALID;
	psEntry->fe_nFlags &= ~FE_FILE_EXISTS;
	nError = 0;
      }
      else
      {
      */      
	printk( "WARNING : lfd_ValidateFileAttribs() could not stat() file %s\n", psEntry->fe_zLongName );
	nError = -ENOENT;
//      }
    }
  }
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_LookupFile( FileEntry_s* psParent,
		    const char* pzName, int nNameLen, FileEntry_s** ppsRes )
{
  int		nError = 0;
  FileEntry_s*	psEntry = NULL;

  if ( NULL == psParent ) {
    printk( "ERROR : lfd_LookupFile called with psParent == NULL\n" );
    return( -EINVAL );
  }

  if ( nNameLen >= LFD_MAX_NAME_LEN ) {
    printk( "ERROR : lfd_LookupFile( %s ) name to long!\n", pzName );
    return( -ENAMETOOLONG );
  }

  if ( DOS_S_ISDIR( psParent->fe_nMode ) )
  {
    if ( (psParent->fe_nFlags & FE_FILE_INFO_LOADED) == 0 ) {
      nError = lfd_ReadDirectoryInfo( psParent );
    }

    if ( 0 == nError )
    {
      for ( psEntry = psParent->fe_u.psFirstChild ;
	    NULL != psEntry ;
	    psEntry = psEntry->fe_psNext )
      {
	if ( strlen( psEntry->fe_zLongName ) == nNameLen &&
	     strnicmp( psEntry->fe_zLongName, pzName, nNameLen ) == 0 )
	{
	  break;
	}
	if ( strlen( psEntry->fe_zShortName ) == nNameLen &&
	     strnicmp( psEntry->fe_zShortName, pzName, nNameLen ) == 0 )
	{
	  break;
	}
      }
      if ( NULL == psEntry ) {
/*					printk( "ERROR : lfd_LookupFile() could not find '%s' in '%s'\n",
					pzName, psParent->fe_zLongName ); */
	nError = -ENOENT;
      } else {
	nError = lfd_ValidateFileAttribs( psEntry );

	if ( 0 != nError ) {
	  psEntry = NULL;
	}
      }
    }
    else
    {
      printk( "Error: lfd_LookupFile() could not read directory '%s'\n",
		psParent->fe_zLongName );
    }
  }
  else
  {
    printk( "Error: lfd_LookupFile() parent '%s' (%p) not directory\n", psParent->fe_zShortName, psParent );
    nError = -ENOTDIR;
  }

  if ( NULL != ppsRes ) {
    *ppsRes = psEntry;
  }

  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static void lfd_release_file( FileEntry_s* psEntry )
{
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_create_file_entry( FileEntry_s* psParent, const char* pzName, int nNameLen,
			   int nMode, FileEntry_s** ppsRes )
{
  int		nError;
  FileEntry_s*	psEntry;

  if ( NULL != ppsRes ) {
    *ppsRes = NULL;
  }

  if ( pzName[0] == '.' && ( nNameLen == 1 || (nNameLen == 2 && pzName[1] == '.') ) ) {
    return( -EEXIST );
  }
  
  nError = lfd_LookupFile( psParent, pzName, nNameLen, &psEntry );

  if ( 0 == nError ) {
    lfd_release_file( psEntry );
    return( -EEXIST );
  }

  if ( -ENOENT == nError )
  {
    nError = 0;

      /* Directory should have been read by lfd_LookupFile(), Just verify that */
    if ( (psParent->fe_nFlags & FE_FILE_INFO_LOADED) != 0 )
    {
      psEntry = kmalloc( sizeof( FileEntry_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

      if ( NULL != psEntry )
      {
	strncpy( psEntry->fe_zLongName, pzName, nNameLen );
	psEntry->fe_zLongName[ nNameLen ] = '\0';

	nError = lfd_CreateShortName( psParent, pzName, nNameLen, psEntry->fe_zShortName );

	if ( 0 == nError )
	{
	  char	zFullPath[ 300 ];

	  lfd_GetFullPath( psParent, zFullPath );

	  strcat( zFullPath, "\\" );
	  strcat( zFullPath, psEntry->fe_zShortName );

	  if ( DOS_S_ISDIR( nMode ) ) {
	    psEntry->fe_nFlags |= FE_FILE_EXISTS;

	    nError = (dos_mkdir( zFullPath, nMode ) != -1 ) ? 0 : -ENOSPC;
	  } else {
	    nError = dos_create( zFullPath );
	    if ( nError >= 0 ) {
	      dos_close( nError );
	      psEntry->fe_nFlags |= FE_FILE_EXISTS;
	      nError = 0;
	    }
	  }

	  if ( 0 == nError )
	  {
	    psEntry->fe_nMode	= nMode;

	    psEntry->fe_u.nSize	= 0;

	    psEntry->fe_nFlags |= FE_FILE_SIZE_VALID;
	    psEntry->fe_nUID	= getfsuid();
	    psEntry->fe_nGID	= getfsgid();
	    psEntry->fe_nATime	= sys_GetTime( NULL );
	    psEntry->fe_nMTime	= psEntry->fe_nATime;
	    psEntry->fe_nCTime	= psEntry->fe_nATime;

	    psEntry->fe_psNext	= psParent->fe_u.psFirstChild;
	    psParent->fe_u.psFirstChild	= psEntry;
	    psEntry->fe_psParent	= psParent;

	    if ( NULL != ppsRes ) {
	      *ppsRes = psEntry;
	    }
	    lfd_write_directory_info( psParent );
	  }
	  else
	  {
	    printk( "ERROR : Unable to create physical file '%s'\n", zFullPath );
	  }
	}
	else
	{
	  printk( "ERROR : Unable to convert '%s' to short name!\n", pzName );
	}
      }
      else
      {
	nError = -ENOMEM;
      }
    }
    else
    {
      nError = -ENOMEM;	/* Dont know exactly what to put here	*/
    }
  }
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static void lfd_unlink_file_entry( FileEntry_s* psNode )
{
  FileEntry_s**			psTmp;

  for( psTmp = &psNode->fe_psParent->fe_u.psFirstChild ;
       NULL != *psTmp ;
       psTmp = &((*psTmp)->fe_psNext) )
  {
    if ( *psTmp == psNode ) {
      *psTmp = psNode->fe_psNext;
      psNode->fe_psParent = NULL;
      break;
    }
  }
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_rename_entry( FileEntry_s* psOldEntry, FileEntry_s* psDestDir, const char* pzNewName, int nNewNameLen )
{
  int		nError;
  int		nNewPathLen;
  char		zOldPath[ 300 ];
  char		zNewPath[ 300 ];
  FileEntry_s*	psOldDir = psOldEntry->fe_psParent;

    // DOS cant move directories :( We should may try to emulate this by creating a new
    // directory, and move all the files? Some other day... maybe.
  
  if ( psOldDir != psDestDir && DOS_S_ISDIR( psOldEntry->fe_nMode ) ) {
    printk( "Error : lfd_rename_entry() attempt to move directory\n" );
    return( -EACCES );
  }
  
  nNewPathLen = lfd_GetFullPath( psDestDir, zNewPath );

  if ( -1 == nNewPathLen ) {
    printk( "Error: lfd_rename_entry() failed to create destination path\n" );
    return( -EIO );
  }
  zNewPath[ nNewPathLen ] = '\\';
  nNewPathLen++;

  nError = lfd_CreateShortName( psDestDir, pzNewName, nNewNameLen, &zNewPath[ nNewPathLen ] );

  if ( nError < 0 ) {
    printk( "Error: lfd_rename_entry() Failed to create short name for %s\n", pzNewName );
    return( nError );
  }
  
//  if ( psOldEntry->fe_nFlags & FE_FILE_EXISTS ) {
    lfd_GetFullPath( psOldEntry, zOldPath );
    lfd_close_dos_handle( psOldEntry );
    nError = dos_rename( zOldPath, zNewPath );
//  }

  if ( nError < 0 ) {
    printk( "lfd_rename_entry() failed to rename physical file from %s to %s\n", zOldPath, zNewPath );
    return( -EIO );
  }
  
  lfd_unlink_file_entry( psOldEntry );

  strcpy( psOldEntry->fe_zShortName, &zNewPath[ nNewPathLen ] );
  memcpy( psOldEntry->fe_zLongName, pzNewName, nNewNameLen );
  psOldEntry->fe_zLongName[ nNewNameLen ] = '\0';

  psOldEntry->fe_psNext		= psDestDir->fe_u.psFirstChild;
  psDestDir->fe_u.psFirstChild	= psOldEntry;
  psOldEntry->fe_psParent	= psDestDir;

  lfd_write_directory_info( psDestDir );

  if ( psOldDir != psDestDir ) {
    lfd_write_directory_info( psOldDir );
  }
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

static int lfd_DeleteDosDir( FileEntry_s* psEntry, const char* pzPath )
{
  int	nError = 0;

  if ( (psEntry->fe_nFlags & FE_FILE_INFO_LOADED) == 0 ) {
    nError = lfd_ReadDirectoryInfo( psEntry );
  }
  if ( nError < 0 ) {
    printk( "lfd_DeleteDosDir() failed to load inode table\n" );
    return( nError );
  }
  if ( NULL == psEntry->fe_u.psFirstChild )
  {
    nError =  dos_rmdir( pzPath );

    if ( nError < 0 ) {
      printk( "lfd_DeleteDosDir:1() dos_rmdir(%s) failed! Err = %d\n", pzPath, nError );
    }
  }
  else
  {
    if ( psEntry->fe_u.psFirstChild->fe_psNext != NULL ||
	 stricmp( psEntry->fe_u.psFirstChild->fe_zShortName, LFD_INODE_FILE_NAME ) != 0 ) {
      printk( "lfd_DeleteDosDir() directory %s not empty\n", psEntry->fe_zLongName );
      return( -ENOTEMPTY );
    }
    nError = lfd_DeleteFileEntry( psEntry->fe_u.psFirstChild ); /* Remove inode table */

    if ( nError < 0 ) {
      printk( "lfd_DeleteDosDir() failed to delete inode table\n" );
      return( nError );
    }
    nError =  dos_rmdir( pzPath );
    if ( nError < 0 ) {
      printk( "lfd_DeleteDosDir:2() dos_rmdir(%s) failed! Err = %d\n", pzPath, nError );
    }
  }
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_DeleteFileEntry( FileEntry_s* psEntry )
{
  int		nError = 0;
  char*	pzPath;

  FileEntry_s*	psParent = psEntry->fe_psParent;

  if ( psParent == NULL ) {
    return( -EINVAL );
  }
    
  pzPath = kmalloc( LFD_MAX_DOS_PATH, MEMF_KERNEL );

  if ( pzPath == NULL ) {
    printk( "Error: lfd_DeleteFileEntry() no memory for path buffer\n" );
    return( -ENOMEM );
  }
  
  lfd_GetFullPath( psEntry, pzPath );

  if ( DOS_S_ISDIR( psEntry->fe_nMode ) ) {
    nError = lfd_DeleteDosDir( psEntry, pzPath );
  } else {
    lfd_flush_node_buffers( psEntry, TRUE, TRUE );

//    if ( psEntry->fe_nFlags & FE_FILE_EXISTS )
    {
      lfd_close_dos_handle( psEntry );
      nError = dos_unlink( pzPath );

      if ( 0 != nError ) {
	printk( "WARNING : Could not delete underlying file '%s'\n", pzPath );
	nError = 0;
      }
    }
  }

  if ( nError >= 0 ) {
    bool bSyncDir = stricmp( psEntry->fe_zShortName, LFD_INODE_FILE_NAME ) != 0;

    lfd_unlink_file_entry( psEntry );
    kfree( psEntry );

    if ( bSyncDir ) {
      lfd_write_directory_info( psParent );
    }
  }
  else
  {
    nError = -ENOTEMPTY;
  }
  kfree( pzPath );
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_TruncFile( FileEntry_s* psEntry )
{
  int		nError = 0;
  char*	pzPath;

  FileEntry_s*	psParent = psEntry->fe_psParent;

  kassertw( NULL != psEntry );

  if ( NULL == psEntry ) {
    return( -EINVAL );
  }

  if ( NULL != psParent )
  {
    pzPath = kmalloc( LFD_MAX_DOS_PATH, MEMF_KERNEL );

    if ( NULL != pzPath )
    {
      lfd_GetFullPath( psEntry, pzPath );

      if ( DOS_S_ISDIR( psEntry->fe_nMode ) )
      {
	nError = -EISDIR;
      }
      else
      {
	lfd_flush_node_buffers( psEntry, TRUE, TRUE );

	if ( /*(psEntry->fe_nFlags & FE_FILE_EXISTS) != 0 &&*/
	     ((psEntry->fe_nFlags & FE_FILE_SIZE_VALID) == 0 || 0 != psEntry->fe_u.nSize ) )
	{
	  int nFile = lfd_get_entry_file_handle( psEntry );

	  if ( -1 != nFile )
	  {
	    dos_lseek( nFile, 0, SEEK_SET );
	    dos_write( nFile, "", 0 );
	  }
	  else
	  {
	    nError = -EIO;
	  }
	}

	psEntry->fe_u.nSize	= 0;
	psEntry->fe_nFlags |= FE_FILE_SIZE_VALID;

	psEntry->fe_nATime = sys_GetTime( NULL );
	psEntry->fe_nMTime = psEntry->fe_nATime;
	psEntry->fe_nCTime = psEntry->fe_nATime;
      }
      kfree( pzPath );
    }
    else
    {
      nError = -ENOMEM;
    }
  }
  else
  {
    nError = -EINVAL;
  }
  return( nError );
}

/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_CachedRead( FileEntry_s* psEntry, void* pBuffer, off_t nPos, size_t nLen )
{
  int		 nRead;
  int		 nTotalRead = 0;
  CacheBuffer_s* psBuffer;


  kassertw( NULL != pBuffer );

  if ( (psEntry->fe_nFlags & FE_FILE_SIZE_VALID ) == 0 ) {
    printk( "WARNING : Attempt to read from file with unsynced file size!\n" );
  }

  while( nLen > 0 && nPos < psEntry->fe_u.nSize )
  {
    int	nFilePos		=	nPos;
    int	nBlkOffset 	= nFilePos & ~(CACHE_BLOCK_SIZE - 1);

    psBuffer = lfd_find_buffer( psEntry, nBlkOffset );

    if ( NULL == psBuffer )
    {
      psBuffer = lfd_AllocBuffer( psEntry );

      if ( NULL != psBuffer )
      {
	psBuffer->cb_psFileEntry = psEntry;
	psBuffer->cb_nOffset 	 = nBlkOffset;
	psBuffer->cb_nBytesUsed  = min( CACHE_BLOCK_SIZE,
					psEntry->fe_u.nSize - psBuffer->cb_nOffset );

	nRead = lfd_read_buffer( psBuffer );
      }
      else
      {
	printk( "ERROR : Unable to allocate cache buffer!!\n" );
	nRead = -1;
      }
    }
    else
    {
      lfd_TouchBuffer( psBuffer );
      nRead = 0;
    }

    kassertw( NULL != psBuffer );

    if ( nRead != -1 )
    {
      nRead = min( nLen, psEntry->fe_u.nSize - nFilePos );
      nRead = min( nRead, CACHE_BLOCK_SIZE - (nFilePos - psBuffer->cb_nOffset) );

      memcpy( pBuffer, &psBuffer->cb_aData[ nFilePos - psBuffer->cb_nOffset ], nRead );

      pBuffer	  = ((int8*) pBuffer) + nRead;
      nPos	 += nRead;
      nLen	 -= nRead;
      nTotalRead += nRead;
    }
    else
    {
      nTotalRead = -1;
      break;
    }
  }
  return( nTotalRead );
}


/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

int lfd_CachedWrite(  FileEntry_s* psEntry, const void* pBuffer, off_t nPos, size_t nLen  )
{
  int		 nRead;
  int		 nError;
  int		 nTotalRead = 0;
  int		 nOrigSize  = psEntry->fe_u.nSize;

  CacheBuffer_s* psBuffer;

  kassertw( NULL != pBuffer );

  if ( nPos + nLen > psEntry->fe_u.nSize ) {
    psEntry->fe_u.nSize	= nPos + nLen;
  }
  if ( psEntry->fe_nFlags & FE_FILE_SIZE_VALID )
  {
    psEntry->fe_nMTime = sys_GetTime( NULL );
    psEntry->fe_nATime = psEntry->fe_nMTime;
    psEntry->fe_nCTime = psEntry->fe_nMTime;		/* FIX ME should NOT be set here!	*/

    while( nLen > 0 && nPos < psEntry->fe_u.nSize )
    {
      int nFilePos   = nPos;
      int nBlkOffset = nFilePos & ~(CACHE_BLOCK_SIZE - 1);

      nRead = min( nLen, CACHE_BLOCK_SIZE - (nFilePos - nBlkOffset) );

      psBuffer = lfd_find_buffer( psEntry, nBlkOffset );


      if ( NULL == psBuffer )
      {
	psBuffer = lfd_AllocBuffer( psEntry );

	psBuffer->cb_psFileEntry = psEntry;
	psBuffer->cb_nOffset 	 = nBlkOffset;
	psBuffer->cb_nBytesUsed  = min( CACHE_BLOCK_SIZE, nOrigSize - psBuffer->cb_nOffset );

	  /* If we dont overwrite the entire buffer,
	   *	we must first fill it up from the file
	   */

	if ( nRead < CACHE_BLOCK_SIZE && psBuffer->cb_nBytesUsed > 0 ) {
	    /*				nError = lfd_read_buffer( psBuffer );	*/
	  nError = 0;
	  lfd_read_buffer( psBuffer );
	} else {
	  nError = 0;
	}
	psBuffer->cb_nBytesUsed = min( CACHE_BLOCK_SIZE,
				       psEntry->fe_u.nSize - psBuffer->cb_nOffset );
      }
      else
      {
	psBuffer->cb_nBytesUsed = min( CACHE_BLOCK_SIZE,
				       psEntry->fe_u.nSize - psBuffer->cb_nOffset );
	lfd_TouchBuffer( psBuffer );
	nError = 0;
      }

      psBuffer->cb_bDirty = true;

      if ( nError >= 0 )
      {
	memcpy( &psBuffer->cb_aData[ nFilePos - psBuffer->cb_nOffset ], pBuffer, nRead );

	pBuffer	    = ((int8*) pBuffer) + nRead;
	nPos	   += nRead;
	nLen	   -= nRead;
	nTotalRead += nRead;
      }
      else
      {
	printk( "ERROR : CachedWrite() could not fill new cache buffer!\n" );
	nTotalRead = -1;
	break;
      }
    }
  }
  else
  {
    printk( "ERROR : Attempt to write to file with unsynked file size!!\n" );
    nTotalRead = -1;
  }

  return( nTotalRead );
}


/******  ****************************************************
 *
 *	NAME:
 *
 *	SYNOPSIS:
 *
 *	FUNCTION:
 *
 *	INPUTS:
 *
 *	RESULTS:
 *
 *	SEE ALSO:
 *
 ****************************************************************************/

void lfd_InitCache( int nCacheSize )
{
  CacheBuffer_s*		pasBuffers;
  int								i;

// 	pasBuffers = kmalloc( sizeof( CacheBuffer_s ) * NUM_BUFFERS, MEMF_KERNEL | MEMF_CLEAR );

  
//  pasBuffers = (CacheBuffer_s*) get_free_pages( PAGE_ALIGN( sizeof( CacheBuffer_s ) * NUM_BUFFERS ) / PAGE_SIZE, MEMF_CLEAR );
  pasBuffers = (CacheBuffer_s*) get_free_pages( (nCacheSize + PAGE_SIZE - 1) / PAGE_SIZE, MEMF_CLEAR );

  for ( i = 0 ; i < LFD_NUM_DOS_HANDLES ; ++i )
  {
    g_apsOpenedEntries[ i ].psEntry 	= NULL;
    g_apsOpenedEntries[ i ].nFileDesc = -1;
  }


  for ( i = 0 ; i < nCacheSize / sizeof(CacheBuffer_s) ; ++i )	{
    pasBuffers[ i ].cb_nMagic1 = 0x12345678;
    pasBuffers[ i ].cb_nMagic2 = 0x87654321;
    lfd_append_buffer( &pasBuffers[ i ] );
  }
}
