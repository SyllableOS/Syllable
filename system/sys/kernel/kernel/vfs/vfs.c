
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
#include <posix/fcntl.h>
#include <posix/unistd.h>
#include <posix/dirent.h>
#include <posix/utime.h>
#include <posix/time.h>
#include <posix/limits.h>
#include <posix/select.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "vfs.h"

static const int O_OPEN_CONTROL_FLAGS = (O_CREAT|O_EXCL|O_NOCTTY|O_TRUNC|O_DIRECTORY|O_NOTRAVERSE);

static int lookup_parent_inode( Inode_s *psParent, const char *pzPath, const char **ppzName, int *pnNameLen, Inode_s **ppsResInode );

void init_flock( void );

static IoContext_s *g_psKernelIoContext;

static bool g_bFixMode = false;

IoContext_s *get_current_iocxt( bool bKernel )
{
	return ( ( bKernel ) ? g_psKernelIoContext : CURRENT_PROC->pr_psIoContext );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_rstat( Inode_s *psInode, struct stat *psStat, bool bKernel )
{
	int nError = -EACCES;

	LOCK_INODE_RO( psInode );
	if ( NULL != psInode->i_psOperations->rstat )
	{
		memset( psStat, 0, sizeof( struct stat ) );
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->rstat( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psStat );
	}
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_wstat( Inode_s *psInode, struct stat *psStat, int nMask )
{
	int nError = -EACCES;

	LOCK_INODE_RO( psInode );
	if ( NULL != psInode->i_psOperations->wstat )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->wstat( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psStat, nMask );
	}
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_readlink( Inode_s *psInode, char *pzBuffer, int nBufLen )
{
	int nError = -EACCES;

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->readlink != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->readlink( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, pzBuffer, nBufLen );
	}
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_locate_inode( Inode_s *psParent, const char *pzName, int nNameLen, Inode_s **ppsRes, bool bFollowMount )
{
	IoContext_s *psCtx = get_current_iocxt( false );
	ino_t nInodeNum;
	int nError;

	if ( 2 == nNameLen && ( '.' == pzName[0] && '.' == pzName[1] ) && psParent->i_nInode == psParent->i_psVolume->v_nRootInode )
	{
		if ( psParent != psCtx->ic_psRootDir )
		{
			psParent = psParent->i_psVolume->v_psMountPoint;
		}
		else
		{
			atomic_inc( &psParent->i_nCount );
			*ppsRes = psParent;
			return ( 0 );
		}
	}

	LOCK_INODE_RO( psParent );

	if ( NULL == psParent->i_psOperations->locate_inode )
	{
		nError = -EACCES;
		goto error;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psParent->i_psOperations->locate_inode( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nNameLen, &nInodeNum );
	if ( nError >= 0 )
	{
		*ppsRes = get_inode( psParent->i_psVolume->v_nDevNum, nInodeNum, bFollowMount );
		if ( NULL == *ppsRes )
		{
			nError = -EIO;
		}
	}
      error:
	UNLOCK_INODE_RO( psParent );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_create( Inode_s *psParent, const char *pzName, int nNameLen, int nFlags, int nPerms, File_s *psFile )
{
	int nError = -EACCES;

	LOCK_INODE_RO( psParent );

	if ( psParent->i_psOperations->create != NULL )
	{
		ino_t nInodeNum;

		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psParent->i_psOperations->create( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nNameLen, nFlags, nPerms & ~CURRENT_THREAD->tr_nUMask, &nInodeNum, &psFile->f_pFSCookie );
		if ( nError >= 0 )
		{
			psFile->f_psInode = get_inode( psParent->i_psVolume->v_nDevNum, nInodeNum, true );
			psFile->f_nType = FDT_FILE;
			add_file_to_inode( psFile->f_psInode, psFile );
		}
	}
	UNLOCK_INODE_RO( psParent );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_symlink( Inode_s *psParent, const char *pzName, int nNameLen, const char *pzNewPath )
{
	int nError = -EPERM;

	LOCK_INODE_RO( psParent );

	if ( psParent->i_psOperations->symlink != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psParent->i_psOperations->symlink( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nNameLen, pzNewPath );
	}
	else
	{
		printk( "Error: File system %p dont support symlinks\n", psParent->i_psOperations );
	}
	UNLOCK_INODE_RO( psParent );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_ioctl( File_s *psFile, int nCmd, void *pBuffer, bool bFromKernel )
{
	Inode_s *psInode = psFile->f_psInode;
	int nError = -ENOTTY;

	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR )
	{
		return ( -EINVAL );
	}

	kassertw( NULL != psInode );

	LOCK_INODE_RO( psInode );

	if ( psInode->i_psOperations->ioctl != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->ioctl( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, nCmd, pBuffer, bFromKernel );
	}
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/**
 * \par Description:
 * Adds a select request to a currently open file.
 * 
 * The file's inode is locked in this function and is unlocked by a matching
 * call to cfs_rem_select_request().
 */
static int cfs_add_select_request( File_s *psFile, SelectRequest_s *psReq )
{
	int nError;
	Inode_s *psInode;

	kassertw( NULL != psFile );
	kassertw( NULL != psFile->f_psInode );

	if ( psFile == NULL || psReq == NULL )
		return -EINVAL;

	psInode = psFile->f_psInode;
	if ( psInode == NULL || psInode->i_psVolume == NULL )
		return -EINVAL;

	LOCK_INODE_RO( psInode );

	if ( NULL != psInode->i_psOperations->add_select_req )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->add_select_req( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, psReq );
	}
	else
	{
		if ( psReq->sr_nMode != SELECT_EXCEPT )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
//    printk( "Error: File system %x doesn't support add_select_req\n",
//            psInode->i_psOperations );
		nError = 0;
	}

    /**
     * We unlock the inode later when the select request is removed
     * (see cfs_rem_select_req) unless an error occurs or the request
     * was not queued.
     */
	if ( nError < 0 || psInode->i_psOperations->add_select_req == NULL )
	{
		UNLOCK_INODE_RO( psInode );
	}

	return ( nError );
}

/**
 * \par Description:
 * Removes a select request from a currently open file.
 * 
 * The file's inode is unlocked in this function -- it was locked by a matching
 * call to cfs_add_select_request().
 */
static int cfs_rem_select_request( File_s *psFile, SelectRequest_s *psReq )
{
	int nError = 0;
	Inode_s *psInode;

	kassertw( NULL != psFile );
	kassertw( NULL != psFile->f_psInode );

	psInode = psFile->f_psInode;

	if ( psInode->i_psOperations->rem_select_req != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->rem_select_req( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, psReq );

		UNLOCK_INODE_RO( psInode );
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cfs_truncate( Inode_s *psInode, off_t nLength )
{
	int nError = -ENOSYS;

	LOCK_INODE_RO( psInode );
	if ( NULL != psInode->i_psOperations->truncate )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->truncate( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, nLength );
	}
	UNLOCK_INODE_RO( psInode );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int follow_sym_link( Inode_s **ppsParent, Inode_s **ppsInode, struct stat *psStat )
{
	Thread_s *psThread = CURRENT_THREAD;
	Inode_s *psInode = *ppsInode;
	int nError = 0;
	char *pzLinkBuffer = NULL;
	int nNestCount = 0;
	struct stat sStatBuf;

	if ( psThread->tr_nSymLinkDepth > 5 )
	{
		return ( -ELOOP );
	}

	if ( psStat == NULL )
	{
		psStat = &sStatBuf;
		nError = cfs_rstat( psInode, psStat, true );
		if ( nError < 0 )
		{
			return ( 0 );	// FIXME: Should be nError, fix when all FS's support rstat()
		}
	}
//    atomic_inc( &psParent->i_nCount );
	psThread->tr_nSymLinkDepth++;
	while ( S_ISLNK( psStat->st_mode ) )
	{
		Inode_s *psNewParent;
		Inode_s *psNewInode;
		const char *pzName;
		int nNameLen;

		if ( nNestCount++ > MAXSYMLINKS )
		{
			nError = -ELOOP;
			break;
		}
		if ( pzLinkBuffer == NULL )
		{
			pzLinkBuffer = kmalloc( PATH_MAX, MEMF_KERNEL );
			if ( pzLinkBuffer == NULL )
			{
				nError = -ENOMEM;
				break;
			}
		}
		nError = cfs_readlink( psInode, pzLinkBuffer, PATH_MAX );
		if ( nError == PATH_MAX )
		{
			nError = -ENAMETOOLONG;
			break;
		}
		if ( nError < 0 )
		{
			break;
		}
		pzLinkBuffer[nError] = '\0';

		nError = lookup_parent_inode( *ppsParent, pzLinkBuffer, &pzName, &nNameLen, &psNewParent );

		if ( nError < 0 )
		{
			break;
		}
		if ( 0 != nNameLen && !( 1 == nNameLen && '.' == pzName[0] ) )
		{
			nError = cfs_locate_inode( psNewParent, pzName, nNameLen, &psNewInode, true );
			if ( nError < 0 )
			{
				put_inode( psNewParent );
				break;
			}
		}
		else
		{
			atomic_inc( &psNewParent->i_nCount );
			psNewInode = psNewParent;
		}
		put_inode( *ppsParent );
		put_inode( psInode );
		*ppsParent = psNewParent;
		psInode = psNewInode;

		nError = cfs_rstat( psInode, psStat, true );
		if ( nError < 0 )
		{
			nError = 0;	// FIXME: Should be removed when all FS's support rstat()
			break;
		}
	}
	psThread->tr_nSymLinkDepth--;

//    put_inode( psParent );

	if ( pzLinkBuffer != NULL )
	{
		kfree( pzLinkBuffer );
	}
	*ppsInode = psInode;
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int lookup_parent_inode( Inode_s *psParent, const char *pzPath, const char **ppzName, int *pnNameLen, Inode_s **ppsResInode )
{
	IoContext_s *psCtx = get_current_iocxt( false );
	Inode_s *psInode;
	const char *pzCurName;
	int nError = 0;
	int nLen = 0;
	int i;
	char c;

	*ppsResInode = NULL;

	LOCK( psCtx->ic_hLock );
	if ( pzPath[0] == '/' )
	{
		psParent = psCtx->ic_psRootDir;
		pzPath++;
	}
	else
	{
		if ( psParent == NULL )
		{
			psParent = psCtx->ic_psCurDir;
		}
	}

	if ( NULL != psParent )
	{
		atomic_inc( &psParent->i_nCount );
	}
	else
	{
		printk( "Error: Could not find CWD on %s\n", pzPath );
		nError = -EINVAL;
		UNLOCK( psCtx->ic_hLock );
		goto error;
	}
	UNLOCK( psCtx->ic_hLock );

	pzCurName = pzPath;

	if ( nError >= 0 )
	{
		psInode = psParent;	/*      In case the path only contain a file name       */

		for ( i = 0, nLen = 0; ( c = pzPath[i] ); ++i )
		{
			if ( '/' == c )
			{
				if ( 0 != nLen || ( 1 == nLen && c == '.' ) )
				{
					nError = cfs_locate_inode( psParent, pzCurName, nLen, &psInode, true );

					if ( 0 == nError )
					{
						nError = follow_sym_link( &psParent, &psInode, NULL );
						if ( nError < 0 )
						{
							put_inode( psInode );
						}
					}
					put_inode( psParent );

					if ( nError < 0 )
					{
						psInode = NULL;
						break;
					}

					nLen = 0;
					psParent = psInode;
				}
				else
				{
					psInode = psParent;
				}
				pzCurName = &pzPath[i + 1];
			}
			else
			{
				nLen++;
			}
		}
	}
	*ppsResInode = psInode;
	*ppzName = pzCurName;
	*pnNameLen = nLen;
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_named_inode( Inode_s *psRoot, const char *pzName, Inode_s **ppsResInode, bool bFollowMount, bool bFollowSymlink )
{
	Inode_s *psInode = NULL;
	Inode_s *psParent;
	int nLen;
	int nError;

	*ppsResInode = NULL;

	/* Check for an empty name */
	if ( pzName[0] == '\0' )
	{
		nError = -ENOENT;
		goto error1;
	}

	nError = lookup_parent_inode( psRoot, pzName, &pzName, &nLen, &psParent );

	if ( 0 != nError )
	{
		goto error1;
	}
	if ( 0 != nLen && !( 1 == nLen && '.' == pzName[0] ) )
	{
		nError = cfs_locate_inode( psParent, pzName, nLen, &psInode, bFollowMount );

		if ( nError < 0 )
		{
			goto error2;
		}
	}
	else
	{
		atomic_inc( &psParent->i_nCount );
		psInode = psParent;
	}

	if ( bFollowSymlink )
	{
		nError = follow_sym_link( &psParent, &psInode, NULL );
		if ( nError < 0 )
		{
			goto error3;
		}

	}
	put_inode( psParent );

	*ppsResInode = psInode;
	return ( 0 );
      error3:
	put_inode( psInode );
      error2:
	put_inode( psParent );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

File_s *alloc_fd( void )
{
	File_s *psFile;

	psFile = kmalloc( sizeof( File_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psFile != NULL )
	{
		atomic_inc( &g_sSysBase.ex_nOpenFileCount );
	}
	return ( psFile );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void free_fd( File_s *psFile )
{
	if ( psFile == NULL )
	{
		printk( "Error: free_fd() called with NULL pointer!\n" );
		return;
	}
	atomic_dec( &g_sSysBase.ex_nOpenFileCount );
	kfree( psFile );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

File_s *get_fd( bool bKernel, int nFile )
{
	IoContext_s *psCtx = get_current_iocxt( bKernel );
	File_s *psFile = NULL;

	LOCK( psCtx->ic_hLock );

	if ( nFile >= 0 && nFile < psCtx->ic_nCount && psCtx->ic_apsFiles[nFile] != NULL )
	{
		psFile = psCtx->ic_apsFiles[nFile];
		atomic_inc( &psFile->f_nRefCount );
	}
	UNLOCK( psCtx->ic_hLock );

	return ( psFile );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int put_fd( File_s *psFile )
{
	Inode_s *psInode = psFile->f_psInode;

	if ( !atomic_dec_and_test( &psFile->f_nRefCount ) )
	{
		return ( 0 );
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );

	LOCK_INODE_RO( psInode );
	switch ( psFile->f_nType )
	{
	case FDT_INDEX_DIR:
		if ( psInode->i_psOperations->close_indexdir != NULL )
		{
			psInode->i_psOperations->close_indexdir( psInode->i_psVolume->v_pFSData, psFile->f_pFSCookie );
		}
		break;
	case FDT_ATTR_DIR:
		if ( psInode->i_psOperations->close_attrdir != NULL )
		{
			psInode->i_psOperations->close_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
		}
		break;
	default:
		if ( psInode->i_psOperations->close != NULL )
		{
			psInode->i_psOperations->close( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
		}
		break;
	}
	remove_file_from_inode( psInode, psFile );
	UNLOCK_INODE_RO( psInode );
	put_inode( psInode );
	free_fd( psFile );

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int remove_fd( bool bKernel, int nFile )
{
	IoContext_s *psCtx = get_current_iocxt( bKernel );
	File_s *psFile = NULL;

	if ( NULL == psCtx )
	{
		panic( "remove_fd() psCtx == NULL. bKernel == %d\n", bKernel );
		return ( -EINVAL );
	}

	LOCK( psCtx->ic_hLock );

	if ( nFile >= 0 && nFile < psCtx->ic_nCount && psCtx->ic_apsFiles[nFile] != NULL )
	{
		psFile = psCtx->ic_apsFiles[nFile];
		psCtx->ic_apsFiles[nFile] = NULL;
		SETBIT( psCtx->ic_panAlloc, nFile, false );
	}

	UNLOCK( psCtx->ic_hLock );

	if ( NULL != psFile && psFile->f_psInode != NULL )
	{
		if( psFile->f_psInode->i_psFirstLock != NULL )
			unlock_inode_record( psFile->f_psInode, 0, MAX_FILE_OFFSET );
		return ( put_fd( psFile ) );
	}
	else
	{
		return ( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int new_fd( bool bKernel, int nNewFile, int nBase, File_s *psFile, bool bCloseOnExec )
{
	IoContext_s *psCtx = get_current_iocxt( bKernel );
	File_s *psOldFile = NULL;
	int nError;

	if ( NULL == psCtx )
	{
		panic( "new_fd() psCtx == NULL. bKernel == %d\n", bKernel );
		return ( -EINVAL );
	}

	LOCK( psCtx->ic_hLock );

	if ( nNewFile >= 0 )
	{
		if ( nNewFile >= psCtx->ic_nCount )
		{
			nError = -EBADF;
			goto error;
		}
		psOldFile = psCtx->ic_apsFiles[nNewFile];
	}
	else
	{
		int i;
		bool bFirst = true;

		for ( i = nBase / 32; i < psCtx->ic_nCount / 32; ++i )
		{
			if ( psCtx->ic_panAlloc[i] != ~0 )
			{
				int j = ( bFirst ) ? ( nBase % 32 ) : 0;

				bFirst = false;

				for ( ; j < 32; ++j )
				{
					if ( GETBIT( psCtx->ic_panAlloc, i * 32 + j ) == false )
					{
						nNewFile = i * 32 + j;
						break;
					}
				}
				if ( nNewFile < 0 )
				{
					printk( "PANIC: new_fd() aliens allocates file descriptors.\n" );
				}
				break;
			}
		}
	}
	if ( nNewFile < 0 )
	{
		nError = -EMFILE;
		goto error;
	}
	kassertw( nNewFile >= 0 && nNewFile < psCtx->ic_nCount );

	SETBIT( psCtx->ic_panAlloc, nNewFile, true );
	SETBIT( psCtx->ic_panCloseOnExec, nNewFile, bCloseOnExec );

	psCtx->ic_apsFiles[nNewFile] = psFile;

	atomic_inc( &psFile->f_nRefCount );
	nError = nNewFile;

      error:
	UNLOCK( psCtx->ic_hLock );

	if ( NULL != psOldFile )
	{
		put_fd( psOldFile );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_fsync( int nFile )
{
	File_s *psFile;
	int nError;

	printk( "sys_fsync( %d ) stub called!\n", nFile );

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		printk( "Error: sys_fsync() invalid file descriptor %d\n", nFile );
		return ( -EBADF );
	}

	Inode_s *psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL != psInode->i_psOperations->fsync )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->fsync( psInode->i_psVolume->v_pFSData, psInode->i_pFSData );
	}
	else
	{
		nError = ( -ENOSYS );
	}
	UNLOCK_INODE_RO( psInode );

	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_truncate( const char *a_pzPath, uint32 nLenLow, uint32 nLenHigh )
{
	Inode_s *psInode;
	char *pzPath;
	int nError;

	off_t nLength = ((off_t)nLenHigh << 32 ) | nLenLow;

	//printk( "sys_truncate( %s, %Ld ) stub called!\n", a_pzPath, nLength );

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_truncate() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );

	if ( nError < 0 )
	{
		goto error1;
	}

	nError = cfs_truncate( psInode, nLength );

	put_inode( psInode );
      error1:
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_ftruncate( int nFile, uint32 nLenLow, uint32 nLenHigh )
{
	File_s *psFile;
	int nError;

	off_t nLength = ((off_t)nLenHigh << 32 ) | nLenLow;

	//printk( "sys_ftruncate( %d, %Ld ) stub called!\n", nFile, nLength );

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		printk( "Error: sys_ftruncate() invalid file descriptor %d\n", nFile );
		return ( -EBADF );
	}

	nError = cfs_truncate( psFile->f_psInode, nLength );

	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_umask( int nMask )
{
	return ( atomic_swap( &CURRENT_THREAD->tr_nUMask, nMask & S_IRWXUGO ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int open_inode( bool bKernel, Inode_s *psInode, int nType, int nMode )
{
	File_s *psFile;
	int nError;

	psFile = alloc_fd();
	if ( psFile == NULL )
	{
		return ( -ENOMEM );
	}

	psFile->f_nMode = nMode & ~( O_OPEN_CONTROL_FLAGS );
	psFile->f_psInode = psInode;
	psFile->f_nType = nType;

	if ( psInode->i_psOperations->open != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->open( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, nMode, &psFile->f_pFSCookie );
		if ( nError < 0 )
		{
			goto error1;
		}
	}

	nError = new_fd( bKernel, -1, 0, psFile, false );
	if ( nError < 0 )
	{
		goto error2;
	}
	atomic_inc( &psInode->i_nCount );
	add_file_to_inode( psInode, psFile );
	return ( nError );
      error2:
	if ( psInode->i_psOperations->close != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		psInode->i_psOperations->close( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
	}
      error1:
	free_fd( psFile );
	return ( nError );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static int open_file( Inode_s **ppsParent, const char *pzName, int nNameLen, int nFlags, File_s *psFile )
{
	int nError = 0;

	if ( 0 == nNameLen || ( 1 == nNameLen && '.' == pzName[0] ) )
	{
		LOCK_INODE_RO( ( *ppsParent ) );

		if ( ( *ppsParent )->i_psOperations->open != NULL )
		{
			kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
			nError = ( *ppsParent )->i_psOperations->open( ( *ppsParent )->i_psVolume->v_pFSData, ( *ppsParent )->i_pFSData, nFlags, &psFile->f_pFSCookie );
		}
		if ( nError >= 0 )
		{
			add_file_to_inode( *ppsParent, psFile );
			atomic_inc( &( *ppsParent )->i_nCount );
			psFile->f_psInode = *ppsParent;
			psFile->f_nType = FDT_DIR;
		}
		UNLOCK_INODE_RO( ( *ppsParent ) );
		return ( nError );
	}
	else
	{
		Inode_s *psInode;
		struct stat sStat;

		nError = cfs_locate_inode( *ppsParent, pzName, nNameLen, &psInode, true );

		if ( nError < 0 )
		{
			return ( nError );
		}

		nError = cfs_rstat( psInode, &sStat, true );

		if ( nError < 0 )
		{
			put_inode( psInode );
			return ( nError );
		}
		if ( S_ISLNK( sStat.st_mode ) )
		{
			if ( ( nFlags & O_NOTRAVERSE ) == 0 )
			{
				nError = follow_sym_link( ppsParent, &psInode, &sStat );

				if ( nError < 0 )
				{
					put_inode( psInode );
					return ( nError );
				}
			}
		}
		if ( ( nFlags & O_DIRECTORY ) && S_ISDIR( sStat.st_mode ) == false )
		{
			put_inode( psInode );
			return ( -ENOTDIR );
		}
		if ( sStat.st_dev == FSID_DEV && sStat.st_ino == DEVFS_DEVTTY )
		{
			Inode_s *psTTY = get_ctty();

			put_inode( psInode );
			if ( psTTY == NULL )
			{
				return ( -ENOTTY );
			}
			psInode = psTTY;
		}

		if ( S_ISFIFO( sStat.st_mode ) )
		{
			Inode_s *psPipe = create_fifo_inode( psInode );

			put_inode( psInode );
			if ( psPipe == NULL )
			{
				return ( -ENOMEM );
			}
			psInode = psPipe;
		}
		LOCK_INODE_RO( psInode );

		if ( psInode->i_psOperations->open != NULL )
		{
			kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
			nError = psInode->i_psOperations->open( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, nFlags, &psFile->f_pFSCookie );
		}
		if ( nError >= 0 )
		{
			add_file_to_inode( psInode, psFile );
			psFile->f_psInode = psInode;
			if ( S_ISDIR( sStat.st_mode ) )
			{
				psFile->f_nType = FDT_DIR;
			}
			else if ( S_ISLNK( sStat.st_mode ) )
			{
				psFile->f_nType = FDT_SYMLINK;
			}
			else if ( S_ISFIFO( sStat.st_mode ) )
			{
				psFile->f_nType = FDT_FIFO;
			}
			else
			{
				psFile->f_nType = FDT_FILE;
			}
		}
		UNLOCK_INODE_RO( psInode );

		if ( nError < 0 )
		{
			put_inode( psInode );
		}
		return ( nError );
	}
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int extended_open( bool bKernel, Inode_s *psRoot, const char *pzPath, Inode_s **ppsParent, int nFlags, int nPerms )
{
	Inode_s *psParent;
	File_s *psFile = NULL;
	int nLen;
	int nFile;
	int nError;
	const char *pzName;

	nError = lookup_parent_inode( psRoot, pzPath, &pzName, &nLen, &psParent );

	if ( nError < 0 )
	{
		goto error1;
	}

	psFile = alloc_fd();

	if ( psFile == NULL )
	{
		nError = -ENOMEM;
		printk( "Error: extended_open() out of memory\n" );
		goto error2;
	}

	if ( nFlags & O_CREAT )
	{
		nError = open_file( &psParent, pzName, nLen, nFlags, psFile );
		if ( nError < 0 )
		{
			nError = cfs_create( psParent, pzName, nLen, nFlags, nPerms, psFile );
		}
	}
	else
	{
		nError = open_file( &psParent, pzName, nLen, nFlags, psFile );
	}
	if ( nError < 0 )
	{
		goto error3;
	}
	psFile->f_nMode = nFlags & ~( O_OPEN_CONTROL_FLAGS );

	nFile = new_fd( bKernel, -1, 0, psFile, false );

	if ( nFile < 0 )
	{
		printk( "Error: extended_open() no free file descriptors\n" );
		nError = nFile;
		atomic_set( &psFile->f_nRefCount, 1 );
		put_fd( psFile );
		goto error2;
	}
	if ( ppsParent != NULL )
	{
		*ppsParent = psParent;
	}
	else
	{
		put_inode( psParent );
	}
	return ( nFile );
      error3:
	free_fd( psFile );
      error2:
	put_inode( psParent );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_open( const char *a_pzPath, int nFlags, ... )
{
	char *pzPath;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_open() failed to dup path\n" );
		return ( nError );
	}
	nError = extended_open( false, NULL, pzPath, NULL, nFlags, ( ( int * )&nFlags )[1] );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int open( const char *pzPath, int nFlags, ... )
{
	return ( extended_open( true, NULL, pzPath, NULL, nFlags, ( ( int * )&nFlags )[1] ) );
}

/** Open a file relative to a given directory.
 * \ingroup DriverAPI
 * \par Description:
 *	based_open() have the same semantics as open() except that you
 *	can specify the "current working directory" as a parameter.
 *
 *	The first parameter should be a file descriptor for a open
 *	directory and the path can be eighter absolute (starting with a "/")
 *	in which case the semantics is exactly the same as for open()
 *	or it can be relative to \p nRootFD.
 *
 *	The "real" current working directory is not affected.
 *
 *	This provide a thread-safe way to do do the following:
 *
 *	\code
 *		int nOldDir = open( ".", O_RDONLY );
 *		fchdir( nRootFD );
 *		nFile = open( pzPath, ... );
 *		fchdir( nOldDir );
 *		close( nOldDir );
 *	\endcode
 *
 *	The problem with the above example in a multithreaded environment
 *	is that other threads will be affected by the change of
 *	current working directory and might get into trouble while
 *	we temporarily change it. Using based_open() is also more
 *	efficient since only one kernel-call is done (based_open() is
 *	just as efficient as open()).
 *
 * \param nRootFD
 *	The directory to use as starting point when searching for
 *	\p pzPath
 * \param pzPath
 *	Path to the file to open. This can eighter be absolute or relative
 *	to \p nRootFD.
 * \param nFlags
 *	Flags controlling how to open the file. Look at open() for a
 *	full description of the flags.
 * \param nMode
 *	The access rights to set on the newly created file if O_CREAT
 *	is specified in \p nFlags
 *
 * \return
 *	based_open() return the new file descriptor on success or
 * \if document_driver_api
 *	a negatice error code if an error occurs.
 * \endif
 * \if document_syscalls
 *	-1 if an error occurs, placing the error code in the global
 *	variable errno.
 * \endif
 * \par Error codes:
 *	- \b EMFILE \p nRootFD is not a valid file descriptor
 *	- \b ENOTDIR \p nRootFD is not a directory
 *	- In addition all error-codes returned by open() can be returned.
 *
 * \sa open(), close()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int based_open( int nRootFD, const char *pzPath, int nFlags, ... )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nRootFD );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = extended_open( false, psFile->f_psInode, pzPath, NULL, nFlags, ( ( int * )&nFlags )[1] );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_based_open( int nRootFD, const char *a_pzPath, int nFlags, ... )
{
	File_s *psFile;
	char *pzPath;
	int nError;

	psFile = get_fd( false, nRootFD );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError >= 0 )
	{
		nError = extended_open( false, psFile->f_psInode, pzPath, NULL, nFlags, ( ( int * )&nFlags )[1] );
		kfree( pzPath );
	}
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_open_image_file( bool bKernel, int nImageID )
{
	Process_s *psProc = CURRENT_PROC;
	int nFile;

	if ( nImageID == IMGFILE_BIN_DIR )
	{
		if ( psProc->pr_psIoContext->ic_psBinDir != NULL )
		{
			LOCK_INODE_RO( psProc->pr_psIoContext->ic_psBinDir );
			nFile = open_inode( bKernel, psProc->pr_psIoContext->ic_psBinDir, FDT_DIR, O_RDONLY );
			UNLOCK_INODE_RO( psProc->pr_psIoContext->ic_psBinDir );
		}
		else
		{
			nFile = -ENOENT;
		}
	}
	else
	{
		Inode_s *psInode = get_image_inode( nImageID );

		if ( psInode != NULL )
		{
			LOCK_INODE_RO( psInode );
			nFile = open_inode( bKernel, psInode, FDT_FILE, O_RDONLY );
			UNLOCK_INODE_RO( psInode );
			put_inode( psInode );
		}
		else
		{
			nFile = -ENOENT;
		}
	}
	return ( nFile );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int open_image_file( int nImageID )
{
	return ( do_open_image_file( true, nImageID ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_open_image_file( int nImageID )
{
	return ( do_open_image_file( false, nImageID ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int copy_fd( bool bKernel, File_s *psFile )
{
	int nFile;

	LOCK_INODE_RO( psFile->f_psInode );
	nFile = open_inode( bKernel, psFile->f_psInode, psFile->f_nType, psFile->f_nMode );
	UNLOCK_INODE_RO( psFile->f_psInode );

	if ( nFile >= 0 )
	{
		off_t nOrigPos = psFile->f_nPos;

		psFile = get_fd( bKernel, nFile );
		if ( psFile != NULL )
		{
			psFile->f_nPos = nOrigPos;
			put_fd( psFile );
		}
	}
	return ( nFile );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static int create_pipe( bool bKernel, int *pnFiles )
{
	Inode_s *psInode;
	int nRdFile;
	int nWrFile;
	int nError;

	psInode = create_fifo_inode( NULL );
	if ( psInode == NULL )
	{
		return ( -ENOMEM );
	}
	LOCK_INODE_RO( psInode );
	nRdFile = open_inode( bKernel, psInode, FDT_FIFO, O_RDONLY );
	if ( nRdFile < 0 )
	{
		nError = nRdFile;
		goto error1;
	}
	nWrFile = open_inode( bKernel, psInode, FDT_FIFO, O_WRONLY );
	if ( nWrFile < 0 )
	{
		nError = nWrFile;
		goto error2;
	}
	UNLOCK_INODE_RO( psInode );

	put_inode( psInode );

	pnFiles[0] = nRdFile;
	pnFiles[1] = nWrFile;
	return ( 0 );
      error2:
	remove_fd( bKernel, nRdFile );
      error1:
	UNLOCK_INODE_RO( psInode );
	put_inode( psInode );
	return ( nError );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int sys_pipe( int *pnFiles )
{
	int anFiles[2];
	int nError;

	nError = create_pipe( false, anFiles );
	if ( nError < 0 )
	{
		return ( nError );
	}
	if ( memcpy_to_user( pnFiles, anFiles, sizeof( int ) * 2 ) < 0 )
	{
		sys_close( anFiles[0] );
		sys_close( anFiles[1] );
		return ( -EFAULT );
	}
	return ( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int pipe( int *pnFiles )
{
	return ( create_pipe( true, pnFiles ) );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_close( int nFile )
{
	return ( remove_fd( false, nFile ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int close( int nFile )
{
	return ( remove_fd( true, nFile ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

IoContext_s *fs_alloc_ioctx( int nCount )
{
	IoContext_s *psCtx;
	const int nByteSize = sizeof( IoContext_s ) + 2 * BITSIZE( nCount ) / 8 + sizeof( File_s * ) * nCount;

	psCtx = kmalloc( nByteSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psCtx == NULL )
	{
		printk( "Error: fs_alloc_ioctx() out of memory\n" );
		return ( NULL );
	}

	psCtx->ic_hLock = create_semaphore( "io_ctx", 1, SEM_RECURSIVE );

	if ( psCtx->ic_hLock < 0 )
	{
		printk( "Error: fs_alloc_ioctx() failed to create semaphore\n" );
		kfree( psCtx );
		return ( NULL );
	}

	psCtx->ic_psCurDir = NULL;
	psCtx->ic_psBinDir = NULL;
	psCtx->ic_psCtrlTTY = NULL;
	psCtx->ic_nCount = nCount;
	psCtx->ic_panAlloc = ( uint32 * )( ( ( char * )psCtx ) + sizeof( IoContext_s ) );
	psCtx->ic_panCloseOnExec = ( uint32 * )( ( ( char * )psCtx ) + sizeof( IoContext_s ) + BITSIZE( nCount ) / 8 );
	psCtx->ic_apsFiles = ( File_s ** )( ( ( char * )psCtx ) + sizeof( IoContext_s ) + BITSIZE( nCount ) * 2 / 8 );
	return ( psCtx );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

IoContext_s *fs_clone_io_context( IoContext_s * psSrc )
{
	IoContext_s *psCtx;
	int i;

	psCtx = fs_alloc_ioctx( psSrc->ic_nCount );

	if ( NULL == psCtx )
	{
		printk( "Error: fs_clone_io_context() out of memory\n" );
		return ( NULL );
	}
	memcpy( psCtx->ic_panAlloc, psSrc->ic_panAlloc, BITSIZE( psSrc->ic_nCount ) / 8 );
	memcpy( psCtx->ic_panCloseOnExec, psSrc->ic_panCloseOnExec, BITSIZE( psSrc->ic_nCount ) / 8 );
	memcpy( psCtx->ic_apsFiles, psSrc->ic_apsFiles, sizeof( File_s * ) * psSrc->ic_nCount );

	for ( i = 0; i < psCtx->ic_nCount; ++i )
	{
		if ( NULL != psCtx->ic_apsFiles[i] )
		{
			atomic_inc( &psCtx->ic_apsFiles[i]->f_nRefCount );
		}
	}
	psCtx->ic_psCurDir = psSrc->ic_psCurDir;
	if ( psCtx->ic_psCurDir != NULL )
	{
		atomic_inc( &psCtx->ic_psCurDir->i_nCount );
	}
	psCtx->ic_psBinDir = psSrc->ic_psBinDir;
	if ( psCtx->ic_psBinDir != NULL )
	{
		atomic_inc( &psCtx->ic_psBinDir->i_nCount );
	}
	clone_ctty( psCtx, psSrc );
	psCtx->ic_psRootDir = psSrc->ic_psRootDir;
	if ( psCtx->ic_psRootDir != NULL )
	{
		atomic_inc( &psCtx->ic_psRootDir->i_nCount );
	}
	return ( psCtx );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void fs_free_ioctx( IoContext_s * psCtx )
{
	int i;

	nwatch_exit( psCtx );

	for ( i = 0; i < psCtx->ic_nCount; ++i )
	{
		if ( psCtx->ic_apsFiles[i] != NULL )
		{
			put_fd( psCtx->ic_apsFiles[i] );
		}
	}
	if ( psCtx->ic_psCurDir != NULL )
	{
		put_inode( psCtx->ic_psCurDir );
	}
	if ( psCtx->ic_psBinDir != NULL )
	{
		put_inode( psCtx->ic_psBinDir );
	}
	if ( psCtx->ic_psRootDir != NULL )
	{
		put_inode( psCtx->ic_psRootDir );
	}
	if ( psCtx->ic_psCtrlTTY != NULL )
	{
		put_inode( psCtx->ic_psCtrlTTY );
	}
	delete_semaphore( psCtx->ic_hLock );
	kfree( psCtx );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_symlink( Inode_s *psRoot, const char *pzSrc, const char *pzDst )
{
	Inode_s *psParent = NULL;
	int nLen;
	int nError;
	const char *pzDstName;

	nError = lookup_parent_inode( psRoot, pzDst, &pzDstName, &nLen, &psParent );

	if ( nError < 0 )
	{
		goto error;
	}
	nError = -EINVAL;

	if ( 0 != nLen )
	{
		nError = cfs_symlink( psParent, pzDstName, nLen, pzSrc );
	}
	put_inode( psParent );
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int symlink( const char *pzSrc, const char *pzDst )
{
	return ( do_symlink( NULL, pzSrc, pzDst ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int based_symlink( int nDir, const char *pzSrc, const char *pzDst )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nDir );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = do_symlink( psFile->f_psInode, pzSrc, pzDst );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_symlink( const char *a_pzSrc, const char *a_pzDst )
{
	char *pzSrc = NULL;
	char *pzDst = NULL;
	int nError;

	nError = strndup_from_user( a_pzSrc, PATH_MAX, &pzSrc );
	if ( nError < 0 )
	{
		printk( "Error: sys_symlink() failed to dup source path\n" );
		goto error;
	}
	nError = strndup_from_user( a_pzDst, PATH_MAX, &pzDst );

	if ( nError < 0 )
	{
		printk( "Error: sys_symlink() failed to dup dest path\n" );
		goto error;
	}
	nError = do_symlink( NULL, pzSrc, pzDst );
      error:
	kfree( pzSrc );
	kfree( pzDst );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_based_symlink( int nDir, const char *a_pzSrc, const char *a_pzDst )
{
	char *pzSrc = NULL;
	char *pzDst = NULL;
	File_s *psFile;
	int nError;

	psFile = get_fd( false, nDir );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = strndup_from_user( a_pzSrc, PATH_MAX, &pzSrc );
	if ( nError < 0 )
	{
		printk( "Error: sys_based_symlink() failed to dup source path\n" );
		goto error;
	}
	nError = strndup_from_user( a_pzDst, PATH_MAX, &pzDst );

	if ( nError < 0 )
	{
		printk( "Error: sys_based_symlink() failed to dup dest path\n" );
		goto error;
	}
	nError = do_symlink( psFile->f_psInode, pzSrc, pzDst );
      error:
	kfree( pzSrc );
	kfree( pzDst );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int do_freadlink( bool bKernel, int nFile, char *pzBuffer, size_t nBufSize )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( psFile->f_nType == FDT_SYMLINK )
	{
		nError = cfs_readlink( psFile->f_psInode, pzBuffer, nBufSize );
	}
	else
	{
		nError = -EINVAL;
	}
	put_fd( psFile );
	return ( nError );
}

/** Read the content of an previously opened symlink.
 * \ingroup DriverAPI
 * \par Description:
 *	Read the content of a symlink previously opened with
 *	open( path, O_NOTRAVERSE ).
 *
 *	The semantics is the same as for readlink() except that it take
 *	and already opened symlink rather than a path-name.
 *
 *	Normally when passing a symlink path to open() it will
 *	automatically open the file/dir that the link points to and
 *	not the link itself. To stop open() from traversing the links
 *	you can add the O_NOTRAVERSE flag to the open-mode. open()
 *	will then return a file-handle representing the link itself
 *	rather than the file the link points at.
 *
 * \param nFile
 *	File descriptor representing the symlink to read.
 * \param pzBuffer
 *	Pointer to a buffer of at least \p nBufSize bytes that will
 *	receive the content of the symlink. The buffer will not
 *	be NUL terminated but the number of bytes added to the
 *	buffer will be returned by freadlink().
 * \param nBufSize
 *	Size of pzBuffer.
 * \return
 *	The call returns the number of bytes placed in the buffers on
 *	success, or
 * \if document_driver_api
 *	a negatice error code if an error occurs.
 * \endif
 * \if document_syscalls
 *	-1 if an error occurs, placing the error code in the global
 *	variable errno.
 * \endif
 *
 * \par Error codes:
 *	- \b EBADF \p nFile is not a valid file descriptor or is not
 *		      open for reading
 *	- \b EINVAL \p nFile is not a symlink
 *	- \b ENOSYS The filesystem hosting \p nFile does not support
 *		      symbolic links.
 *
 * \sa readlink()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


int freadlink( int nFile, char *pzBuffer, size_t nBufSize )
{
	return ( do_freadlink( true, nFile, pzBuffer, nBufSize ) );
}

int sys_freadlink( int nFile, char *pzBuffer, size_t nBufSize )
{
	int nError;

	if ( verify_mem_area( pzBuffer, nBufSize, true ) < 0 )
	{
		return ( -EFAULT );
	}
	nError = do_freadlink( false, nFile, pzBuffer, nBufSize );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int readlink( const char *pzPath, char *pzBuffer, size_t nBufSize )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzPath, &psInode, true, false );

	if ( nError < 0 )
	{
		return ( nError );
	}
	nError = cfs_readlink( psInode, pzBuffer, nBufSize );
	put_inode( psInode );
	return ( nError );
}

int sys_readlink( const char *a_pzPath, char *pzBuffer, size_t nBufSize )
{
	char *pzPath;
	int nError;

	if ( verify_mem_area( pzBuffer, nBufSize, true ) < 0 )
	{
		return ( -EFAULT );
	}
	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_readlink() failed to dup source path\n" );
		return ( nError );
	}
	nError = readlink( pzPath, pzBuffer, nBufSize );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * 	Some operations need to remove trailing slashes for POSIX.1
 * 	conformance. For rename we also need to change the behaviour
 * 	depending on whether we had a trailing slash or not.. (we
 * 	cannot rename normal files with trailing slashes, only dirs)
 *
 * 	"dummy" is used to make sure we don't do "/" -> "".
 *
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool remove_trailing_slashes( char *pzName )
{
	int nError;
	char nChar[1];
	char *pzRemove = nChar + 1;

	for ( ;; )
	{
		char c = *pzName;

		pzName++;
		if ( c == 0 )
		{
			break;
		}
		if ( c != '/' )
		{
			pzRemove = NULL;
			continue;
		}
		if ( pzRemove != NULL )
		{
			continue;
		}
		pzRemove = pzName;
	}

	nError = 0;
	if ( pzRemove != NULL )
	{
		pzRemove[-1] = 0;
		nError = 1;
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_rename( const char *a_pzOldName, const char *a_pzNewName )
{
	Inode_s *psOldDir;
	Inode_s *psNewDir;

	int nOldLen;
	int nNewLen;

	char *pzOldPath;
	char *pzNewPath;

	const char *pzOldName;
	const char *pzNewName;

	bool bMustBeDir;
	int nError;

	nError = strndup_from_user( a_pzNewName, PATH_MAX, &pzNewPath );
	if ( nError < 0 )
	{
		goto error1;
	}
	nError = strndup_from_user( a_pzOldName, PATH_MAX, &pzOldPath );
	if ( nError < 0 )
	{
		goto error2;
	}

	bMustBeDir = remove_trailing_slashes( pzNewPath );
	bMustBeDir = remove_trailing_slashes( pzOldPath ) || bMustBeDir;

	nError = lookup_parent_inode( NULL, pzOldPath, &pzOldName, &nOldLen, &psOldDir );

	if ( nError < 0 )
	{
		goto error3;
	}

	nError = lookup_parent_inode( NULL, pzNewPath, &pzNewName, &nNewLen, &psNewDir );

	if ( nError < 0 )
	{
		goto error4;
	}
	if ( psNewDir->i_psVolume->v_nDevNum != psOldDir->i_psVolume->v_nDevNum )
	{
		nError = -EXDEV;
		goto error5;
	}

	LOCK_INODE_RO( psOldDir );

	if ( psOldDir->i_psOperations->rename == NULL )
	{
		nError = -EINVAL;
		goto error6;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psOldDir->i_psOperations->rename( psOldDir->i_psVolume->v_pFSData, psOldDir->i_pFSData, pzOldName, nOldLen, psNewDir->i_pFSData, pzNewName, nNewLen, bMustBeDir );
      error6:
	UNLOCK_INODE_RO( psOldDir );
      error5:
	put_inode( psNewDir );
      error4:
	put_inode( psOldDir );
      error3:
	kfree( pzNewPath );
      error2:
	kfree( pzOldPath );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int lock_iovec_buffers( const struct iovec *psVector, size_t nCount, bool bWrite )
{
	int i;

	if ( nCount <= 0 )
	{
		return ( 0 );
	}
	if ( verify_mem_area( psVector, sizeof( struct iovec ) * nCount, false ) < 0 )
	{
		return ( -EFAULT );
	}
	for ( i = 0; i < nCount; ++i )
	{
		if ( psVector[i].iov_len == 0 )
		{
			continue;
		}
		if ( verify_mem_area( psVector[i].iov_base, psVector[i].iov_len, bWrite ) < 0 )
		{
			return ( -EFAULT );
		}
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void unlock_iovec_buffers( const struct iovec *psVector, size_t nCount )
{
	if ( nCount <= 0 )
	{
		return;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t read_pos_p( File_s *psFile, off_t nPos, void *pBuffer, size_t nLength )
{
	Inode_s *psInode;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	if ( NULL == psFile )
	{
		printk( "PANIC: read_pos_p() called with psFile == NULL!!!\n" );
		return ( -EBADF );
	}

	if ( psFile->f_nType == FDT_DIR )
	{
		return ( -EISDIR );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR || psFile->f_nType == FDT_SYMLINK )
	{
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );

	if ( psInode->i_psOperations->read == NULL )
	{
		nError = -EINVAL;
		goto error;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->read( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, nPos, pBuffer, nLength );
	if ( nError < 0 )
	{
		goto error;
	}
      error:
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t readv_pos_p( File_s *psFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	Inode_s *psInode;
	int nError;

	if ( NULL == psFile )
	{
		printk( "PANIC: readv_pos_p() called with psFile == NULL!!!\n" );
		return ( -EBADF );
	}

	if ( psFile->f_nType == FDT_DIR )
	{
		return ( -EISDIR );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR || psFile->f_nType == FDT_SYMLINK )
	{
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );

	if ( psInode->i_psOperations->readv == NULL )
	{
		if ( psInode->i_psOperations->read == NULL )
		{
			nError = -EINVAL;
			goto error;
		}
		else
		{
			int i;

			nError = 0;
			for ( i = 0; i < nCount; ++i )
			{
				ssize_t nCurSize = psInode->i_psOperations->read( psInode->i_psVolume->v_pFSData, psInode->i_pFSData,
					psFile->f_pFSCookie, nPos + nError,
					psVector[i].iov_base, psVector[i].iov_len );

				if ( nCurSize < 0 )
				{
					nError = nCurSize;
					goto error;
				}
				nError += nCurSize;
				if ( nCurSize != psVector[i].iov_len )
				{
					break;
				}
			}
		}
	}
	else
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->readv( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, nPos, psVector, nCount );
	}
      error:
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t read_p( File_s *psFile, void *pBuffer, size_t nLength )
{
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	if ( NULL == psFile )
	{
		printk( "Panic: read_pos_p() called with psFile == NULL!!!\n" );
		return ( -EBADF );
	}

	nError = read_pos_p( psFile, psFile->f_nPos, pBuffer, nLength );

	if ( nError < 0 )
	{
		goto error;
	}

	psFile->f_nPos += nError;
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t read_pos( int nFile, off_t nPos, void *pBuffer, size_t nLength )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = read_pos_p( psFile, nPos, pBuffer, nLength );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_read_pos( int nFile, off_t nPos, void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( nLength != 0 )
	{
		if ( verify_mem_area( pBuffer, nLength, true ) < 0 )
		{
			printk( "Error: sys_read_pos() failed to lock buffer\n" );
			nError = -EFAULT;
			goto error;
		}
	}

	nError = read_pos_p( psFile, nPos, pBuffer, nLength );
      error:
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t read( int nFile, void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}
	psFile = get_fd( true, nFile );
	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = read_p( psFile, pBuffer, nLength );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_read( int nFile, void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( nLength != 0 )
	{
		if ( verify_mem_area( pBuffer, nLength, true ) < 0 )
		{
			printk( "Error: sys_read() failed to lock buffer\n" );
			nError = -EFAULT;
			goto error;
		}
	}

	nError = read_p( psFile, pBuffer, nLength );
      error:
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t readv( int nFile, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = readv_pos_p( psFile, psFile->f_nPos, psVector, nCount );
	put_fd( psFile );

	if ( nError > 0 )
	{
		psFile->f_nPos += nError;
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t readv_pos( int nFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = readv_pos_p( psFile, nPos, psVector, nCount );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_readv_pos( int nFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if ( lock_iovec_buffers( psVector, nCount, true ) < 0 )
	{
		nError = -EFAULT;
		goto error;
	}

	nError = readv_pos_p( psFile, nPos, psVector, nCount );

	if ( nError > 0 )
	{
		nPos += nError;
	}
	unlock_iovec_buffers( psVector, nCount );
      error:
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_readv( int nFile, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if ( lock_iovec_buffers( psVector, nCount, true ) < 0 )
	{
		nError = -EFAULT;
		goto error;
	}

	nError = readv_pos_p( psFile, psFile->f_nPos, psVector, nCount );

	if ( nError > 0 )
	{
		psFile->f_nPos += nError;
	}
	unlock_iovec_buffers( psVector, nCount );
      error:
	put_fd( psFile );
	return ( nError );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t write_pos_p( File_s *psFile, off_t nPos, const void *pBuffer, size_t nLength )
{
	Inode_s *psInode;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	if ( NULL == psFile )
	{
		printk( "PANIC: write_pos_p() called with psFile == NULL\n" );
		return ( -EBADF );
	}

	if ( psFile->f_nType == FDT_DIR )
	{
		return ( -EISDIR );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR || psFile->f_nType == FDT_SYMLINK )
	{
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->write == NULL )
	{
		nError = -EINVAL;
		goto error;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->write( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, nPos, pBuffer, nLength );
      error:
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t write_pos( int nFile, off_t nPos, const void *pBuffer, size_t nLength )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = write_pos_p( psFile, nPos, pBuffer, nLength );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t write_p( File_s *psFile, const void *pBuffer, size_t nLength )
{
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	if ( NULL == psFile )
	{
		printk( "PANIC: write_p() called with psFile == NULL\n" );
		return ( -EBADF );
	}

	nError = write_pos_p( psFile, psFile->f_nPos, pBuffer, nLength );

	if ( nError < 0 )
	{
		goto error;
	}

	if ( ( psFile->f_nMode & O_APPEND ) == 0 )
	{
		psFile->f_nPos += nError;
	}
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_write( int nFile, const void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( nLength != 0 )
	{
		if ( verify_mem_area( pBuffer, nLength, false ) < 0 )
		{
			printk( "Error: sys_write() failed to lock buffer\n" );
			nError = -EFAULT;
			goto error;
		}
	}

	nError = write_p( psFile, pBuffer, nLength );

      error:
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_write_pos( int nFile, off_t nPos, const void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( nLength != 0 )
	{
		if ( verify_mem_area( pBuffer, nLength, false ) < 0 )
		{
			printk( "Error: sys_write_pos() failed to lock buffer\n" );
			nError = -EFAULT;
			goto error;
		}
	}

	nError = write_pos_p( psFile, nPos, pBuffer, nLength );

      error:
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t write( int nFile, const void *pBuffer, size_t nLength )
{
	File_s *psFile;
	int nError;

	if ( nLength < 0 )
	{
		return ( -EINVAL );
	}

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = write_p( psFile, pBuffer, nLength );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t writev_pos_p( File_s *psFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	Inode_s *psInode;
	int nError;

	if ( NULL == psFile )
	{
		printk( "PANIC: writev_pos_p() called with psFile == NULL\n" );
		return ( -EBADF );
	}

	if ( psFile->f_nType == FDT_DIR )
	{
		return ( -EISDIR );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR || psFile->f_nType == FDT_SYMLINK )
	{
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->writev == NULL )
	{
		if ( psInode->i_psOperations->write == NULL )
		{
			nError = -EINVAL;
			goto error;
		}
		else
		{
			int i;

			nError = 0;
			for ( i = 0; i < nCount; ++i )
			{
				ssize_t nCurSize = psInode->i_psOperations->write( psInode->i_psVolume->v_pFSData, psInode->i_pFSData,
					psFile->f_pFSCookie, nPos + nError,
					psVector[i].iov_base, psVector[i].iov_len );

				if ( nCurSize < 0 )
				{
					nError = nCurSize;
					goto error;
				}
				nError += nCurSize;
				if ( nCurSize != psVector[i].iov_len )
				{
					break;
				}
			}
		}
	}
	else
	{
		nError = psInode->i_psOperations->writev( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, nPos, psVector, nCount );
	}
      error:
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t writev_pos( int nFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = writev_pos_p( psFile, nPos, psVector, nCount );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t writev( int nFile, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( true, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = writev_pos_p( psFile, psFile->f_nPos, psVector, nCount );
	put_fd( psFile );

	if ( nError > 0 )
	{
		psFile->f_nPos += nError;
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_writev( int nFile, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if ( lock_iovec_buffers( psVector, nCount, false ) < 0 )
	{
		nError = -EFAULT;
		goto error;
	}
	nError = writev_pos_p( psFile, psFile->f_nPos, psVector, nCount );
	if ( nError > 0 )
	{
		psFile->f_nPos += nError;
	}
	unlock_iovec_buffers( psVector, nCount );
      error:
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_writev_pos( int nFile, off_t nPos, const struct iovec *psVector, int nCount )
{
	File_s *psFile;
	ssize_t nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( lock_iovec_buffers( psVector, nCount, false ) < 0 )
	{
		nError = -EFAULT;
		goto error;
	}
	nError = writev_pos_p( psFile, nPos, psVector, nCount );

	unlock_iovec_buffers( psVector, nCount );
      error:
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_getdents( bool bKernel, int nFile, struct kernel_dirent *psDirEnt, int nBufSize )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;
	struct kernel_dirent sEntry;

	if ( NULL == psDirEnt )
	{
		return ( -EINVAL );
	}

	memset( &sEntry, 0, sizeof( sEntry ) );

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -ENOENT );
	}


	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );

	if ( psFile->f_nType != FDT_DIR )
	{
		nError = -ENOTDIR;
		goto error;
	}

	if ( NULL == psInode->i_psOperations->readdir )
	{
		printk( "Error: sys_readdir() Filesystem %p does not support readdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->readdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, psFile->f_nPos, &sEntry, sizeof( struct kernel_dirent ) );
	if ( psInode->i_nInode == psInode->i_psVolume->v_nRootInode )
	{
		if ( sEntry.d_namlen == 2 && sEntry.d_name[0] == '.' && sEntry.d_name[1] == '.' )
		{
			sEntry.d_ino = psInode->i_psVolume->v_psMountPoint->i_nInode;
		}
	}

	if ( nError > 0 )
	{
		psFile->f_nPos += nError;
	}
      error:
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );


	if ( nBufSize < sizeof( sEntry ) )
	{
		memcpy( psDirEnt, &sEntry, 1 + 255 + 8 );
	}
	else
	{
		if ( nBufSize != sizeof( sEntry ) )
		{
			printk( "do_getdents() invalid size %d (sizeof(kernel_dirent)=%d\n", nBufSize, sizeof( sEntry ) );
		}
		memcpy( psDirEnt, &sEntry, nBufSize );
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_getdents( int nFile, struct kernel_dirent *psDirEnt, int nBufSize )
{
	int nError;

	nError = verify_mem_area( psDirEnt, nBufSize, true );
	if ( nError < 0 )
	{
		printk( "sys_getdents() failed to lock destination buffer (%d)\n", nBufSize );
		return ( -EFAULT );
	}
	nError = do_getdents( false, nFile, psDirEnt, nBufSize );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int getdents( int nFile, struct kernel_dirent *psDirEnt, int nBufSize )
{
	return ( do_getdents( true, nFile, psDirEnt, nBufSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int do_rewinddir( bool bKernel, int nFile )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if ( psFile->f_nType != FDT_DIR )
	{
		nError = -ENOTDIR;
		goto error;
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );


	if ( NULL == psInode->i_psOperations->rewinddir )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->rewinddir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
	psFile->f_nPos = 0;
	UNLOCK_INODE_RO( psInode );
      error:
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_rewinddir( int nFile )
{
	return ( do_rewinddir( false, nFile ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_utime( const char *a_pzPath, const struct utimbuf *psBuf )
{
	Inode_s *psInode;
	struct stat sStat;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_utime() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );
	kfree( pzPath );

	if ( nError < 0 )
	{
		goto error1;
	}
	if ( NULL != psBuf )
	{
		sStat.st_mtime = psBuf->modtime;
		sStat.st_atime = psBuf->actime;
	}
	else
	{
		sStat.st_mtime = sStat.st_atime = sys_GetTime( NULL );
	}

	nError = cfs_wstat( psInode, &sStat, WSTAT_ATIME | WSTAT_MTIME );

	put_inode( psInode );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_chmod( const char *a_pzPath, const mode_t nMode )
{
	Process_s *psProc = CURRENT_PROC;
	Inode_s *psInode;
	struct stat sStat;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_chmod() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );
	kfree( pzPath );

	if ( nError < 0 )
	{
		goto error1;
	}

	nError = cfs_rstat( psInode, &sStat, true );
	if ( nError < 0 )
	{
		goto error1;
	}

	if ( psProc->pr_nFSUID != sStat.st_uid && is_in_group( sStat.st_gid ) == false && is_root( true ) == false )
	{
		nError = -EPERM;
		goto error1;
	}

	sStat.st_mode = nMode & S_IALLUGO;

	nError = cfs_wstat( psInode, &sStat, WSTAT_MODE );

	put_inode( psInode );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_fchmod( bool bKernel, int nFile, mode_t nMode )
{
	Process_s *psProc = CURRENT_PROC;
	File_s *psFile;
	struct stat sStat;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	nError = cfs_rstat( psFile->f_psInode, &sStat, true );
	if ( nError < 0 )
	{
		goto error1;
	}

	if ( psProc->pr_nFSUID != sStat.st_uid && is_in_group( sStat.st_gid ) == false && is_root( true ) == false )
	{
		nError = -EPERM;
		goto error1;
	}

	sStat.st_mode = nMode & S_IALLUGO;

	nError = cfs_wstat( psFile->f_psInode, &sStat, WSTAT_MODE );
      error1:
	put_fd( psFile );
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int fchmod( int nFile, mode_t nMode )
{
	return ( do_fchmod( true, nFile, nMode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_fchmod( int nFile, mode_t nMode )
{
	return ( do_fchmod( false, nFile, nMode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int chown_inode( Inode_s *psInode, uid_t nUID, gid_t nGID )
{
	Process_s *psProc = CURRENT_PROC;
	struct stat sStat;
	int nError;

	if ( nUID == -1 && nGID == -1 )
	{
		return ( 0 );
	}

	if ( nUID != -1 && is_root( true ) != true )
	{
		return ( -EPERM );
	}


	if ( nGID != -1 )
	{
		nError = cfs_rstat( psInode, &sStat, true );
		if ( nError < 0 )
		{
			return ( nError );
		}

		if ( psProc->pr_nFSUID != sStat.st_uid && is_in_group( nGID ) == false && is_root( true ) == false )
		{
			return ( -EPERM );
		}
	}
	sStat.st_uid = nUID;
	sStat.st_gid = nGID;

	nError = cfs_wstat( psInode, &sStat, ( ( nGID == -1 ) ? 0 : WSTAT_GID ) | ( ( nUID == -1 ) ? 0 : WSTAT_UID ) );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int chown( const char *pzPath, uid_t nUID, gid_t nGID )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );

	if ( nError < 0 )
	{
		return ( nError );
	}
	nError = chown_inode( psInode, nUID, nGID );
	put_inode( psInode );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_chown( const char *a_pzPath, uid_t nUID, gid_t nGID )
{
	char *pzPath;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_chown() failed to dup source path\n" );
		return ( nError );
	}
	nError = chown( pzPath, nUID, nGID );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_fchown( bool bKernel, int nFile, uid_t nUID, gid_t nGID )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = chown_inode( psFile->f_psInode, nUID, nGID );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int fchown( int nFile, uid_t nUID, gid_t nGID )
{
	return ( do_fchown( true, nFile, nUID, nGID ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_fchown( int nFile, uid_t nUID, gid_t nGID )
{
	return ( do_fchown( false, nFile, nUID, nGID ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int stat( const char *pzPath, struct stat *psStat )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );

	if ( nError < 0 )
	{
		goto error1;
	}

	nError = cfs_rstat( psInode, psStat, false );

	if ( g_bFixMode )
	{
		psStat->st_mode |= 0777;
	}

	if ( 0 != nError )
	{
		printk( "Error: sys_stat() rstat() failed (%p)\n", psInode );
	}

	put_inode( psInode );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_stat( const char *a_pzPath, struct stat *psStat )
{
	struct stat sStat;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_stat() failed to dup source path\n" );
		return ( nError );
	}

	nError = stat( pzPath, &sStat );
	kfree( pzPath );

	if ( nError >= 0 )
	{
		if ( memcpy_to_user( psStat, &sStat, sizeof( sStat ) ) < 0 )
		{
			printk( "sys_stat() invalid buffer pointer\n" );
			return ( -EFAULT );
		}
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_lstat( const char *a_pzPath, struct stat *psStat )
{
	struct stat sStat;
	Inode_s *psInode;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_lstat() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, false );
	kfree( pzPath );

	if ( nError < 0 )
	{
		return ( nError );
	}

	nError = cfs_rstat( psInode, &sStat, false );

	if ( 0 == nError )
	{
		nError = memcpy_to_user( psStat, &sStat, sizeof( sStat ) );
	}
	else
	{
		printk( "Error: sys_lstat() rstat() failed\n" );
	}

	if ( g_bFixMode )
	{
		psStat->st_mode |= 0777;
	}

	put_inode( psInode );

	return ( nError );
}

int lstat( const char *pzPath, struct stat *psStat )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzPath, &psInode, true, false );
	if ( nError < 0 )
	{
		return ( nError );
	}
	nError = cfs_rstat( psInode, psStat, false );
	put_inode( psInode );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int do_fstat( bool bKernel, int nFile, struct stat *psStat )
{
	struct stat sStat;
	File_s *psFile;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		printk( "Error: sys_fstat() invalid file descriptor %d\n", nFile );
		return ( -EBADF );
	}

	nError = cfs_rstat( psFile->f_psInode, &sStat, false );

	if ( 0 == nError )
	{
		if ( bKernel )
		{
			*psStat = sStat;
		}
		else
		{
			nError = memcpy_to_user( psStat, &sStat, sizeof( sStat ) );
		}
	}
	else
	{
		printk( "Error: sys_fstat() rstat() failed\n" );
	}

	if ( g_bFixMode )
	{
		psStat->st_mode |= 0777;
	}

	put_fd( psFile );

	return ( nError );
}

int sys_fstat( int nFile, struct stat *psStat )
{
	return ( do_fstat( false, nFile, psStat ) );
}

int fstat( int nFile, struct stat *psStat )
{
	return ( do_fstat( true, nFile, psStat ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_isatty( int nFile )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	printk( "Wheee!!!! someone called sys_isatty!!!\n" );
	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( 0 );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->isatty )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->isatty( psInode->i_psVolume->v_pFSData, psInode->i_pFSData );
	}
	else
	{
		nError = 0;
	}
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

off_t do_lseek( bool bKernel, int nFile, off_t nOffset, int nMode )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if( psFile->f_nType == FDT_FIFO )
	{
		put_fd( psFile );
		return -ESPIPE;
	}

	switch ( nMode )
	{
	case SEEK_SET:
		if ( nOffset < 0 )
		{
			nError = -EINVAL;
		}
		else
		{
			psFile->f_nPos = nOffset;
			nError = 0;
		}
		break;
	case SEEK_CUR:
		if ( psFile->f_nPos + nOffset < 0 )
		{
			nError = -EINVAL;
		}
		else
		{
			psFile->f_nPos += nOffset;
			nError = 0;
		}
		break;
	case SEEK_END:
		{
			struct stat sStat;

			nError = cfs_rstat( psFile->f_psInode, &sStat, true );
			if ( 0 == nError )
			{
				if ( sStat.st_size + nOffset < 0 )
				{
					nError = -EINVAL;
				}
				else
				{
					psFile->f_nPos = sStat.st_size + nOffset;
				}
			}
			break;
		}
	default:
		nError = -EINVAL;
		break;
	}
	put_fd( psFile );

	return ( ( nError < 0 ) ? nError : psFile->f_nPos );
}

off_t lseek( int nFile, off_t nOffset, int nMode )
{
	return ( do_lseek( true, nFile, nOffset, nMode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_lseek( int nFile, uint32 nOffLow, uint32 nOffHigh, int nMode, off_t *pnNewPos )
{
	off_t nOffset = ((off_t)nOffHigh << 32 ) | nOffLow;
	off_t nNewPos = do_lseek( false, nFile, nOffset, nMode );

	if ( nNewPos < 0 )
	{
		return ( nNewPos );
	}
	else
	{
		if ( memcpy_to_user( pnNewPos, &nNewPos, sizeof( nNewPos ) ) < 0 )
		{
			return ( -EFAULT );
		}
		return ( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_old_seek( int nFile, int nOffset, int nMode )
{
	return ( do_lseek( false, nFile, nOffset, nMode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_mknod( const char *a_pzPath, int nMode, dev_t nDev )
{
	Inode_s *psParent = NULL;
	int nLen;
	int nError;
	const char *pzName;
	char *pzPath = NULL;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_mknod() failed to dup source path\n" );
		return ( nError );
	}

	nError = lookup_parent_inode( NULL, pzPath, &pzName, &nLen, &psParent );

	if ( 0 != nError )
	{
		return ( nError );
	}

	if ( 0 == nLen )
	{
		nError = -EINVAL;
		goto error1;
	}

	LOCK_INODE_RO( psParent );
	if ( psParent->i_psOperations->mknod == NULL )
	{
		nError = -EACCES;
		goto error2;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psParent->i_psOperations->mknod( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nLen, nMode & ~CURRENT_THREAD->tr_nUMask, nDev );

      error2:
	UNLOCK_INODE_RO( psParent );
      error1:
	put_inode( psParent );
	kfree( pzPath );
	return ( nError );
}

static int do_mkdir( Inode_s *psRoot, const char *pzPath, mode_t nMode )
{
	Inode_s *psParent = NULL;
	int nLen;
	int nError;
	const char *pzName;

	nError = lookup_parent_inode( psRoot, pzPath, &pzName, &nLen, &psParent );

	if ( nError != 0 )
	{
		return ( nError );
	}

	if ( nLen == 0 )
	{
		nError = -EINVAL;
		goto error1;
	}

	LOCK_INODE_RO( psParent );
	if ( psParent->i_psOperations->mkdir == NULL )
	{
		nError = -EACCES;
		goto error2;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psParent->i_psOperations->mkdir( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nLen, nMode & ~CURRENT_THREAD->tr_nUMask );

      error2:
	UNLOCK_INODE_RO( psParent );
      error1:
	put_inode( psParent );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int mkdir( const char *pzPath, mode_t nMode )
{
	return ( do_mkdir( NULL, pzPath, nMode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int based_mkdir( int nDir, const char *pzPath, mode_t nMode )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nDir );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = do_mkdir( psFile->f_psInode, pzPath, nMode );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_mkdir( const char *a_pzPath, mode_t nMode )
{
	char *pzPath = NULL;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_mkdir() failed to dup source path\n" );
		return ( nError );
	}
	nError = do_mkdir( NULL, pzPath, nMode );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_based_mkdir( int nDir, const char *a_pzPath, mode_t nMode )
{
	File_s *psFile;
	char *pzPath = NULL;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_based_mkdir() failed to dup source path\n" );
		return ( nError );
	}

	psFile = get_fd( false, nDir );

	if ( psFile == NULL )
	{
		nError = -EMFILE;
		goto error1;
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		nError = -ENOTDIR;
		goto error2;
	}
	nError = do_mkdir( psFile->f_psInode, pzPath, nMode );
      error2:
	put_fd( psFile );
      error1:
	kfree( pzPath );
	return ( nError );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_rmdir( Inode_s *psRoot, const char *pzPath )
{
	Inode_s *psParent = NULL;
	int nLen;
	int nError;
	const char *pzName;

	nError = lookup_parent_inode( psRoot, pzPath, &pzName, &nLen, &psParent );

	if ( nError != 0 )
	{
		return ( nError );
	}

	if ( nLen == 0 )
	{
		nError = -EINVAL;
		goto error1;
	}

	LOCK_INODE_RO( psParent );

	if ( psParent->i_psOperations->rmdir == NULL )
	{
		nError = -EACCES;
		goto error2;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psParent->i_psOperations->rmdir( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nLen );

      error2:
	UNLOCK_INODE_RO( psParent );
      error1:
	put_inode( psParent );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int rmdir( const char *pzPath )
{
	return ( do_rmdir( NULL, pzPath ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int based_rmdir( int nDir, const char *pzPath )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nDir );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = do_rmdir( psFile->f_psInode, pzPath );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_rmdir( const char *a_pzPath )
{
	int nError;
	char *pzPath = NULL;
	int nLength;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_rmdir() failed to dup source path\n" );
		return ( nError );
	}

	nLength = strlen( pzPath );
	if ( pzPath[nLength - 1] == '/' )
	{
		pzPath[nLength - 1] = '\0';
	}

	nError = do_rmdir( NULL, pzPath );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_based_rmdir( int nDir, const char *a_pzPath )
{
	File_s *psFile;
	char *pzPath = NULL;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_based_rmdir() failed to dup source path\n" );
		return ( nError );
	}

	psFile = get_fd( false, nDir );

	if ( psFile == NULL )
	{
		nError = -EMFILE;
		goto error1;
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		nError = -ENOTDIR;
		goto error2;
	}
	nError = do_rmdir( psFile->f_psInode, pzPath );
      error2:
	put_fd( psFile );
      error1:
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_dup( int nOldFile )
{
	File_s *psFile;
	int nNewFile;

	psFile = get_fd( false, nOldFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nNewFile = new_fd( false, -1, 0, psFile, false );

	put_fd( psFile );

	return ( nNewFile );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_dup2( int nSrc, int nDst )
{
	File_s *psFile;
	int nNewFile;

	psFile = get_fd( false, nSrc );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nNewFile = new_fd( false, nDst, 0, psFile, false );

	kassertw( nNewFile == nDst );

	put_fd( psFile );

	return ( nDst );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_fchdir( bool bKernel, int nFile )
{
	IoContext_s *psCtx = get_current_iocxt( false );
	File_s *psFile;
	Inode_s *psOldInode;
	struct stat sStat;
	int nError = 0;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = cfs_rstat( psFile->f_psInode, &sStat, true );

	if ( nError < 0 )
	{
		goto error1;
	}
	if ( S_ISDIR( sStat.st_mode ) == false )
	{
		nError = -ENOTDIR;
		goto error1;
	}

	LOCK( psCtx->ic_hLock );
	psOldInode = psCtx->ic_psCurDir;
	psCtx->ic_psCurDir = psFile->f_psInode;

	kassertw( NULL != psCtx->ic_psCurDir );
	if ( NULL != psCtx->ic_psCurDir )
	{
		atomic_inc( &psCtx->ic_psCurDir->i_nCount );
	}
	UNLOCK( psCtx->ic_hLock );

	if ( psOldInode != NULL )
	{
		put_inode( psOldInode );
	}
      error1:
	put_fd( psFile );

	return ( nError );
}

int fchdir( int nFile )
{
	return ( do_fchdir( true, nFile ) );
}

int sys_fchdir( int nFile )
{
	return ( do_fchdir( false, nFile ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_chdir( const char *pzPath )
{
	int nFile;
	int nError;

	nFile = sys_open( pzPath, O_RDONLY );

	if ( nFile < 0 )
	{
		return ( nFile );
	}

	nError = sys_fchdir( nFile );
	sys_close( nFile );

	return ( nError );
}

int chdir( const char *pzPath )
{
	int nFile;
	int nError;

	nFile = open( pzPath, O_RDONLY );

	if ( nFile < 0 )
	{
		return ( nFile );
	}

	nError = fchdir( nFile );
	close( nFile );

	return ( nError );
}

int sys_chroot( const char *a_pzPath )
{
	IoContext_s *psCtx = get_current_iocxt( false );
	Inode_s *psInode;
	Inode_s *psOldInode;
	struct stat sStat;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_chroot() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );
	kfree( pzPath );

	if ( nError < 0 )
	{
		goto error1;
	}
	nError = cfs_rstat( psInode, &sStat, true );

	if ( nError < 0 )
	{
		goto error2;
	}
	if ( S_ISDIR( sStat.st_mode ) == false )
	{
		nError = -ENOTDIR;
		goto error2;
	}
	LOCK( psCtx->ic_hLock );
	psOldInode = psCtx->ic_psRootDir;
	psCtx->ic_psRootDir = psInode;
	UNLOCK( psCtx->ic_hLock );

	if ( psOldInode != NULL )
	{
		put_inode( psOldInode );
	}
	return ( EOK );
      error2:
	put_inode( psInode );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_unlink( Inode_s *psRoot, const char *pzPath )
{
	Inode_s *psParent = NULL;
	int nLen;
	int nError;
	const char *pzName;

	nError = lookup_parent_inode( psRoot, pzPath, &pzName, &nLen, &psParent );

	if ( nError != 0 )
	{
		return ( nError );
	}

	if ( nLen == 0 )
	{
		nError = -ENOENT;
		goto error1;
	}
	LOCK_INODE_RO( psParent );
	if ( psParent->i_psOperations->unlink == NULL )
	{
		nError = -EACCES;
		goto error2;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psParent->i_psOperations->unlink( psParent->i_psVolume->v_pFSData, psParent->i_pFSData, pzName, nLen );
      error2:
	UNLOCK_INODE_RO( psParent );
      error1:
	put_inode( psParent );
	return ( nError );
}

int unlink( const char *pzPath )
{
	return ( do_unlink( NULL, pzPath ) );
}

int based_unlink( int nDir, const char *pzPath )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nDir );

	if ( psFile == NULL )
	{
		return ( -EMFILE );
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		put_fd( psFile );
		return ( -ENOTDIR );
	}
	nError = do_unlink( psFile->f_psInode, pzPath );
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_unlink( const char *a_pzPath )
{
	char *pzPath = NULL;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_unlink() failed to dup source path\n" );
		return ( nError );
	}
	nError = do_unlink( NULL, pzPath );
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_based_unlink( int nDir, const char *a_pzPath )
{
	File_s *psFile;
	char *pzPath = NULL;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_based_unlink() failed to dup source path\n" );
		return ( nError );
	}

	psFile = get_fd( false, nDir );

	if ( psFile == NULL )
	{
		nError = -EMFILE;
		goto error1;
	}
	if ( psFile->f_nType != FDT_DIR )
	{
		nError = -ENOTDIR;
		goto error2;
	}
	nError = do_unlink( psFile->f_psInode, pzPath );
      error2:
	put_fd( psFile );
      error1:
	kfree( pzPath );
	return ( nError );
}

/**
 * \par Description:
 * Sets the file flags (excluding any flags that are only pertinent at open).
 * Note that the flag changes are recorded even if the file's inode's driver
 * returns an error.
 */

static int set_file_flags( File_s *psFile, int nFlags )
{
	int nError = 0;

	Inode_s *psInode = psFile->f_psInode;

	psFile->f_nMode = nFlags & ~( O_OPEN_CONTROL_FLAGS );

	LOCK_INODE_RO( psInode );
	if ( NULL != psInode->i_psOperations->setflags )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		nError = psInode->i_psOperations->setflags( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, psFile->f_nMode );
	}
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

static off_t get_real_position( File_s *psFile, int nWhence, off_t nOffset )
{
	switch ( nWhence )
	{
	case SEEK_SET:
		return ( nOffset );
	case SEEK_CUR:
		return ( psFile->f_nPos + nOffset );
	case SEEK_END:
		{
			struct stat sStat;
			int nError = cfs_rstat( psFile->f_psInode, &sStat, true );

			if ( nError < 0 )
			{
				return ( nError );
			}
			return ( sStat.st_size + nOffset );
		}
	default:
		return ( -EINVAL );
	}
}

static int lock_file( File_s *psFile, bool bWait, struct flock *psLock )
{
	off_t nStart;
	off_t nEnd;

	nStart = get_real_position( psFile, psLock->l_whence, psLock->l_start );
	if ( nStart < 0 )
	{
		return ( nStart );
	}
	if ( psLock->l_len == 0 )
	{
		nEnd = MAX_FILE_OFFSET;
	}
	else
	{
		nEnd = nStart + psLock->l_len - 1;
		if ( nEnd < 0 || nEnd >= MAX_FILE_OFFSET )
		{
			return ( -EINVAL );
		}
	}
	return ( lock_inode_record( psFile->f_psInode, nStart, nEnd, psLock->l_type, bWait ) );
}

static int unlock_file( File_s *psFile, struct flock *psLock )
{
	off_t nStart;
	off_t nEnd;

	nStart = get_real_position( psFile, psLock->l_whence, psLock->l_start );
	if ( nStart < 0 )
	{
		return ( nStart );
	}
	if ( psLock->l_len == 0 )
	{
		nEnd = MAX_FILE_OFFSET;
	}
	else
	{
		nEnd = nStart + psLock->l_len - 1;
		if ( nEnd < 0 || nEnd >= MAX_FILE_OFFSET )
		{
			return ( -EINVAL );
		}
	}
	return ( unlock_inode_record( psFile->f_psInode, nStart, nEnd ) );
}

static int get_file_lock( File_s *psFile, struct flock *psLock )
{
	off_t nStart;
	off_t nEnd;

	nStart = get_real_position( psFile, psLock->l_whence, psLock->l_start );
	if ( nStart < 0 )
	{
		return ( nStart );
	}
	if ( psLock->l_len == 0 )
	{
		nEnd = MAX_FILE_OFFSET;
	}
	else
	{
		nEnd = nStart + psLock->l_len - 1;
		if ( nEnd < 0 || nEnd >= MAX_FILE_OFFSET )
		{
			return ( -EINVAL );
		}
	}
	return ( get_inode_lock_record( psFile->f_psInode, nStart, nEnd, psLock->l_type, psLock ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_fcntl( int nFile, int nCmd, int nArg )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	psInode = psFile->f_psInode;

	switch ( nCmd )
	{
	case F_DUPFD:
		nError = new_fd( false, -1, nArg, psFile, false );
		break;
	case F_COPYFD:
		nError = copy_fd( false, psFile );
		break;
	case F_GETFL:
		nError = psFile->f_nMode;
		break;
	case F_SETFL:
		// In the case of an append-only file, O_APPEND cannot be cleared
		nError = set_file_flags( psFile, ( psFile->f_nMode & ~( O_APPEND | O_NONBLOCK ) ) | ( nArg & ( O_APPEND | O_NONBLOCK ) ) );
		break;
	case F_GETFD:		/* Get close-on-exec flag */
		{
			IoContext_s *psCtx = get_current_iocxt( false );

			LOCK( psCtx->ic_hLock );
			nError = ( GETBIT( psCtx->ic_panCloseOnExec, nFile ) ) ? 0x01 : 0x00;
			UNLOCK( psCtx->ic_hLock );
			break;
		}
	case F_SETFD:		/* Set close-on-exec flag */
		{
			IoContext_s *psCtx = get_current_iocxt( false );

			LOCK( psCtx->ic_hLock );
			if ( nArg & 0x01 )
			{
				SETBIT( psCtx->ic_panCloseOnExec, nFile, true );
			}
			else
			{
				SETBIT( psCtx->ic_panCloseOnExec, nFile, false );
			}
			UNLOCK( psCtx->ic_hLock );
			nError = 0;
			break;
		}

	case F_SETLK:
	case F_SETLKW:
		{
			struct flock sLock;

			nError = memcpy_from_user( &sLock, ( struct flock * )nArg, sizeof( sLock ) );
			if ( nError < 0 )
			{
				break;
			}
			if ( sLock.l_type == F_UNLCK )
			{
				nError = unlock_file( psFile, &sLock );
			}
			else
			{
				nError = lock_file( psFile, nCmd == F_SETLKW, &sLock );
			}
			break;
		}
	case F_GETLK:
		{
			struct flock sLock;

			nError = memcpy_from_user( &sLock, ( struct flock * )nArg, sizeof( sLock ) );
			if ( nError < 0 )
			{
				break;
			}

			nError = get_file_lock( psFile, &sLock );
			if ( nError >= 0 )
			{
				nError = memcpy_to_user( ( struct flock * )nArg, &sLock, sizeof( sLock ) );
			}
			break;
		}
	default:
		printk( "Error : Unknown fcntl() command %d\n", nCmd );
		nError = -EINVAL;
		break;

	}
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_ioctl( bool bKernel, int nFile, int nCmd, void *pBuffer )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	nError = cfs_ioctl( psFile, nCmd, pBuffer, bKernel );

	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_ioctl( int nFile, int nCmd, void *pBuffer )
{
	return ( do_ioctl( false, nFile, nCmd, pBuffer ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int ioctl( int nFile, int nCmd, void *pBuffer )
{
	return ( do_ioctl( true, nFile, nCmd, pBuffer ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_select( int nMaxCnt, fd_set *psReadSet, fd_set *psWriteSet, fd_set *psExceptSet, struct kernel_timeval *psTimeOut )
{
	int nRealCnt = 0;
	int nReadyCount = 0;
	SelectRequest_s *pasRequests;
	int nError = 0;
	sem_id hSyncSema;
	fd_set sReadSet;
	fd_set sWriteSet;
	fd_set sExceptSet;
	int nSetSize = ( nMaxCnt + 7 ) / 8;
	struct kernel_timeval sTimeOut;
	bigtime_t nTimeOut;
	int i;
	int j;

	if ( nMaxCnt < 0 )
	{
		return ( -EINVAL );
	}
	if ( psTimeOut != NULL )
	{
		nError = memcpy_from_user( &sTimeOut, psTimeOut, sizeof( sTimeOut ) );
		if ( nError < 0 )
		{
			return ( nError );
		}
	}
	if ( psReadSet != NULL )
	{
		nError = memcpy_from_user( &sReadSet, psReadSet, nSetSize );
		if ( nError < 0 )
		{
			return ( nError );
		}
	}
	if ( psWriteSet != NULL )
	{
		nError = memcpy_from_user( &sWriteSet, psWriteSet, nSetSize );
		if ( nError < 0 )
		{
			return ( nError );
		}
	}
	if ( psExceptSet != NULL )
	{
		nError = memcpy_from_user( &sExceptSet, psExceptSet, nSetSize );
		if ( nError < 0 )
		{
			return ( nError );
		}
	}

	hSyncSema = create_semaphore( "select_sync", 0, 0 );

	if ( hSyncSema < 0 )
	{
		printk( "Error: sys_select() failed to create semaphore : %d\n", hSyncSema );
		return ( hSyncSema );
	}

	if ( NULL != psTimeOut )
	{
		nTimeOut = ( bigtime_t )sTimeOut.tv_sec * 1000000LL + ( bigtime_t )sTimeOut.tv_usec;
	}
	else
	{
		nTimeOut = INFINITE_TIMEOUT;
	}

	pasRequests = kmalloc( sizeof( SelectRequest_s ) * nMaxCnt * 3, MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == pasRequests )
	{
		nError = -ENOMEM;
		goto error1;
	}

	j = 0;

      /*** Build list of requests	***/

//  printk( "select(): count descriptors (max=%d)\n", nMaxCnt );
	for ( i = 0; i < nMaxCnt; ++i )
	{
		if ( NULL != psReadSet )
		{
			if ( FD_ISSET( i, &sReadSet ) )
			{
				pasRequests[j].sr_nFileDesc = i;
				pasRequests[j].sr_hSema = hSyncSema;
				pasRequests[j].sr_pFile = get_fd( false, i );
				pasRequests[j].sr_nMode = SELECT_READ;

				if ( pasRequests[j].sr_pFile == NULL )
				{
					nError = -EBADF;
					goto error2;
				}
				j++;
				nRealCnt++;
			}
		}
		if ( NULL != psWriteSet && FD_ISSET( i, &sWriteSet ) )
		{
			pasRequests[j].sr_nFileDesc = i;
			pasRequests[j].sr_hSema = hSyncSema;
			pasRequests[j].sr_pFile = get_fd( false, i );;
			pasRequests[j].sr_nMode = SELECT_WRITE;

			if ( pasRequests[j].sr_pFile == NULL )
			{
				nError = -EBADF;
				goto error2;
			}
			nRealCnt++;
			j++;
		}
		if ( NULL != psExceptSet && FD_ISSET( i, &sExceptSet ) )
		{
			pasRequests[j].sr_nFileDesc = i;
			pasRequests[j].sr_hSema = hSyncSema;
			pasRequests[j].sr_pFile = get_fd( false, i );;
			pasRequests[j].sr_nMode = SELECT_EXCEPT;

			if ( pasRequests[j].sr_pFile == NULL )
			{
				nError = -EBADF;
				goto error2;
			}
			nRealCnt++;
			j++;
		}
	}

//  printk( "select(): found %d descriptors\n", nRealCnt );

      /*** Send all request's	***/
	for ( i = 0; i < nRealCnt; ++i )
	{
		kassertw( NULL != pasRequests[i].sr_pFile );

		nError = cfs_add_select_request( ( ( File_s * )pasRequests[i].sr_pFile ), &pasRequests[i] );

		if ( nError < 0 )
		{
			for ( i = 0; i < nRealCnt; ++i )
			{
				if ( pasRequests[i].sr_bSendt )
				{
					cfs_rem_select_request( ( ( File_s * )pasRequests[i].sr_pFile ), &pasRequests[i] );
				}
			}
			break;
		}
		else
		{
			pasRequests[i].sr_bSendt = true;
		}
	}


	if ( nError < 0 )
	{
		printk( "Error: select(): failed to send descriptors %d\n", nError );
		goto error2;
	}

//  printk( "select(): wait for selectors to become ready\n" );


    /*** Wait for one, or more of the request's to be satisfied	***/
	if ( nRealCnt <= 0 && psTimeOut == NULL )
	{
		kerndbg( KERN_WARNING, "sys_select(): Got request for 0 of %d descriptors with " "infinite timeout (read=%p)\n", nMaxCnt, psReadSet );
	}

	nError = lock_semaphore( hSyncSema, 0, nTimeOut );

	if ( nError == -EWOULDBLOCK || nError == -ETIME )
	{
		nError = 0;
	}

	if ( nError >= 0 )
	{
		if ( NULL != psReadSet )
		{
			memset( &sReadSet, 0, nSetSize );
		}
		if ( NULL != psWriteSet )
		{
			memset( &sWriteSet, 0, nSetSize );
		}
		if ( NULL != psExceptSet )
		{
			memset( &sExceptSet, 0, nSetSize );
		}
	}
//  printk( "select(): count ready descriptors\n" );

	for ( i = 0; i < nRealCnt; ++i )
	{
//    printk( "Remove selector %d from fs %d\n", i,
//            pasRequests[i].sr_psFile->f_psInode->i_psVolume->v_nDevNum );
		cfs_rem_select_request( ( ( File_s * )pasRequests[i].sr_pFile ), &pasRequests[i] );
//    printk( "Check if desc is ready %d\n", pasRequests[i].sr_bReady );
		if ( pasRequests[i].sr_bReady && nError >= 0 )
		{
			switch ( pasRequests[i].sr_nMode )
			{
			case SELECT_READ:
				if ( NULL != psReadSet )
				{
					FD_SET( pasRequests[i].sr_nFileDesc, &sReadSet );
				}
				break;
			case SELECT_WRITE:
				if ( NULL != psWriteSet )
				{
					FD_SET( pasRequests[i].sr_nFileDesc, &sWriteSet );
				}
				break;
			case SELECT_EXCEPT:
				if ( NULL != psExceptSet )
				{
					FD_SET( pasRequests[i].sr_nFileDesc, &sExceptSet );
				}
				break;
			}
			nReadyCount++;
		}
	}
//  printk( "select(): found %d ready descriptors (%d)\n",
//          nReadyCount, nError );

	if ( nError >= 0 /*nReadyCount > 0 */  )
	{
		if ( psReadSet != NULL )
		{
			nError = memcpy_to_user( psReadSet, &sReadSet, nSetSize );
			if ( nError < 0 )
			{
				goto error2;
			}
		}
		if ( psWriteSet != NULL )
		{
			nError = memcpy_to_user( psWriteSet, &sWriteSet, nSetSize );
			if ( nError < 0 )
			{
				goto error2;
			}
		}
		if ( psExceptSet != NULL )
		{
			nError = memcpy_to_user( psExceptSet, &sExceptSet, nSetSize );
			if ( nError < 0 )
			{
				goto error2;
			}
		}
		nError = nReadyCount;
	}

      error2:
	for ( i = 0; i < nRealCnt; ++i )
	{
		if( NULL != pasRequests[i].sr_pFile )
			put_fd( ( File_s * )pasRequests[i].sr_pFile );
	}

	kfree( pasRequests );
      error1:
	delete_semaphore( hSyncSema );
//  printk( "select(): release file descriptors, return %d\n", nError );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_access( const char *a_pzPath, int nMode )
{
	Inode_s *psInode;
	int nError;
	char *pzPath;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		printk( "Error: sys_access() failed to dup source path\n" );
		return ( nError );
	}

	nError = get_named_inode( NULL, pzPath, &psInode, true, true );
	kfree( pzPath );

	if ( nError >= 0 )
	{
		put_inode( psInode );
	}

	if ( nError >= 0 )
	{
		return ( 0 );
	}
	else
	{
		return ( nError );
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_file_blocks_p( File_s *psFile, off_t nPos, int nBlockCount, off_t *pnStart, int *pnActualCount )
{
	Inode_s *psInode;
	int nError;

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->get_file_blocks )
	{
		UNLOCK_INODE_RO( psInode );
		return ( -ENOSYS );
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->get_file_blocks( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, nPos, nBlockCount, pnStart, pnActualCount );
	UNLOCK_INODE_RO( psInode );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int32 sys_FileLength( int nFileDesc )
{
	struct stat sStat;
	int nError = sys_fstat( nFileDesc, &sStat );

	if ( 0 == nError )
	{
		return ( sStat.st_size );
	}
	else
	{
		return ( nError );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/*
int Seek( int nFileDesc, int nDistance, int	nMode )
{
    return( sys_Seek( nFileDesc, nDistance, nMode ) );
}
*/

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int normalize_path( const char *pzPath, char **ppzResult )
{
	char *pzNewPath = kmalloc( PATH_MAX, MEMF_KERNEL );
	char *pzLinkBuf = kmalloc( PATH_MAX, MEMF_KERNEL );
	char *pzName = pzNewPath + 1;
	int nNewPathLen = 1;
	int nLinkCount = 0;
	int nError;

	if ( pzNewPath == NULL || pzLinkBuf == NULL )
	{
		nError = -ENOMEM;
		goto error;
	}
	pzNewPath[0] = '/';
	while ( *pzPath == '/' )
		pzPath++;

	while ( true )
	{
		if ( *pzPath == '/' || *pzPath == '\0' )
		{
			struct stat sStat;

			pzNewPath[nNewPathLen] = '\0';
			nError = lstat( pzNewPath, &sStat );
			if ( nError < 0 )
			{
				goto error;
			}
			if ( S_ISLNK( sStat.st_mode ) )
			{
				int nLinkLen;
				int j;
				int k;

				if ( nLinkCount++ > 25 )
				{
					goto error;
				}
				nLinkLen = readlink( pzNewPath, pzLinkBuf, PATH_MAX );
				if ( nLinkLen < 0 || nLinkLen == PATH_MAX )
				{
					if ( nLinkLen < 0 )
					{
						nError = nLinkLen;
					}
					else
					{
						nError = -ENAMETOOLONG;
					}
					goto error;
				}
				for ( j = 0, k = 0; j < nLinkLen; )
				{
					pzLinkBuf[k++] = pzLinkBuf[j];
					if ( pzLinkBuf[j] == '/' )
					{
						while ( pzLinkBuf[j] == '/' )
							j++;
					}
					else
					{
						j++;
					}
				}
				nLinkLen = k;
				if ( nLinkLen > 0 && pzLinkBuf[nLinkLen - 1] == '/' )
				{
					nLinkLen--;
				}
				if ( nLinkLen > 0 && pzLinkBuf[0] != '/' )
				{
					if ( nLinkLen + nNewPathLen + 1 > PATH_MAX )
					{
						nError = -ENAMETOOLONG;
						goto error;
					}
					memcpy( pzName, pzLinkBuf, nLinkLen );
					nNewPathLen = pzName - pzNewPath + nLinkLen;
				}
				else
				{
					memcpy( pzNewPath, pzLinkBuf, nLinkLen );
					nNewPathLen = nLinkLen;
				}
			}
			pzNewPath[nNewPathLen++] = '/';
			while ( *pzPath == '/' )
				pzPath++;
			if ( *pzPath == 0 )
			{
				pzNewPath[nNewPathLen] = '\0';
				break;
			}
			pzName = pzNewPath + nNewPathLen;
		}
		else
		{
			pzNewPath[nNewPathLen++] = *pzPath++;
		}
	}
	pzNewPath[nNewPathLen - 1] = '\0';	// Remove the trailing slash
	kfree( pzLinkBuf );
	*ppzResult = pzNewPath;
	return ( 0 );
      error:
	kfree( pzNewPath );
	kfree( pzLinkBuf );
	return ( nError );
}


/** Iterate a directory, calling a callback for each file.
 * \par Description:
 *	iterate_directory() iterates through all files in a directory and
 *	call a provided callback function for each file, and optionally
 *	each sub-dir. iterate_directory() also accepts a void pointer that
 *	is passed through to the callback along with the path of the file
 *	and the stat() buffer for the file currently being processed.
 *
 * \par Note:
 * \par Warning:
 * \param pzDst
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int iterate_directory( const char *pzDirectory, bool bIncludeDirs, iterate_dir_callback *pfCallback, void *pArg )
{
	char *pzPathBuf;
	int nPathBase;
	int nDir;
	int nError = 0;

	struct kernel_dirent sDirEnt;

	pzPathBuf = kmalloc( 4096, MEMF_KERNEL );

	if ( pzPathBuf == NULL )
	{
		printk( "Error: iterate_directory() no memory for path-buffer\n" );
		return ( -ENOMEM );
	}
	strcpy( pzPathBuf, pzDirectory );
	nDir = open( pzDirectory, O_RDONLY );

	if ( nDir < 0 )
	{
		printk( "Error: iterate_directory() failed to open directory %s (%d)\n", pzDirectory, nDir );
		kfree( pzPathBuf );
		return ( nDir );
	}
	nPathBase = strlen( pzPathBuf );
	if ( pzPathBuf[nPathBase - 1] != '/' )
	{
		pzPathBuf[nPathBase] = '/';
		nPathBase++;
	}

	while ( getdents( nDir, &sDirEnt, sizeof( sDirEnt ) ) == 1 )
	{
		struct stat sStat;
		int nError;

		if ( strcmp( sDirEnt.d_name, "." ) == 0 || strcmp( sDirEnt.d_name, ".." ) == 0 )
		{
			continue;
		}
		strcpy( pzPathBuf + nPathBase, sDirEnt.d_name );
		nError = stat( pzPathBuf, &sStat );
		if ( nError < 0 )
		{
			printk( "Error: iterate_directory() failed to stat %s\n", pzPathBuf );
			continue;
		}
		if ( bIncludeDirs == false && S_ISDIR( sStat.st_mode ) )
		{
			continue;
		}
		nError = pfCallback( pzPathBuf, &sStat, pArg );
		if ( nError < 0 )
		{
			break;
		}
	}
	kfree( pzPathBuf );
	close( nDir );
	return ( nError );
}

static int insert_name( char *pzBuffer, int nCurPathLen, char *pzName, int nNameLen )
{
	if ( nCurPathLen > 0 )
	{
		memmove( pzBuffer + nNameLen + 1, pzBuffer, nCurPathLen );
	}
	pzBuffer[0] = '/';
	memcpy( pzBuffer + 1, pzName, nNameLen );
	return ( nCurPathLen + nNameLen + 1 );
}

status_t get_dirname( Inode_s *psInode, char *pzPath, size_t nBufSize )
{
	Process_s *psProc = CURRENT_PROC;
	Inode_s *psParent;
	int nError = 1;
	int nPathLen = 0;

	atomic_inc( &psInode->i_nCount );

	while ( nError == 1 )
	{
		struct kernel_dirent sDirEnt;
		int nDir;
		bool bIsMntPnt;

		nError = cfs_locate_inode( psInode, "..", 2, &psParent, true );
		if ( nError < 0 )
		{
			break;
		}
		LOCK_INODE_RO( psParent );
		nDir = open_inode( true, psParent, FDT_DIR, O_RDONLY );
		UNLOCK_INODE_RO( psParent );
		if ( nDir < 0 )
		{
			nError = nDir;
			put_inode( psParent );
			break;
		}
		bIsMntPnt = ( psInode->i_psVolume != psParent->i_psVolume );
		nError = -ENOENT;
		while ( getdents( nDir, &sDirEnt, sizeof( sDirEnt ) ) == 1 )
		{
			if ( strcmp( sDirEnt.d_name, "." ) == 0 || strcmp( sDirEnt.d_name, ".." ) == 0 )
			{
				continue;
			}
			if ( bIsMntPnt )
			{
				Inode_s *psTmp;

				nError = cfs_locate_inode( psParent, sDirEnt.d_name, sDirEnt.d_namlen, &psTmp, false );
				if ( nError < 0 )
				{
					break;
				}
				if ( psTmp->i_psMount == psInode )
				{
					if ( nPathLen + sDirEnt.d_namlen + 1 > nBufSize )
					{
						nError = -ENAMETOOLONG;
						break;
					}
					nPathLen = insert_name( pzPath, nPathLen, sDirEnt.d_name, sDirEnt.d_namlen );
					put_inode( psTmp );
					nError = 1;
					break;
				}
				else
				{
					put_inode( psTmp );
				}
			}
			else if ( sDirEnt.d_ino == psInode->i_nInode )
			{
				if ( nPathLen + sDirEnt.d_namlen + 1 > nBufSize )
				{
					nError = -ENAMETOOLONG;
					break;
				}
				nPathLen = insert_name( pzPath, nPathLen, sDirEnt.d_name, sDirEnt.d_namlen );
				nError = 1;
				break;
			}
		}
		close( nDir );
		put_inode( psInode );
		psInode = psParent;
		if ( nError == 1 && psInode == psProc->pr_psIoContext->ic_psRootDir )
		{
			nError = 0;
			if ( nPathLen < nBufSize )
			{
				pzPath[nPathLen] = '\0';
			}
			break;
		}
	}
	put_inode( psInode );
	return ( ( nError >= 0 ) ? nPathLen : nError );
}

status_t get_directory_path( int nFD, char *pzBuffer, int nBufLen )
{
	File_s *psFile;
	int nError;

	psFile = get_fd( true, nFD );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	nError = get_dirname( psFile->f_psInode, pzBuffer, nBufLen );
	put_fd( psFile );
	return ( nError );
}

status_t sys_get_directory_path( int nFD, char *pzBuffer, int nBufLen )
{
	File_s *psFile;
	int nError = 0;

	psFile = get_fd( false, nFD );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( verify_mem_area( pzBuffer, nBufLen, true ) >= 0 )
	{
		nError = get_dirname( psFile->f_psInode, pzBuffer, nBufLen );
	}
	else
	{
		nError = -EFAULT;
	}
	put_fd( psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void dbg_set_mask_mode( int argc, char **argv )
{
	if ( argc < 2 )
	{
		dbprintf( DBP_DEBUGGER, "Mode masking is now %s\n", ( g_bFixMode ) ? "on" : "off" );
	}
	else
	{
		g_bFixMode = atol( argv[1] ) != 0;
	}
	dbprintf( DBP_DEBUGGER, "Mode masking is now %s\n", ( g_bFixMode ) ? "on" : "off" );
}


void init_vfs( void )
{
	g_psKernelIoContext = fs_alloc_ioctx( 1024 );
	kassertw( g_psKernelIoContext != NULL );
	init_flock();
	nwatch_init();
	register_debug_cmd( "mode_msk", "mode_msk mode - if true stat() will merge 0777 into mode", dbg_set_mask_mode );
}
