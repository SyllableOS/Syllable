
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
#include <atheos/filesystem.h>
#include <atheos/kernel.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "vfs.h"

typedef struct FileNode FileNode_s;
typedef struct SuperInfo SuperInfo_s;

#define	RFS_MAX_NAME_LEN	64


enum
{
	RFS_ROOT = 1,
};


struct FileNode
{
	FileNode_s *fn_psNextHash;
	FileNode_s *fn_psNextSibling;
	FileNode_s *fn_psParent;
	FileNode_s *fn_psFirstChild;
	ino_t fn_nInodeNum;
	int fn_nMode;
	int fn_nSize;
	int fn_nLinkCount;
	bool fn_bIsLoaded;	///< true between read_inode() and write_inode()
	char fn_zName[RFS_MAX_NAME_LEN];
	char *fn_pzSymLinkPath;
};

typedef struct
{
	kdev_t nDevNum;
	FileNode_s *psRootNode;
} RootVolume_s;

typedef struct
{
	int nPos;
} RFSCookie_s;

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static FileNode_s *rfs_create_node( FileNode_s *psParent, const char *pzName, int nNameLen, int nMode )
{
	FileNode_s *psNewNode;

	if ( nNameLen < 1 || nNameLen >= RFS_MAX_NAME_LEN )
	{
		return ( NULL );
	}


	if ( ( psNewNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR ) ) )
	{
		memcpy( psNewNode->fn_zName, pzName, nNameLen );
		psNewNode->fn_zName[nNameLen] = 0;

		psNewNode->fn_nMode = nMode;


		psNewNode->fn_psNextSibling = psParent->fn_psFirstChild;
		psParent->fn_psFirstChild = psNewNode;
		psNewNode->fn_psParent = psParent;
		psNewNode->fn_nLinkCount = 1;

		psNewNode->fn_nInodeNum = ( uint )psNewNode;

		return ( psNewNode );
	}
	return ( NULL );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static void rfs_DeleteNode( FileNode_s *psNode )
{
	FileNode_s *psParent = psNode->fn_psParent;
	FileNode_s **ppsTmp;

	if ( NULL == psParent )
	{
		printk( "iiik, sombody tried to delete '/' !!!\n" );
		return;
	}

	for ( ppsTmp = &psParent->fn_psFirstChild; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->fn_psNextSibling ) )
	{
		if ( *ppsTmp == psNode )
		{
			*ppsTmp = psNode->fn_psNextSibling;
			break;
		}
	}
	kfree( psNode );

/*	sys_DeleteSemaphore( psNode->fn_hSema ); */

}


