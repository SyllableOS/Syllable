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

#include <posix/fcntl.h>
#include <posix/dirent.h>
#include <posix/errno.h>
#include <posix/stat.h>

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>

#include "lfndos.h"
#include "dos.h"

#include <macros.h>


static	FileNode_s* g_psFirstNode = NULL;
static	sem_id	    g_hFileSysLock = -1;


typedef struct
{
  int	rd_nPos;
} ReadDirCookie_s;

typedef struct
{
  kdev_t	lv_nDevNum;
  sem_id	lv_hFileSysLock;

  int		lv_nOpenFiles;
  FileNode_s*	lv_psFirstNode;
  FileNode_s*	lv_psRootNode;

    /*	File entry for directory where deleted files are stored until last reference is gone	*/
  FileEntry_s*	lv_psDelDirEntry;
	
} LfdVolume_s;



/** Mount a LFD file system
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

static int lfd_mount( kdev_t nDevNum, const char* pzDevPath, uint32 nFlags, void* pArgs, int nArgLen,
		      void** ppVolData, ino_t* pnRootIno )
{
  LfdVolume_s*	psVolume;

  psVolume = kmalloc( sizeof( LfdVolume_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

  if ( NULL != psVolume )
  {
    FileNode_s*	 psRootNode;
    FileEntry_s* psEntry;
    char	 zDelmePath[64];
    
    psVolume->lv_nDevNum = nDevNum;
	
    printk( "Mount lfd\n" );

    psEntry = kmalloc( sizeof( FileEntry_s ), MEMF_KERNEL | MEMF_LOCKED | MEMF_CLEAR );
    psEntry->fe_nFlags = FE_FILE_EXISTS;

    psVolume->lv_psDelDirEntry = kmalloc( sizeof( FileEntry_s ),
					  MEMF_KERNEL | MEMF_LOCKED | MEMF_CLEAR );

    psVolume->lv_psDelDirEntry->fe_nFlags   = FE_FILE_EXISTS;
    psVolume->lv_psDelDirEntry->fe_psParent = psEntry;	/* Child of root	*/
    psEntry->fe_u.psFirstChild		    = psVolume->lv_psDelDirEntry;

    strcpy( zDelmePath, pzDevPath );
    strcat( zDelmePath, "/_ATHDEL_" );
    dos_mkdir( zDelmePath, 0 );
    
    strcpy( psVolume->lv_psDelDirEntry->fe_zShortName, "_ATHDEL_" );
    strcpy( psVolume->lv_psDelDirEntry->fe_zLongName, "_athdel_" );
    psVolume->lv_psDelDirEntry->fe_nMode = DOS_S_IFDIR | S_IRWXU | S_IRWXO | S_IRWXG;

    if ( (psRootNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR )) )
    {
      psRootNode->fn_psFileEntry = psEntry;
      psRootNode->fn_nLinkCount  = 1;

      strcpy( psEntry->fe_zShortName, pzDevPath );
      strcpy( psEntry->fe_zLongName, "" );

      psEntry->fe_nMode = DOS_S_IFDIR | S_IRWXUGO;

      psRootNode->fn_nRefCount = 1;

			
	/*	The address should hopfully be unique so we use it as inode number	*/

      psRootNode->fn_nInoNum	= (int) psEntry;
      psRootNode->fn_psNext	= NULL;
      psVolume->lv_psFirstNode	= psRootNode;
      psVolume->lv_psRootNode	= psRootNode;
      g_psFirstNode		= psRootNode;


      *pnRootIno = psRootNode->fn_nInoNum;
			
      printk( "LFD file system mounted (%p)\n", psEntry );
    }
    *ppVolData = psVolume;
  }
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

