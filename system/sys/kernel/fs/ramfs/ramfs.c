/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/bootmodules.h>

#include <macros.h>

typedef	struct	FileNode	FileNode_s;
typedef	struct	SuperInfo	SuperInfo_s;

#define	RFS_MAX_NAME_LEN	64
#define RFS_MAX_FILESIZE	(1024*1024*2)

enum
{
  RFS_ROOT = 1,
};


struct	FileNode
{
    FileNode_s*	fn_psNextHash;
    FileNode_s*	fn_psNextSibling;
    FileNode_s*	fn_psParent;
    FileNode_s*	fn_psFirstChild;
    int		fn_nInodeNum;
    int		fn_nMode;
    int		fn_nSize;
    int		fn_nTime;
    int		fn_nLinkCount;
    bool	fn_bIsLoaded;	// true between read_inode() and write_inode()
    char	fn_zName[ RFS_MAX_NAME_LEN ];
    char*	fn_pBuffer;
};

typedef struct
{
    kdev_t	rv_nDevNum;
    ino_t	rv_hMutex;
    FileNode_s*	rv_psRootNode;
    int		rv_nFSSize;
} RDVolume_s;

typedef struct
{
    int	nPos;
} RFSCookie_s;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FileNode_s* rfs_create_node( FileNode_s* psParent, const char* pzName, int nNameLen, int nMode )
{
    FileNode_s*	psNewNode;

    if ( nNameLen < 1 || nNameLen >= RFS_MAX_NAME_LEN ) {
	return( NULL );
    }

  
    if ( (psNewNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK ) ) ) {
	memcpy( psNewNode->fn_zName, pzName, nNameLen );
	psNewNode->fn_zName[ nNameLen ] = 0;

	psNewNode->fn_nMode	=	nMode;
	psNewNode->fn_nTime    = get_real_time() / 1000000;

	psNewNode->fn_psNextSibling = psParent->fn_psFirstChild;
	psParent->fn_psFirstChild   = psNewNode;
	psNewNode->fn_psParent	    = psParent;
	psNewNode->fn_nLinkCount    = 1;
		
	psNewNode->fn_nInodeNum	=	(int) psNewNode;

	return( psNewNode );
    }
    return( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void rfs_delete_node( RDVolume_s* psVolume, FileNode_s* psNode )
{
    FileNode_s* psParent = psNode->fn_psParent;
    FileNode_s** ppsTmp;

    if ( psParent != NULL ) {
	for ( ppsTmp = &psParent->fn_psFirstChild ; NULL != *ppsTmp ; ppsTmp = &((*ppsTmp)->fn_psNextSibling) ) {
	    if ( *ppsTmp == psNode ) {
		*ppsTmp = psNode->fn_psNextSibling;
		break;
	    }
	}
    }
    psVolume->rv_nFSSize -= psNode->fn_nSize;
    kfree( psNode );
}

static FileNode_s* rfs_find_node( FileNode_s* psParent, const char* pzName, int nNameLen )
{
    FileNode_s*	psNode;

    if ( nNameLen == 1 && '.' == pzName[0] ) {
	return( psParent );
    }
    if ( nNameLen == 2 && '.' == pzName[0] && '.' == pzName[1] ) {
	if ( NULL != psParent->fn_psParent ) {
	    return( psParent->fn_psParent );
	} else {
	    printk( "Error: rfs_find_node() called with .. on root level\n" );
	    return( NULL );
	}
    }
    for ( psNode = psParent->fn_psFirstChild ; NULL != psNode ; psNode = psNode->fn_psNextSibling ) {
	if ( strlen( psNode->fn_zName ) == nNameLen && strncmp( psNode->fn_zName, pzName, nNameLen ) == 0 ) {
	    return( psNode );
	}
    }
    return( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_lookup( void* pVolume, void* pParent, const char* pzName, int nNameLen, ino_t* pnResInode )
{
    FileNode_s*	psParentNode = pParent;
    FileNode_s*	psNewNode;

    *pnResInode = 0;

    if ( nNameLen == 1 && '.' == pzName[0] ) {
	*pnResInode = psParentNode->fn_nInodeNum;
	goto done;
    }
    if ( nNameLen == 2 && '.' == pzName[0] && '.' == pzName[1] ) {
	if ( NULL != psParentNode->fn_psParent ) {
	    *pnResInode = psParentNode->fn_psParent->fn_nInodeNum;
	    goto done;
	} else {
	    printk( "Error: rfs_lookup() called with .. on root level\n" );
	    return( -ENOENT );
	}
    }
    for ( psNewNode = psParentNode->fn_psFirstChild ; NULL != psNewNode ; psNewNode = psNewNode->fn_psNextSibling ) {
	if ( strlen(psNewNode->fn_zName ) == nNameLen && strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 ) {
	    *pnResInode = psNewNode->fn_nInodeNum;
	    break;
	}
    }
done:
    if ( 0L != *pnResInode ) {
	return( 0 );
    } else {
	return( -ENOENT );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_read( void* pVolume, void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nSize )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s* psNode = pNode;
    int		nError;

    LOCK( psVolume->rv_hMutex );
    if ( psNode->fn_pBuffer == NULL || nPos >= psNode->fn_nSize ) {
	nError = 0;
	goto done;
    }
    if ( nPos + nSize > psNode->fn_nSize ) {
	nSize = psNode->fn_nSize - nPos;
    }
    memcpy( pBuf, psNode->fn_pBuffer + nPos, nSize );
    nError = nSize;
done:
    UNLOCK( psVolume->rv_hMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_write( void* pVolume, void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s* psNode = pNode;
    int		nError;

    if ( nPos + nLen > RFS_MAX_FILESIZE ) {
	return( -EFBIG );
    }
    if ( nPos < 0 ) {
	return( -EINVAL );
    }
    
    LOCK( psVolume->rv_hMutex );
    if ( nPos + nLen > psNode->fn_nSize || psNode->fn_pBuffer == NULL ) {
	void* pBuffer = kmalloc( nPos + nLen, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( pBuffer == NULL ) {
	    nError = -ENOMEM;
	    goto error;
	}
	if ( psNode->fn_pBuffer != NULL ) {
	    memcpy( pBuffer, psNode->fn_pBuffer, psNode->fn_nSize );
	    kfree( psNode->fn_pBuffer );
	}
	psVolume->rv_nFSSize += nPos + nLen - psNode->fn_nSize;
	psNode->fn_pBuffer = pBuffer;
	psNode->fn_nSize = nPos + nLen;
    }
    memcpy( psNode->fn_pBuffer + nPos, pBuf, nLen );
    nError = nLen;
error:
    UNLOCK( psVolume->rv_hMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int rfs_readdir( void* pVolume, void* pNode, void* pCookie, int nPos,
		 struct kernel_dirent* psFileInfo, size_t nBufSize )
{
    FileNode_s* psParentNode = pNode;
    FileNode_s* psNode;
    int         nCurPos = nPos;

    if ( nCurPos == 0 ) {
	strcpy( psFileInfo->d_name, "." );
	psFileInfo->d_namlen = 1;
	psFileInfo->d_ino    = psParentNode->fn_nInodeNum;
	return( 1 );
    } else if ( nCurPos == 1 && psParentNode->fn_psParent != NULL ) {
	strcpy( psFileInfo->d_name, ".." );
	psFileInfo->d_namlen = 2;
	psFileInfo->d_ino    = psParentNode->fn_psParent->fn_nInodeNum;
	return( 1 );
    }
    if ( psParentNode->fn_psParent == NULL ) {
	nCurPos -= 1;
    } else {
	nCurPos -= 2;
    }
    
    for ( psNode = psParentNode->fn_psFirstChild ; NULL != psNode ; psNode = psNode->fn_psNextSibling )
    {
	if ( nCurPos == 0 )
	{
	    strcpy( psFileInfo->d_name, psNode->fn_zName );
	    psFileInfo->d_namlen = strlen( psFileInfo->d_name );
	    psFileInfo->d_ino	   = psNode->fn_nInodeNum;
			
	    return( 1 );
	}
	nCurPos--;
    }
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_rstat( void* pVolume, void* pNode, struct stat* psStat )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s*   psNode = pNode;

    psStat->st_ino 	= psNode->fn_nInodeNum;
    psStat->st_dev	= psVolume->rv_nDevNum;
    psStat->st_size 	= psNode->fn_nSize;
    psStat->st_mode	= psNode->fn_nMode;
    psStat->st_nlink	= psNode->fn_nLinkCount;
    psStat->st_atime	= psNode->fn_nTime;
    psStat->st_mtime	= psNode->fn_nTime;
    psStat->st_ctime	= psNode->fn_nTime;
    psStat->st_uid	= 0;
    psStat->st_gid	= 0;

    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_open( void* pVolume, void* pNode, int nMode, void** ppCookie )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_close( void* pVolume, void* pNode, void* pCookie )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_read_inode( void* pVolume, ino_t nInodeNum, void** ppNode )
{
    RDVolume_s*	psVolume = pVolume;
    FileNode_s*	psNode;

    switch( nInodeNum )
    {
	case RFS_ROOT:
	    psNode = psVolume->rv_psRootNode;
	    break;
	default:
	    psNode = (FileNode_s*) ((int)nInodeNum);
	    if ( psNode->fn_nInodeNum != (int) psNode ) {
		printk( "rfs_read_inode() invalid inode %Lx\n", nInodeNum );
		psNode = NULL;
	    }
	    break;
    }
	
    if ( NULL != psNode )
    {
	kassertw( false == psNode->fn_bIsLoaded );
	psNode->fn_bIsLoaded = true;
	*ppNode = psNode;
	return( 0 );
    }
    else
    {
	*ppNode = NULL;
	return( -EINVAL );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_write_inode( void* pVolume, void* pNode )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s* psNode   = pNode;
	
    psNode->fn_bIsLoaded	= false;

    if ( 0 == psNode->fn_nLinkCount )
    {
	if ( NULL != psNode->fn_pBuffer ) {
	    kfree( psNode->fn_pBuffer );
	}
	psVolume->rv_nFSSize -= psNode->fn_nSize;
	kfree( psNode );
    }
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_mkdir( void* pVolume, void* pParent, const char *pzName, int nNameLen, int nPerms )
{
    FileNode_s*	psParentNode = pParent;
    FileNode_s*	psNewNode;

    for ( psNewNode = psParentNode->fn_psFirstChild ; psNewNode != NULL ; psNewNode = psNewNode->fn_psNextSibling ) {
	if ( strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 && strlen( psNewNode->fn_zName ) == nNameLen ) {
	    return( -EEXIST );
	}
    }
  
    if ( (psNewNode = rfs_create_node( psParentNode, pzName, nNameLen, S_IFDIR | (nPerms & S_IRWXUGO) )) )
    {
	return( 0 );
    }
    return( -1 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_rmdir( void* pVolume, void* pParentNode, const char* pzName, int nNameLen )
{
    RDVolume_s*	 psVolume = pVolume;
    FileNode_s*  psParent = pParentNode;
    FileNode_s*	 psNode   = NULL;
    FileNode_s** ppsTmp;
    int		 nError = -ENOENT;
	
    for ( ppsTmp = &psParent->fn_psFirstChild ; NULL != *ppsTmp ; ppsTmp = &((*ppsTmp)->fn_psNextSibling) ) {
	if ( strlen( (*ppsTmp)->fn_zName ) == nNameLen && strncmp( (*ppsTmp)->fn_zName, pzName, nNameLen ) == 0 ) {
	    psNode = *ppsTmp;
			
	    if ( S_ISDIR( psNode->fn_nMode ) == false ) {
		nError = -ENOTDIR;
		psNode = NULL;
		break;
	    }
	    if ( psNode->fn_psFirstChild != NULL ) {
		nError = -ENOTEMPTY;
		psNode = NULL;
		break;
	    }
	    *ppsTmp = psNode->fn_psNextSibling;
	    nError = 0;
	    break;
	}
    }

    if ( NULL != psNode )
    {
	psNode->fn_psParent = NULL;
	psNode->fn_nLinkCount--;

	kassertw( 0 == psNode->fn_nLinkCount );
		
	if ( false == psNode->fn_bIsLoaded )
	{
	    if ( NULL != psNode->fn_pBuffer ) {
		kfree( psNode->fn_pBuffer );
	    }
	    psVolume->rv_nFSSize -= psNode->fn_nSize;
	    kfree( psNode );
	}
    }
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_unlink( void* pVolume, void* pParentNode, const char* pzName, int nNameLen )
{
    RDVolume_s*	 psVolume = pVolume;
    FileNode_s*  psParent = pParentNode;
    FileNode_s*	 psNode	  = NULL;
    FileNode_s** ppsTmp;
    int		 nError = -ENOENT;
	
    for ( ppsTmp = &psParent->fn_psFirstChild ; NULL != *ppsTmp ; ppsTmp = &((*ppsTmp)->fn_psNextSibling) ) {
	if ( strlen( (*ppsTmp)->fn_zName ) == nNameLen && strncmp( (*ppsTmp)->fn_zName, pzName, nNameLen ) == 0 ) {
	    psNode = *ppsTmp;
	    if ( S_ISDIR( psNode->fn_nMode ) ) {
		nError = -EISDIR;
		psNode = NULL;
		break;
	    }
	    *ppsTmp = psNode->fn_psNextSibling;
	    nError = 0;
	    break;
	}
    }

    if ( NULL != psNode ) {
	psNode->fn_psParent = NULL;
	psNode->fn_nLinkCount--;

	kassertw( 0 == psNode->fn_nLinkCount );
		
	if ( psNode->fn_bIsLoaded == false ) {
	    if ( NULL != psNode->fn_pBuffer ) {
		kfree( psNode->fn_pBuffer );
	    }
	    psVolume->rv_nFSSize -= psNode->fn_nSize;
	    kfree( psNode );
	}
    }
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_create( void* pVolume, void* pParent, const char* pzName, int nNameLen,
		       int nMode, int nPerms, ino_t* pnInodeNum, void** ppCookie )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s* psParent = pParent;
    FileNode_s* psNode;
    int		nError = 0;

    LOCK( psVolume->rv_hMutex );

    for ( psNode = psParent->fn_psFirstChild ; psNode != NULL ; psNode = psNode->fn_psNextSibling ) {
	if ( strncmp( psNode->fn_zName, pzName, nNameLen ) == 0 && strlen( psNode->fn_zName ) == nNameLen ) {
	    UNLOCK( psVolume->rv_hMutex );
	    return( -EEXIST );
	}
    }
    
    psNode = rfs_create_node( psParent, pzName, nNameLen, S_IFREG | (nPerms & 0777) );
    if ( NULL != psNode ) {
	*pnInodeNum = psNode->fn_nInodeNum;
	ppCookie = NULL;
    } else {
	nError = -ENOMEM;
    }
    UNLOCK( psVolume->rv_hMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_symlink( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, const char* pzNewPath )
{
    RDVolume_s* psVolume = pVolume;
    FileNode_s* psParent = pParentNode;
    FileNode_s* psNode;
    int		nError = 0;

    LOCK( psVolume->rv_hMutex );

    for ( psNode = psParent->fn_psFirstChild ; psNode != NULL ; psNode = psNode->fn_psNextSibling ) {
	if ( strncmp( psNode->fn_zName, pzName, nNameLen ) == 0 && strlen( psNode->fn_zName ) == nNameLen ) {
	    UNLOCK( psVolume->rv_hMutex );
	    return( -EEXIST );
	}
    }
    
    psNode = rfs_create_node( psParent, pzName, nNameLen, S_IFLNK | S_IRUGO | S_IWUGO );

    if ( NULL != psNode ) {
	psNode->fn_nSize   = strlen( pzNewPath ) + 1;
	psNode->fn_pBuffer = kmalloc( psNode->fn_nSize , MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if ( NULL != psNode->fn_pBuffer ) {
	    strcpy( psNode->fn_pBuffer, pzNewPath );
	    psVolume->rv_nFSSize += psNode->fn_nSize;
	} else {
	    nError = -ENOMEM;
	    rfs_delete_node( psVolume, psNode );
	}
    } else {
	nError = -ENOMEM;
    }
    UNLOCK( psVolume->rv_hMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_readlink( void* pVolume, void* pNode, char* pzBuf, size_t nBufSize )
{
    FileNode_s* psNode = pNode;

    if ( S_ISLNK( psNode->fn_nMode ) == false ) {
	return( -EINVAL );
    }
    if ( psNode->fn_pBuffer == NULL ) {
	return( 0 );
    }
    if ( nBufSize > psNode->fn_nSize ) {
	nBufSize = psNode->fn_nSize;
    }
    memcpy( pzBuf, psNode->fn_pBuffer, nBufSize );
    return( nBufSize );
}

static int rfs_rename( void* pVolume, void* pOldDir, const char* pzOldName, int nOldNameLen,
		       void* pNewDir, const char* pzNewName, int nNewNameLen, bool bMustBeDir )
{
    RDVolume_s*  psVolume = pVolume;
    FileNode_s*  psOldDir = pOldDir;
    FileNode_s*  psNewDir = pNewDir;
    FileNode_s*	 psNode;
    FileNode_s*  psDstNode;
    int		 nError;
    LOCK( psVolume->rv_hMutex );

    psNode = rfs_find_node( psOldDir, pzOldName, nOldNameLen );
    if ( psNode == NULL ) {
	nError = -ENOENT;
	goto error;
    }
    if ( bMustBeDir && S_ISDIR( psNode->fn_nMode ) == false ) {
	nError = -ENOTDIR;
	goto error;
    }
    psDstNode = rfs_find_node( psNewDir, pzNewName, nNewNameLen );
    if ( psDstNode != NULL ) {
	if ( S_ISDIR( psDstNode->fn_nMode ) ) {
	    nError = -EISDIR;
	    goto error;
	}
	rfs_unlink( pVolume, pNewDir, pzNewName, nNewNameLen );
    }

    if ( psOldDir != psNewDir ) {
	FileNode_s** ppsTmp;
	for ( ppsTmp = &psOldDir->fn_psFirstChild ; NULL != *ppsTmp ; ppsTmp = &((*ppsTmp)->fn_psNextSibling) ) {
	    if ( psNode == *ppsTmp ) {
		*ppsTmp = psNode->fn_psNextSibling;
		break;
	    }
	}
	psNode->fn_psNextSibling = psNewDir->fn_psFirstChild;
	psNewDir->fn_psFirstChild = psNode;
    }
    memcpy( psNode->fn_zName, pzNewName, nNewNameLen );
    psNode->fn_zName[nNewNameLen] = '\0';
    nError = 0;
error:
    UNLOCK( psVolume->rv_hMutex );
    return( nError );
}

static int rfs_parse_image( RDVolume_s* psVolume, FileNode_s* psParent, uint8* pBuffer, int nSize )
{
    int nBytesRead = 0;
    
    while( nSize > 4 ) {
	char zName[256];
	uint32 nMode  = *((uint32*)pBuffer);
	uint32 nNameLen;
	pBuffer += 4;
	nSize   -= 4;
	nBytesRead += 4;

	if ( nMode == ~0 ) {
	    break;
	}
	
	nNameLen = *pBuffer++;
	nSize--;
	nBytesRead++;

	if ( nNameLen > nSize ) {
	    printk( "Warning: RAM disk image is corrupted\n" );
	    return( -EINVAL );
	}
	memcpy( zName, pBuffer, nNameLen );
	zName[nNameLen] = '\0';
	pBuffer += nNameLen;
	nSize   -= nNameLen;
	nBytesRead += nNameLen;

	if ( S_ISDIR( nMode ) ) {
	    FileNode_s* psDir;
	    int		nLen;

	    psDir = rfs_find_node( psParent, zName, nNameLen );
	    if ( psDir == NULL ) {
		psDir = rfs_create_node( psParent, zName, nNameLen, S_IFDIR | (nMode & S_IRWXUGO) );
	    } else {
		if ( S_ISDIR( psDir->fn_nMode ) == false ) {
		    printk( "Error: RAM disk contain both file and directory with the same name" );
		    return( -EINVAL );
		}
	    }
	    if ( psDir == NULL ) {
		return( -ENOMEM );
	    }
	    nLen = rfs_parse_image( psVolume, psDir, pBuffer, nSize );
	    if ( nLen < 0 ) {
		return( nLen );
	    }
	    pBuffer    += nLen;
	    nSize      -= nLen;
	    nBytesRead += nLen;
	} else if ( S_ISREG( nMode ) || S_ISLNK( nMode ) ) {
	    FileNode_s* psFile;
	    uint32 nFileSize;
	    
	    if ( nSize < 4 ) {
		printk( "Warning: RAM disk image is corrupted\n" );
		return( -EINVAL );
	    }
	    nFileSize = *((uint32*)pBuffer);
	    pBuffer += 4; nSize -= 4; nBytesRead += 4;
	    if ( nSize < nFileSize ) {
		printk( "Warning: RAM disk image is corrupted\n" );
		return( -EINVAL );
	    }
	    psFile = rfs_create_node( psParent, zName, nNameLen, nMode );
	    if ( psFile != NULL ) {
		psFile->fn_nSize = nFileSize;
		psFile->fn_pBuffer = kmalloc( nFileSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );
		if ( psFile->fn_pBuffer != NULL ) {
		    memcpy( psFile->fn_pBuffer, pBuffer, nFileSize );
		    psVolume->rv_nFSSize += nFileSize;
		} else {
		    rfs_delete_node( psVolume, psFile );
		}
	    }
	    pBuffer += nFileSize; nSize -= nFileSize; ; nBytesRead += nFileSize;
	} else {
	    return( -EINVAL );
	}
    }
    return( nBytesRead );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int rfs_mount( kdev_t nDevNum, const char* pzDevPath,
		      uint32 nFlags, const void* pArgs, int nArgLen, void** ppVolData, ino_t* pnRootIno )
{
    RDVolume_s* psVolume = kmalloc( sizeof( RDVolume_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
    FileNode_s*	psRootNode;
    int		nImageFile;
    
    if ( psVolume == NULL ) {
	return( -ENOMEM );
    }
    psRootNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
    if ( psRootNode == NULL ) {
	kfree( psVolume );
	return( -ENOMEM );
    }
    psVolume->rv_hMutex = create_semaphore( "ramfs_mutex", 1, SEM_RECURSIVE );
    if ( psVolume->rv_hMutex < 0 ) {
	kfree( psVolume );
	kfree( psRootNode );
	return( -ENOMEM );
    }

    psRootNode->fn_nTime    = get_real_time() / 1000000;
    psRootNode->fn_zName[ 0 ] = '\0';
    psRootNode->fn_nLinkCount = 1;

    psRootNode->fn_nMode	= 0777 | S_IFDIR;
    psRootNode->fn_nInodeNum	= RFS_ROOT;

    psRootNode->fn_psNextSibling = NULL;
    psRootNode->fn_psParent	 = NULL;

    psVolume->rv_psRootNode = psRootNode;
    psVolume->rv_nDevNum    = nDevNum;

    *pnRootIno = RFS_ROOT;
    *ppVolData = psVolume;

    if ( pzDevPath != NULL ) {
	if ( pzDevPath[0] == '@' ) {
	    int nModCount = get_boot_module_count();
	    const char* pzStart = pzDevPath + 1;
	    const char* pzName;
	    for ( pzName = pzStart ; ; ++pzName )
	    {
		char c = *pzName;
		int i;
		if ( c != ',' && c != '\0' ) {
		    continue;
		}
		for ( i = 0 ; i < nModCount ; ++i ) {
		    BootModule_s* psModule = get_boot_module( i );
		    if ( psModule == NULL ) {
			continue;
		    }
		    if ( strncmp( psModule->bm_pzModuleArgs, pzStart, pzName - pzStart ) == 0 ) {
			printk( "Init RAM FS from boot module %s\n", psModule->bm_pzModuleArgs );
			rfs_parse_image( psVolume, psRootNode, psModule->bm_pAddress, psModule->bm_nSize );
			put_boot_module( psModule );
			break;
		    }
		    put_boot_module( psModule );
		}
		if ( c == 0 ) {
		    break;
		}
		pzStart = pzName + 1;
	    }
	} else {
	    struct stat sStat;
	    nImageFile = open( pzDevPath, O_RDONLY );
	    if ( nImageFile < 0 ) {
		printk( "rfs_mount() failed to open image file %s\n", pzDevPath );
		return( 0 );
	    }
	    if ( fstat( nImageFile, &sStat ) >= 0 ) {
		void* pBuffer = kmalloc( sStat.st_size, MEMF_KERNEL | MEMF_OKTOFAILHACK );
		if ( pBuffer != NULL ) {
		    if ( read( nImageFile, pBuffer, sStat.st_size ) == sStat.st_size ) {
			rfs_parse_image( psVolume, psRootNode, pBuffer, sStat.st_size );
		    }
		    kfree( pBuffer );
		}
	    }
	}
    }
    return( 0 );
}

int rfs_rfsstat( void* pVolume, fs_info* psInfo )
{
    RDVolume_s* psVolume = pVolume;
    
    psInfo->fi_dev		= psVolume->rv_nDevNum;
    psInfo->fi_root		= RFS_ROOT;
    psInfo->fi_flags		= 0;
    psInfo->fi_block_size	= 512;
    psInfo->fi_io_size		= 65536;
    psInfo->fi_total_blocks	= (psVolume->rv_nFSSize + 511) / 512;
    psInfo->fi_free_blocks	= 0;
    psInfo->fi_free_user_blocks = 0;
    psInfo->fi_total_inodes	= -1;
    psInfo->fi_free_inodes	= -1;
    strcpy( psInfo->fi_volume_name, "ramfs" );
    return( 0 );
}

static void rfs_delete_all_files( RDVolume_s* psVolume, FileNode_s* psRoot )
{
    while ( psRoot->fn_psFirstChild != NULL ) {
	rfs_delete_all_files( psVolume, psRoot->fn_psFirstChild );
    }
    rfs_delete_node( psVolume, psRoot );
}

static int rfs_unmount( void* pVolume )
{
    RDVolume_s* psVolume = pVolume;
    rfs_delete_all_files( psVolume, psVolume->rv_psRootNode );
    delete_semaphore( psVolume->rv_hMutex );
    kfree( psVolume );
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FSOperations_s	g_sOperations =
{
    NULL,		// op_probe
    rfs_mount,		// op_mount
    rfs_unmount,	/* op_unmount */
    rfs_read_inode,
    rfs_write_inode,
    rfs_lookup, 	/* Lookup	*/
    NULL,		/* access 	*/
    rfs_create,		/* create, 	*/
    rfs_mkdir, 		/* mkdir 	*/
    NULL,		/* mknod	*/
    rfs_symlink,	/* symlink 	*/
    NULL,		/* link 	*/
    rfs_rename,		/* rename 	*/
    rfs_unlink,		/* unlink 	*/
    rfs_rmdir, 		/* rmdir 	*/
    rfs_readlink,	/* readlink 	*/
    NULL, 		/* ppendir 	*/
    NULL,		/* closedir 	*/
    NULL, 		/* RewindDir	*/
    rfs_readdir, 	/* ReadDir 	*/
    rfs_open,		/* open		*/
    rfs_close,		/* close 	*/
    NULL, 		/* FreeCookie	*/
    rfs_read, 		// op_read
    rfs_write,		// op_write
    NULL,		// op_readv
    NULL,		// op_writev
    NULL,		/* ioctl 	*/
    NULL, 		/* setflags 	*/
    rfs_rstat,
    NULL,		/* wstat 	*/
    NULL,		/* fsync 	*/
    NULL, 		/* initialize	*/
    NULL, 		/* sync 	*/
    rfs_rfsstat, 	/* rfsstat	*/
    NULL, 		/* wfsstat 	*/
    NULL		/* isatty	*/
};


int fs_init( const char* pzName, FSOperations_s** ppsOps )
{
    *ppsOps = &g_sOperations;
    return( FSDRIVER_API_VERSION );
}