/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_lookup( void *pVolume, void *pParent, const char *pzName, int nNameLen, ino_t *pnResInode )
{
	FileNode_s *psParentNode = pParent;
	FileNode_s *psNewNode;

	*pnResInode = 0;

	if ( nNameLen == 2 && '.' == pzName[0] && '.' == pzName[1] )
	{
		if ( NULL != psParentNode->fn_psParent )
		{
			*pnResInode = psParentNode->fn_psParent->fn_nInodeNum;
		}
		else
		{
			printk( "Error: rfs_lookup() called with .. on root level\n" );
			return ( -ENOENT );
		}
	}
	for ( psNewNode = psParentNode->fn_psFirstChild; NULL != psNewNode; psNewNode = psNewNode->fn_psNextSibling )
	{
		if ( strlen( psNewNode->fn_zName ) == nNameLen && strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 )
		{
			*pnResInode = psNewNode->fn_nInodeNum;
			break;
		}
	}

	if ( 0L != *pnResInode )
	{
		return ( 0 );
	}
	else
	{
		return ( -ENOENT );
	}
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen )
{
	return ( -EACCES );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuf, size_t nLen )
{
	return ( -EACCES );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

int rfs_readdir( void *pVolume, void *pNode, void *pCookie, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	FileNode_s *psParentNode = pNode;
	FileNode_s *psNode;
	int nCurPos = nPos;

	for ( psNode = psParentNode->fn_psFirstChild; NULL != psNode; psNode = psNode->fn_psNextSibling )
	{
		if ( nCurPos == 0 )
		{
			strcpy( psFileInfo->d_name, psNode->fn_zName );
			psFileInfo->d_namlen = strlen( psFileInfo->d_name );
			psFileInfo->d_ino = psNode->fn_nInodeNum;
			return ( 1 );
		}
		nCurPos--;
	}
	return ( 0 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	RootVolume_s *psVolume = pVolume;
	FileNode_s *psNode = pNode;

	psStat->st_ino = psNode->fn_nInodeNum;
	psStat->st_dev = psVolume->nDevNum;
	psStat->st_size = psNode->fn_nSize;
	psStat->st_mode = psNode->fn_nMode;
	psStat->st_nlink = psNode->fn_nLinkCount;
	psStat->st_atime = 0;
	psStat->st_mtime = 0;
	psStat->st_ctime = 0;
	psStat->st_uid = 0;
	psStat->st_gid = 0;

	return ( 0 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_open( void *pVolume, void *pNode, int nMode, void **ppCookie )
{
	return ( 0 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_close( void *pVolume, void *pNode, void *pCookie )
{
	return ( 0 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	RootVolume_s *psVolume = pVolume;
	FileNode_s *psNode;

	switch ( nInodeNum )
	{
	case RFS_ROOT:
		psNode = psVolume->psRootNode;
		break;
	default:
		psNode = ( FileNode_s * )( ( int )nInodeNum );
		if ( psNode->fn_nInodeNum != ( int )psNode )
		{
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
		return ( 0 );
	}
	else
	{
		*ppNode = NULL;
		return ( -EINVAL );
	}
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_write_inode( void *pVolData, void *pNode )
{
	FileNode_s *psNode = pNode;

	psNode->fn_bIsLoaded = false;

	if ( 0 == psNode->fn_nLinkCount )
	{
		printk( "rfs_write_inode() : Deleting node <%s>\n", psNode->fn_zName );

		if ( NULL != psNode->fn_pzSymLinkPath )
		{
			kfree( psNode->fn_pzSymLinkPath );
		}
		kfree( psNode );
	}
	return ( 0 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_mkdir( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nPerms )
{
	FileNode_s *psParentNode = pParent;
	FileNode_s *psNewNode;

	for ( psNewNode = psParentNode->fn_psFirstChild; psNewNode != NULL; psNewNode = psNewNode->fn_psNextSibling )
	{
		if ( strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 && strlen( psNewNode->fn_zName ) == nNameLen )
		{
			return ( -EEXIST );
		}
	}

	if ( ( psNewNode = rfs_create_node( psParentNode, pzName, nNameLen, S_IFDIR | ( nPerms & S_IRWXUGO ) ) ) )
	{
		return ( 0 );
	}
	return ( -1 );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_rmdir( void *pVolData, void *pParentNode, const char *pzName, int nNameLen )
{
	FileNode_s *psParent = pParentNode;
	FileNode_s *psNode = NULL;
	FileNode_s **ppsTmp;
	int nError = -ENOENT;

	for ( ppsTmp = &psParent->fn_psFirstChild; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->fn_psNextSibling ) )
	{
		if ( strlen( ( *ppsTmp )->fn_zName ) == nNameLen && strncmp( ( *ppsTmp )->fn_zName, pzName, nNameLen ) == 0 )
		{
			psNode = *ppsTmp;

			if ( S_ISDIR( psNode->fn_nMode ) == false )
			{
				nError = -ENOTDIR;
				psNode = NULL;
				break;
			}
			if ( psNode->fn_psFirstChild != NULL )
			{
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
			printk( "rfs_rmdir() : Delete node <%s>\n", pzName );
			if ( NULL != psNode->fn_pzSymLinkPath )
			{
				kfree( psNode->fn_pzSymLinkPath );
			}
			kfree( psNode );
		}
	}
	return ( nError );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_unlink( void *pVolData, void *pParentNode, const char *pzName, int nNameLen )
{
	FileNode_s *psParent = pParentNode;
	FileNode_s *psNode = NULL;
	FileNode_s **ppsTmp;
	int nError = -ENOENT;

	for ( ppsTmp = &psParent->fn_psFirstChild; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->fn_psNextSibling ) )
	{
		if ( strlen( ( *ppsTmp )->fn_zName ) == nNameLen && strncmp( ( *ppsTmp )->fn_zName, pzName, nNameLen ) == 0 )
		{
			psNode = *ppsTmp;
			if ( S_ISDIR( psNode->fn_nMode ) )
			{
				nError = -EISDIR;
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
			printk( "rfs_unlink() : Delete node <%s>\n", pzName );
			if ( NULL != psNode->fn_pzSymLinkPath )
			{
				kfree( psNode->fn_pzSymLinkPath );
			}
			kfree( psNode );
		}
	}
	return ( nError );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/


static int rfs_symlink( void *pVolume, void *pParentNode, const char *pzName, int nNameLen, const char *pzNewPath )
{
	FileNode_s *psParent = pParentNode;
	FileNode_s *psNode;
	int nError = 0;

	psNode = rfs_create_node( psParent, pzName, nNameLen, S_IFLNK | S_IRUGO | S_IWUGO );

	if ( NULL != psNode )
	{
		psNode->fn_pzSymLinkPath = kmalloc( strlen( pzNewPath ) + 1, MEMF_KERNEL );

		if ( NULL != psNode->fn_pzSymLinkPath )
		{
			strcpy( psNode->fn_pzSymLinkPath, pzNewPath );
		}
		else
		{
			nError = -ENOMEM;
			rfs_DeleteNode( psNode );
		}
	}
	else
	{
		nError = -ENOMEM;
	}
	return ( nError );
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static int rfs_readlink( void *pVolume, void *pNode, char *pzBuf, size_t nBufSize )
{
	FileNode_s *psNode = pNode;

	if ( NULL != psNode->fn_pzSymLinkPath )
	{
		strncpy( pzBuf, psNode->fn_pzSymLinkPath, nBufSize );
		pzBuf[nBufSize - 1] = '\0';
		return ( strlen( pzBuf ) );
	}
	else
	{
		return ( -EINVAL );
	}
}

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

static FSOperations_s g_sOperations = {
	NULL,			// op_probe
	NULL,			// op_mount
	NULL,			// op_unmount
	rfs_read_inode,
	rfs_write_inode,
	rfs_lookup,		/* Lookup       */
	NULL,			/* access       */
	NULL,			/* create,      */
	rfs_mkdir,		/* mkdir        */
	NULL,			/* mknod        */
	rfs_symlink,		/* symlink      */
	NULL,			/* link         */
	NULL,			/* rename       */
	rfs_unlink,		/* unlink       */
	rfs_rmdir,		/* rmdir        */
	rfs_readlink,		/* readlink     */
	NULL,			/* ppendir      */
	NULL,			/* closedir     */
	NULL,			/* RewindDir    */
	rfs_readdir,		/* ReadDir      */
	rfs_open,		/* open         */
	rfs_close,		/* close        */
	NULL,			/* FreeCookie   */
	rfs_read,		// op_read
	rfs_write,		// op_write
	NULL,			// op_readv
	NULL,			// op_writev
	NULL,			/* ioctl        */
	NULL,			/* setflags     */
	rfs_rstat,
	NULL,			/* wstat        */
	NULL,			/* fsync        */
	NULL,			/* initialize   */
	NULL,			/* sync         */
	NULL,			/* rfsstat      */
	NULL,			/* wfsstat      */
	NULL			/* isatty       */
};

/****** Virtual root file system ********************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

bool mount_root_fs( void )
{
	RootVolume_s *psVolume;
	Volume_s *psKVolume;
	FileNode_s *psRootNode;

	psKVolume = kmalloc( sizeof( Volume_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psKVolume == NULL )
	{
		panic( "mount_root_fs() failed to alloc memory for kernel volume struct\n" );
		return ( false );
	}

	psVolume = kmalloc( sizeof( RootVolume_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psVolume == NULL )
	{
		panic( "mount_root_fs() failed to alloc memory for volume struct\n" );
		return ( false );
	}

	psRootNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psRootNode == NULL )
	{
		panic( "mount_root_fs() failed to alloc memory for root inode\n" );
		return ( false );
	}


	psRootNode->fn_zName[0] = '\0';
	psRootNode->fn_nInodeNum = RFS_ROOT;
	psRootNode->fn_nMode = S_IFDIR | 0777;
	psRootNode->fn_psNextSibling = NULL;
	psRootNode->fn_psParent = NULL;

	psVolume->nDevNum = FSID_ROOT;

	psVolume->psRootNode = psRootNode;

	psKVolume->v_psNext = NULL;
	psKVolume->v_nDevNum = FSID_ROOT;
	psKVolume->v_pFSData = psVolume;
	psKVolume->v_psOperations = &g_sOperations;
	psKVolume->v_nRootInode = psRootNode->fn_nInodeNum;

	add_mount_point( psKVolume );

	CURRENT_PROC->pr_psIoContext->ic_psRootDir = get_inode( psVolume->nDevNum, psRootNode->fn_nInodeNum, false );

	printk( "Root file system mounted %p\n", &g_sOperations );

	return ( true );
}