static int lfd_UnMount( void* pVolData )
{
  return( 0 );
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

int lfd_Lock( void )
{
  return( lock_semaphore( g_hFileSysLock, SEM_NOSIG, INFINITE_TIMEOUT ) );
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

void	lfd_Unlock( void )
{
  unlock_semaphore( g_hFileSysLock );
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

static int lfd_rstat( void* pVolume, void* pNode, struct stat* psStat )
{
  LfdVolume_s*	psVolume = pVolume;
  FileNode_s* 	psNode = pNode;

  psStat->st_dev	= psVolume->lv_nDevNum;
  psStat->st_ino 	= psNode->fn_nInoNum;
  psStat->st_size 	= (DOS_S_ISDIR( psNode->fn_psFileEntry->fe_nMode )) ? 0 : psNode->fn_psFileEntry->fe_u.nSize;

  if ( DOS_S_ISREG( psNode->fn_psFileEntry->fe_nMode ) ) {
    psStat->st_mode = S_IFREG | (psNode->fn_psFileEntry->fe_nMode & S_IRWXUGO);
  } else if ( DOS_S_ISLNK( psNode->fn_psFileEntry->fe_nMode ) ) {
    psStat->st_mode = S_IFLNK | (psNode->fn_psFileEntry->fe_nMode & S_IRWXUGO);
  } else if ( DOS_S_ISDIR( psNode->fn_psFileEntry->fe_nMode ) ) {
    psStat->st_mode = S_IFDIR | (psNode->fn_psFileEntry->fe_nMode & S_IRWXUGO);
  } else {
    printk( "lfd_rstat() invalid file mode %08x\n", psNode->fn_psFileEntry->fe_nMode );
  }
  if ( psNode->fn_psFileEntry->fe_nMode & DOS_S_ISUID ) {
    psStat->st_mode |= S_ISUID;
  }
  if ( psNode->fn_psFileEntry->fe_nMode & DOS_S_ISGID ) {
    psStat->st_mode |= S_ISGID;
  }
  
//  pNode = 0xdeadbabe;
  psStat->st_atime   = psNode->fn_psFileEntry->fe_nATime;
  psStat->st_mtime   = psNode->fn_psFileEntry->fe_nMTime;
  psStat->st_ctime   = psNode->fn_psFileEntry->fe_nCTime;
  psStat->st_nlink   = psNode->fn_nLinkCount;
  psStat->st_blksize = CACHE_BLOCK_SIZE;
	
  psStat->st_uid     = psNode->fn_psFileEntry->fe_nUID;
  psStat->st_gid     = psNode->fn_psFileEntry->fe_nGID;
	
  return( 0 );
}

static int lfd_wstat( void* pVolume, void* pNode, const struct stat* psStat, uint32 nMask )
{
  FileNode_s* 	psNode = pNode;

  if ( (nMask & WSTAT_ATIME) || (nMask & WSTAT_MTIME) ) {
    psNode->fn_psFileEntry->fe_nATime = psStat->st_mtime;
    psNode->fn_psFileEntry->fe_nMTime = psStat->st_mtime;
  }
  if ( nMask & WSTAT_MODE ) {
    psNode->fn_psFileEntry->fe_nMode = (psNode->fn_psFileEntry->fe_nMode & ~S_IRWXUGO) | (psStat->st_mode & S_IRWXUGO);
    if ( psStat->st_mode & S_ISUID ) psNode->fn_psFileEntry->fe_nMode |= DOS_S_ISUID;
    if ( psStat->st_mode & S_ISGID ) psNode->fn_psFileEntry->fe_nMode |= DOS_S_ISGID;
  }
	
  
  if ( nMask & WSTAT_UID ) {
    psNode->fn_psFileEntry->fe_nUID = psStat->st_uid;
  }
  if ( nMask & WSTAT_GID ) {
    psNode->fn_psFileEntry->fe_nGID = psStat->st_gid;
  }

  lfd_write_directory_info( psNode->fn_psFileEntry->fe_psParent );
	
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

static int lfd_create_node( LfdVolume_s* psVolume, FileEntry_s* psEntry, FileNode_s** ppsResNode )
{
  FileNode_s*	psNode;

  *ppsResNode = NULL;

  if ( (psNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR )) )
  {
    psNode->fn_nLinkCount  = 1;
    psNode->fn_psFileEntry = psEntry;

      /*	The address should hopfully be unique so we use it as inode number	*/

    psNode->fn_nInoNum	 = (int) psEntry;
    psNode->fn_nRefCount = 1;

    psNode->fn_psNext	= g_psFirstNode;
    g_psFirstNode	= psNode;

    *ppsResNode	=	psNode;

    return( 0 );
  }
  return( -ENOMEM );
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

static void lfd_delete_node( LfdVolume_s* psVolume, FileNode_s* psNode )
{
  FileNode_s**			psTmp;

  for( psTmp = &g_psFirstNode ; NULL != *psTmp ; psTmp = &((*psTmp)->fn_psNext) )
  {
    if ( *psTmp == psNode )	{
      *psTmp = psNode->fn_psNext;
      break;
    }
  }
  kfree( psNode );
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

static int lfd_lookup( void* pVolData, void* pParent, const char* pzName, int nNameLen, ino_t* pnResInode )
{
  int nError = 0;

  FileNode_s*	psParentNode = pParent;
  FileEntry_s*	psEntry;

  
	
/*	printk( "Lookup( '%s' )\n", pzName ); */

  if ( NULL == pParent ) {
    return( -ENOENT );
  }

  if ( nNameLen == 1 && pzName[0] == '.' )
  {
    *pnResInode = psParentNode->fn_nInoNum;
  }
  else
  {
    if ( nNameLen == 2 && pzName[0] == '.' && pzName[1] == '.' )
    {
      psEntry	=	psParentNode->fn_psFileEntry;

      kassertw( NULL != psEntry );

      if ( NULL != psEntry->fe_psParent ) {
	psEntry	=	psEntry->fe_psParent;
      }
      *pnResInode = (int) psEntry;
    }
    else
    {
      FileEntry_s* psParentEntry = psParentNode->fn_psFileEntry;

      nError = lfd_Lock();

      if ( 0 == nError ) {
	nError = lfd_LookupFile( psParentEntry, pzName, nNameLen, &psEntry );
	lfd_Unlock();
      }
      if ( 0 == nError ) {
	if ( stricmp( psEntry->fe_zShortName, LFD_INODE_FILE_NAME ) == 0 ) {
	  printk( "lfd_lookup() attempt to lookup inode table\n" );
	  nError = -ENOENT;
	} else {
	  *pnResInode = (int) psEntry;
	}
      }
    }
  }
  return( nError );
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

static int lfd_create( void* pVloume, void* pParent, const char* pzName, int nNameLen,
		       int nFlags, int nMode, ino_t* pnResInode, void** ppCookie )
{
  FileNode_s*	psParentNode = pParent;
  FileEntry_s*	psEntry;
  int		nError = 0;

  *ppCookie = NULL;
	
  if ( NULL == pParent ) {
    return( -ENOENT );
  }
  nError = lfd_Lock();

  if ( 0 == nError ) {
    uint32 nDOSMode = DOS_S_IFREG | (nMode & S_IRWXUGO);
    if ( nMode & S_ISUID ) nDOSMode |= DOS_S_ISUID;
    if ( nMode & S_ISGID ) nDOSMode |= DOS_S_ISGID;
    
    nError = lfd_create_file_entry( psParentNode->fn_psFileEntry, pzName, nNameLen, nDOSMode, &psEntry );
    lfd_Unlock();
  }

  if ( nError == 0 ) {
    LfdCooki_s* psCookie = kmalloc( sizeof( LfdCooki_s ), MEMF_KERNEL | MEMF_CLEAR );

    if ( NULL != psCookie ) {
      psCookie->nFlags = nFlags;
			
      *pnResInode = (int) psEntry;
      *ppCookie = psCookie;
    } else {
      nError = -ENOMEM;
    }
  }
  return( nError );
}

static int lfd_symlink( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, const char* pzNewPath )
{
  FileNode_s* psParentNode = pParentNode;
  FileEntry_s*	psEntry;
  int		nError = 0;

  if ( NULL == pParentNode ) {
    return( -ENOENT );
  }
  nError = lfd_Lock();

  if ( 0 != nError ) {
    return( nError );
  }
  nError = lfd_create_file_entry( psParentNode->fn_psFileEntry, pzName, nNameLen, DOS_S_IFLNK | S_IRWXUGO, &psEntry );

  if ( nError < 0 ) {
    goto out;
  }

  nError = lfd_CachedWrite( psEntry, LFD_SYMLINK_MARKER, 0, LFD_SYMLINK_MARKER_SIZE );
  nError = lfd_CachedWrite( psEntry, pzNewPath, 16, strlen( pzNewPath ) );
  lfd_write_directory_info( psEntry->fe_psParent );

  if ( nError > 0 ) {
    nError = 0;
  }
out:
  lfd_Unlock();
  return( nError );
}

static int lfd_readlink( void* pVolume, void* pNode, char* pzBuf, size_t nBufSize )
{
  FileNode_s*	psNode	= pNode;
  int		nRead   = 0;

  if ( DOS_S_ISLNK( psNode->fn_psFileEntry->fe_nMode ) == false ) {
    return( -EINVAL );
  }
	
  if ( 0 == nBufSize ) {
    return( 0 );
  }

  nRead = lfd_Lock();

  if ( nRead != 0 ) {
    printk( "WARNING : lfd_readlink() could not obtain file system semaphore!!!!!!!!\n" );
    return( nRead );
  }

  kassertw( NULL != pzBuf );

  nRead = lfd_CachedRead( psNode->fn_psFileEntry, pzBuf, 16, nBufSize );
 
  lfd_Unlock();
  return( nRead );
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

static int lfd_read( void* pVolume, void* pNode, void* pCookie, off_t nPos, void* pBuffer, size_t nLen )
{
  FileNode_s*	psNode	= pNode;
  int		nRead   = 0;

  if ( DOS_S_ISDIR( psNode->fn_psFileEntry->fe_nMode ) ) {
    return( -EISDIR );
  }
	
  if ( 0 == nLen ) {
    return( 0 );
  }

  nRead = lfd_Lock();

  if ( 0 == nRead )
  {
    kassertw( NULL != pBuffer );

    nRead = lfd_CachedRead( psNode->fn_psFileEntry, pBuffer, nPos, nLen );

    lfd_Unlock();
  }
  else
  {
    printk( "WARNING : lfd_read() could not obtain file system semaphore!!!!!!!!\n" );
  }

/*
  printk( "%ld bytes read from %s\n", nRead, psNode->fn_psFileEntry->fe_zLongName );
  */
  return( nRead );
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

static int lfd_write( void* pVolume, void* pNode, void* pCookie, off_t nPos, const void* pBuffer, size_t nLen )
{
  FileNode_s*	psNode	 = pNode;
  LfdCooki_s*	psCookie = pCookie;
  int		nRead    = 0;
  off_t	nRealPos = (psCookie->nFlags & O_APPEND) ? psNode->fn_psFileEntry->fe_u.nSize : nPos;

  if ( 0 == nLen ) {
    return( 0 );
  }
	

  if ( DOS_S_ISREG( psNode->fn_psFileEntry->fe_nMode ) == false ) {
    printk( "ERROR : Attempt to write to directory %s\n", psNode->fn_psFileEntry->fe_zLongName );
    return( -EISDIR );
  }


  nRead = lfd_Lock();

  if ( 0 == nRead )
  {
    kassertw( NULL != pBuffer );

    nRead = lfd_CachedWrite( psNode->fn_psFileEntry, pBuffer, nRealPos, nLen );
    lfd_write_directory_info( psNode->fn_psFileEntry->fe_psParent );

    lfd_Unlock();
  }

  return( nRead );
}

int lfd_rewinddir( void* pVolume, void* pNode, void* pCookie )
{
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

static int lfd_read_dir( void* pVolume, void* pNode, void* pCookie, int nPos, struct kernel_dirent* psFileInfo, size_t nBufSize )
{
  FileNode_s*  psNode	 = pNode;
  FileEntry_s* psDirEnt = psNode->fn_psFileEntry;
  FileEntry_s* psEntry;
  int	       i;
  int	       nError 	 = 0;

  if ( DOS_S_ISDIR( psDirEnt->fe_nMode ) == 0 ) {
    return( -ENOTDIR );
  }

  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_read_dir() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }
  
  if ( (psDirEnt->fe_nFlags & FE_FILE_INFO_LOADED) == 0 ) {
    nError = lfd_ReadDirectoryInfo( psDirEnt );
    if ( nError < 0 ) {
      goto error;
    }
  }

  if ( nPos < 2 )
  {
    strcpy( psFileInfo->d_name, (0 == nPos) ? "." : ".." );
    if ( 0 == nPos ) {
      psFileInfo->d_ino  = psNode->fn_nInoNum;
    } else {
      if ( psNode->fn_psFileEntry->fe_psParent != NULL ) {
	psFileInfo->d_ino  = ((int)psNode->fn_psFileEntry->fe_psParent);
      }
    }
    nError = 1;
  }
  else
  {
    for ( i = 2, psEntry = psDirEnt->fe_u.psFirstChild  ;
	  i < nPos && NULL != psEntry ; ++i, psEntry = psEntry->fe_psNext )
    {
      if ( stricmp( psEntry->fe_zLongName, LFD_INODE_FILE_NAME ) == 0 ) {
	--i;
	continue;
      }
      if ( psEntry->fe_psNext != NULL && 
	   stricmp( psEntry->fe_psNext->fe_zLongName, LFD_INODE_FILE_NAME ) == 0 ) {
	psEntry = psEntry->fe_psNext;
      }
    }
    if ( psEntry != NULL && stricmp( psEntry->fe_zLongName, LFD_INODE_FILE_NAME ) == 0 ) {
      psEntry = psEntry->fe_psNext;
    }
    if ( psEntry != NULL )
    {
      strcpy( psFileInfo->d_name, psEntry->fe_zLongName );
      psFileInfo->d_ino  = (int)psEntry;
      nError = 1;
    }
    else
    {
      if ( i == nPos ) {
	nError = 0;
      } else {
	nError = -EINVAL;
	nPos = 0;
      }
    }
  }
  psFileInfo->d_namlen = strlen( psFileInfo->d_name );
error:
  lfd_Unlock();

  return( nError );
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

static int lfd_open( void* pVolData, void* pNode, int nMode, void** ppCookie )
{
  FileNode_s* psNode = pNode;
  LfdCooki_s* psCookie = kmalloc( sizeof( LfdCooki_s ), MEMF_KERNEL | MEMF_CLEAR );
  int	      nError = 0;
	
  if ( NULL != psCookie )
  {
    psCookie->nFlags = nMode;
    *ppCookie = psCookie;

    if ( nMode & O_TRUNC )
    {
      if ( DOS_S_ISDIR( psNode->fn_psFileEntry->fe_nMode ) )
      {
	kfree( psCookie );
	nError = -EISDIR;
      }
      else
      {
	lfd_Lock();
	lfd_TruncFile( psNode->fn_psFileEntry );
	lfd_write_directory_info( psNode->fn_psFileEntry->fe_psParent );
	lfd_Unlock();
      }
    }
  }
  else
  {
    nError = -ENOMEM;
  }
  return( nError );
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

static int lfd_close( void* pVolume, void* pNode, void* pCookie )
{
#if 0
  int	nError;

  nError = lfd_Lock();

  if ( 0 == nError )
  {
    if ( NULL != psFile->f_pFSData ) {
    } else {
      FileNode_s*	psNode;

      psNode	=	psFile->f_psInode->i_pFSData;

      if ( NULL != psNode ) {
	  /*
	    if ( -1 != psNode->fn_hFileHandle ) {
	    sys_Forbid();
	    dos_setftime( psNode->fn_hFileHandle, psFile->f_psInode->i_nMTime );
	    close( psNode->fn_hFileHandle );
	    sys_Permit();
	    psNode->fn_hFileHandle = -1;
	    }
	    */
      } else {
	printk( "ERROR : lfd_Close() Inode without i_pFSData!!!\n" );
      }
    }
    lfd_Unlock();
  }
  else
  {
    printk( "WARNING : lfd_Close() could not obtain file system semaphore!!!!!!!!\n" );
  }
#endif
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

FileEntry_s*	lfd_InodeToEntry( int nInode )
{
  return( (FileEntry_s*) nInode );
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

static int lfd_read_inode( void* pVolume, ino_t nInodeNum, void** ppNode )
{
  LfdVolume_s* 	psVolume = pVolume;
  FileEntry_s*	psEntry	 = lfd_InodeToEntry( nInodeNum );
  FileNode_s*	psNode;
  int		nError;

  nError = lfd_Lock();

  if ( stricmp( psEntry->fe_zShortName, LFD_INODE_FILE_NAME ) == 0 ) {
    printk( "Error: lfd_read_inode() attempt to load inode table\n" );
    lfd_Unlock();
    return( -EINVAL );
  }
  
  if ( 0 == nError )
  {
    for ( psNode = g_psFirstNode ; NULL != psNode ; psNode = psNode->fn_psNext ) {
      if ( psNode->fn_psFileEntry == psEntry ) {
	break;
      }
    }

    if ( NULL != psEntry ) {
      if ( NULL == psNode ) {
	if ( lfd_create_node( psVolume, psEntry, &psNode ) != 0 ) {
	  psNode = NULL;
	}
      }
    }
    if ( NULL != psNode ) {
      *ppNode = psNode;
      psEntry->fe_nFlags |= FE_INODE_LOADED;
    }
    lfd_Unlock();
  }
  else
  {
    printk( "WARNING : lfd_read_inode() could not obtain file system semaphore!!!!!!!!\n" );
  }
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

static int lfd_write_inode( void* pVolume, void* pNode )
{
  LfdVolume_s*	psVolume = pVolume;
  FileNode_s*	psNode = pNode;
  int		nError;

  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_write_inode() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }

  if ( NULL != psNode )
  {
    kassertw( psNode->fn_psFileEntry->fe_nFlags & FE_INODE_LOADED );
    psNode->fn_psFileEntry->fe_nFlags &= ~FE_INODE_LOADED;
    if ( psNode->fn_nLinkCount == 0 ) {
      lfd_DeleteFileEntry( psNode->fn_psFileEntry );
      psNode->fn_psFileEntry = NULL;
    }
    lfd_delete_node( psVolume, psNode );
  }
  else
  {
    printk( "ERROR : lfd_ReleaseInode() i_pFSData == NULL\n" );
  }
  lfd_Unlock();
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

static int lfd_mkdir( void* pVolume, void* pParent, const char *pzName, int nNameLen, int nMode )
{
  FileNode_s*	psParentNode = pParent;
  int		nError = 0;
  int nDOSMode = DOS_S_IFDIR | (nMode & S_IRWXUGO);
  if ( nMode & S_ISUID ) nDOSMode |= DOS_S_ISUID;
  if ( nMode & S_ISGID ) nDOSMode |= DOS_S_ISGID;

  if ( NULL == pParent ) {
    return( -ENOENT );
  }

  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_MkDir() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }
  
  nError = lfd_create_file_entry( psParentNode->fn_psFileEntry, pzName, nNameLen, nDOSMode, NULL );

  lfd_Unlock();
  
  return( nError );
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

static int lfd_rmdir( void* pVolume, void* pParent, const char *pzName, int nNameLen )
{
  LfdVolume_s*	psVolume = pVolume;
  FileNode_s*	psParent = pParent;
  FileEntry_s*	psEntry;
  int		nError = 0;

  if ( NULL == pParent ) {
    printk( "ERROR : lfd_rmdir() called with NULL pointer as psParent!\n" );
    return( -EINVAL );
  }
retry:
  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_rmdir() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }
  
  nError = lfd_LookupFile( psParent->fn_psFileEntry, pzName, nNameLen, &psEntry );

  if ( nError < 0 ) {
    goto error;
  }
  if ( DOS_S_ISDIR( psEntry->fe_nMode ) == false ) {
    nError = -ENOTDIR;
    printk( "Error: lfd_rmdir() attempt to remove regular file %s\n", pzName );
    goto error;
  }
  if ( psEntry->fe_nFlags & FE_INODE_LOADED ) {
    lfd_Unlock();
    printk( "lfd_rmdir() must flush inode for %s\n", pzName );
    if ( flush_inode(  psVolume->lv_nDevNum, (uint32) psEntry ) == 0 ) {
      goto retry;
    }
    lfd_Lock();
    nError = -EBUSY;
    printk( "Error: lfd_rmdir() can't remove directory loaded by the kernel\n" );
  } else {
    nError = lfd_DeleteFileEntry( psEntry );
  }
error:
  lfd_Unlock();
  return( nError );
}

static int lfd_unlink( void* pVolume, void* pParent, const char *pzName, int nNameLen )
{
  LfdVolume_s*	psVolume = pVolume;
  FileNode_s*	psParent;
  FileNode_s*	psNode;
  FileEntry_s*	psEntry;
  int		nError = 0;

  if ( NULL == pParent ) {
    printk( "ERROR : lfd_unlink() called with NULL pointer as psParent!\n" );
    return( -EINVAL );
  }

  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_unlink() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }

  psParent = pParent;

  nError = lfd_LookupFile( psParent->fn_psFileEntry, pzName, nNameLen, &psEntry );

  if ( nError < 0 ) {
    goto error;
  }
  
  if ( DOS_S_ISDIR( psEntry->fe_nMode )  ) {
    nError = -EISDIR;
    goto error;
  }

  nError = get_vnode( psVolume->lv_nDevNum, (int) psEntry, (void**) &psNode );

  if ( nError >= 0 )
  {
    static int nDelNum = 0;
    char       zNewName[16];

    sprintf( zNewName, "%08x.del", nDelNum++ );

    psNode->fn_nLinkCount--;

    kassertw( 0 == psNode->fn_nLinkCount );

    nError = lfd_rename_entry( psEntry, psVolume->lv_psDelDirEntry, zNewName, strlen( zNewName ) );
    if ( nError < 0 ) {
      printk( "Error: lfd_unlink() failed to move %s into delme as %s\n", pzName, zNewName );
    }
  } else {
    printk( "Error: lfd_unlink() failed to load vnode for %s\n", pzName );
  }
  put_vnode( psVolume->lv_nDevNum, (int) psEntry );
error:
  lfd_Unlock();

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

static int lfd_rename( void* pVolume, void* pOldDir, const char* pzOldName, int nOldNameLen,
		       void* pNewDir, const char* pzNewName, int nNewNameLen, bool bMustBeDir )
{
  FileNode_s*	psODirNode;
  FileNode_s*	psNDirNode;
  FileEntry_s*	psOEntry;
  FileEntry_s*	psNEntry;
  int		nError = 0;

  nError = lfd_Lock();

  if ( nError < 0 ) {
    printk( "WARNING : lfd_Rename() could not obtain file system semaphore!!!!!!!!\n" );
    return( nError );
  }
  
  psODirNode	=	pOldDir;
  psNDirNode	=	pNewDir;

  nError = lfd_LookupFile( psODirNode->fn_psFileEntry, pzOldName, nOldNameLen, &psOEntry );

  if ( nError < 0 ) {
    printk( "ERROR : lfd_Rename() Could not lookup '%s'\n", pzOldName );
    goto error;
  }
  
  if ( psOEntry->fe_psParent == NULL ) {
    nError = -EACCES;
    goto error;
  }
  if ( bMustBeDir && DOS_S_ISDIR( psOEntry->fe_nMode ) == false ) {
    nError = -ENOTDIR;
    goto error;
  }
  
  nError = lfd_LookupFile( psNDirNode->fn_psFileEntry, pzNewName, nNewNameLen, &psNEntry );

  if ( 0 == nError ) {
    nError = lfd_unlink( pVolume, pNewDir, pzNewName, nNewNameLen );

    if ( 0 != nError ) {
      printk( "ERROR : lfd_rename() failed to delete dest <%s> Err=%d\n", pzNewName, nError );
    }
  }

  if ( 0 == nError || -ENOENT == nError )
  {
    nError = lfd_rename_entry( psOEntry, psNDirNode->fn_psFileEntry,
			       pzNewName, nNewNameLen );
  }
  else
  {
    if ( 0 == nError ) {
      nError = -EEXIST;
    }
  }
error:
  lfd_Unlock();
  return( nError );
}



static FSOperations_s	g_sOperations =
{
  lfd_mount,
  lfd_UnMount,
  lfd_read_inode,
  lfd_write_inode,
  lfd_lookup,
  NULL,			/* op_access */
  lfd_create,
  lfd_mkdir,
  NULL,			/* mknod	*/
  lfd_symlink,		/* op_symlink */
  NULL,			/* op_link */
  lfd_rename,
  lfd_unlink,
  lfd_rmdir,
  lfd_readlink,		/* op_readlink */
  NULL,			/* lfd_OpenDir */
  NULL,			/* lfd_CloseDir */
  lfd_rewinddir,	/* op_rewinddir */
  lfd_read_dir,
  lfd_open,
  lfd_close,
  NULL,			/* lfd_FreeCookie */
  lfd_read,		// op_read
  lfd_write,		// op_write
  NULL,			// op_readv
  NULL,			// op_writev
  NULL,			/* op_ioctl */
  NULL, 		/* op_setflags */
  lfd_rstat,		/* lfd_Stat */
  lfd_wstat,		/* op_wstat */
  NULL,			/* op_fsync */
  NULL, 		/* op_initialize */
  NULL, 		/* op_sync */
  NULL, 		/* op_rfsstat */
  NULL, 		/* op_wfsstat */
  NULL			/* op_isatty	*/
};

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

int sys_SyncDosFS( void )
{
  if ( lfd_Lock() == 0 )
  {
//    lfd_FlushAllBuffers();
    while( lfd_flush_some_buffers() );
    lfd_close_dos_handle( NULL );
    lfd_Unlock();
  }
  else
  {
    printk( "WARNING : lfd_SyncDosFS() could not obtain file system semaphore!!!!!!!!\n" );
  }
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

static int SyncThread1( void* pData )
{
  for (;;)
  {
    snooze( 1000000 );
    if ( lfd_Lock() == 0 )
    {
      lfd_flush_some_buffers();
//      lfd_close_dos_handle( NULL );
      lfd_Unlock();
    }
    else
    {
      printk( "WARNING : lfd_SyncDosFS() could not obtain file system semaphore!!!!!!!!\n" );
    }
  }
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

void register_lfd( int nFATCacheSize )
{
  thread_id	hSyncThread1;

  g_hFileSysLock = create_semaphore( "lfd_lock", 1, SEM_REQURSIVE );
  
  lfd_InitCache( nFATCacheSize );

  register_file_system( "dosfat", &g_sOperations );

  hSyncThread1 = spawn_kernel_thread( "lfd_syncer1", SyncThread1, 0, DEFAULT_KERNEL_STACK, NULL );
  wakeup_thread( hSyncThread1, false );

  printk( "LFNDOS file system initialized %p\n", &g_sOperations );
}

