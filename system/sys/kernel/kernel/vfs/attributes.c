
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
#include <posix/fcntl.h>
#include <posix/unistd.h>
#include <posix/dirent.h>
#include <posix/utime.h>
#include <posix/time.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/sysbase.h"
#include "vfs.h"

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_open_attrdir( int nFile )
{
	File_s *psFile;
	Inode_s *psInode;
	int nAttrDir;
	int nError;


	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		nError = -EBADF;
		goto error1;
	}
	psInode = psFile->f_psInode;
	atomic_inc( &psInode->i_nCount );
	put_fd( psFile );


//    nError = get_named_inode( NULL, pzName, &psInode, true, true );

//    if ( nError < 0 ) {
//      goto error1;
//    }

	psFile = alloc_fd();

	if ( psFile == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->open_attrdir != NULL )
	{
		nError = psInode->i_psOperations->open_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, &psFile->f_pFSCookie );
	}
	else
	{
		nError = -ENOSYS;
	}
	UNLOCK_INODE_RO( psInode );

	if ( nError < 0 )
	{
		goto error3;
	}

	psFile->f_psInode = psInode;
	psFile->f_nType = FDT_ATTR_DIR;

	nAttrDir = new_fd( false, -1, 0, psFile, false );
	if ( nAttrDir < 0 )
	{
		nError = nAttrDir;
		goto error4;
	}
	add_file_to_inode( psInode, psFile );
	return ( nAttrDir );
      error4:
	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->close_attrdir != NULL )
	{
		psInode->i_psOperations->close_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
	}
	UNLOCK_INODE_RO( psInode );
      error3:
	free_fd( psFile );
      error2:
	put_inode( psInode );
      error1:
	return ( nError );
}


#if 0
int sys_fs_open_attrdir( const char *pzName )
{
	File_s *psFile;
	Inode_s *psInode;
	int nFile;
	int nError;

	nFile = sys_open( pzName, O_RDONLY );

	if ( nFile < 0 )
	{
		nError = nFile;
		goto error1;
	}

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		nError = -EBADF;
		goto error2;
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->open_attrdir )
	{
		nError = -ENOSYS;
		goto error3;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->open_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, &psFile->f_pAttribCookie );
	if ( nError < 0 )
	{
		goto error3;
	}
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );
	return ( nFile );
      error3:
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );
      error2:
	sys_close( nFile );
      error1:
	return ( nError );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_close_attrdir( int nFile )
{
	return ( sys_close( nFile ) );
#if 0
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->close_attrdir )
	{
		put_fd( psFile );
		UNLOCK_INODE_RO( psInode );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->close_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pAttribCookie );
	UNLOCK_INODE_RO( psInode );
	psFile->f_pAttribCookie = NULL;

	put_fd( psFile );

	sys_close( nFile );
	return ( nError );
#endif
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_rewind_attrdir( int nFile )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}

	if ( psFile->f_nType != FDT_ATTR_DIR )
	{
		put_fd( psFile );
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->rewind_attrdir )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->rewind_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
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

int sys_read_attrdir( int nFile, struct kernel_dirent *psBuffer, int nCount )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( psFile->f_nType != FDT_ATTR_DIR )
	{
		put_fd( psFile );
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->read_attrdir )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->read_attrdir( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie, psBuffer, nCount * sizeof( *psBuffer ) );
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

int sys_remove_attr( int nFile, const char *pzName )
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

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->remove_attr )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->remove_attr( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, pzName, strlen( pzName ) );
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

int sys_stat_attr( int nFile, const char *pzName, struct attr_info *psBuffer )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_SOCKET )
	{
		put_fd( psFile );
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->stat_attr )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->stat_attr( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, pzName, strlen( pzName ), psBuffer );
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

static int do_write_attr( int nFile, const char *pzName, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nLen )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( false, nFile );

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_SOCKET )
	{
		put_fd( psFile );
		return ( -EINVAL );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->write_attr )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->write_attr( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, pzName, strlen( pzName ), nFlags, nType, pBuffer, nPos, nLen );
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

int sys_write_attr( WriteAttrParams_s * psParams )
{
	return ( do_write_attr( psParams->wa_nFile, psParams->wa_pzName, psParams->wa_nFlags, psParams->wa_nType, psParams->wa_pBuffer, psParams->wa_nPos, psParams->wa_nLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_read_attr( int nFile, const char *pzName, int nType, void *pBuffer, off_t nPos, size_t nLen )
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

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->read_attr )
	{
		UNLOCK_INODE_RO( psInode );
		put_fd( psFile );
		return ( -ENOSYS );
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->read_attr( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, pzName, strlen( pzName ), nType, pBuffer, nPos, nLen );
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

int sys_read_attr( WriteAttrParams_s * psParams )
{
	return ( do_read_attr( psParams->wa_nFile, psParams->wa_pzName, psParams->wa_nType, psParams->wa_pBuffer, psParams->wa_nPos, psParams->wa_nLen ) );
}




int sys_open_indexdir( const char *pzVolume )
{
	Inode_s *psInode;
	Inode_s *psTmp;
	File_s *psFile;
	int nFile;
	int nError;

	nError = get_named_inode( NULL, pzVolume, &psInode, true, true );

	if ( nError < 0 )
	{
		goto error1;
	}
	if ( psInode->i_psOperations->open_indexdir == NULL )
	{
		nError = -ENOSYS;
		goto error2;
	}
	if ( psInode->i_psVolume == NULL || psInode->i_psVolume->v_psMountPoint == NULL || psInode->i_psVolume->v_psMountPoint->i_psMount == NULL )
	{
		nError = -ENOSYS;
		goto error2;
	}
	psTmp = psInode->i_psVolume->v_psMountPoint->i_psMount;
	atomic_inc( &psTmp->i_nCount );
	put_inode( psInode );
	psInode = psTmp;



	psFile = alloc_fd();

	if ( psFile == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->open_indexdir != NULL )
	{
		nError = psInode->i_psOperations->open_indexdir( psInode->i_psVolume->v_pFSData, &psFile->f_pFSCookie );
	}
	else
	{
		nError = -ENOSYS;
	}
	UNLOCK_INODE_RO( psInode );

	if ( nError < 0 )
	{
		goto error3;
	}

	psFile->f_psInode = psInode;
	psFile->f_nType = FDT_INDEX_DIR;
	nFile = new_fd( false, -1, 0, psFile, false );
	if ( nFile < 0 )
	{
		nError = nFile;
		goto error4;
	}
	add_file_to_inode( psInode, psFile );
	return ( nFile );
      error4:
	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->close_indexdir != NULL )
	{
		psInode->i_psOperations->close_indexdir( psInode->i_psVolume->v_pFSData, psFile->f_pFSCookie );
	}
	UNLOCK_INODE_RO( psInode );
      error3:
	free_fd( psFile );
      error2:
	put_inode( psInode );
      error1:
	return ( nError );
}

static int do_rewind_indexdir( bool bKernel, int nDir )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	psFile = get_fd( bKernel, nDir );

	if ( NULL == psFile )
	{
		return ( -ENOENT );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );

	if ( psFile->f_nType != FDT_INDEX_DIR )
	{
		nError = -ENOTDIR;
		goto error;
	}

	if ( NULL == psInode->i_psOperations->rewind_indexdir )
	{
		printk( "Error: do_rewind_indexdir() Filesystem %p does not support rewind_indexdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->rewind_indexdir( psInode->i_psVolume->v_pFSData, psFile->f_pFSCookie );
      error:
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );

	return ( nError );
}

int sys_rewind_indexdir( int nDir )
{
	return ( do_rewind_indexdir( false, nDir ) );
}


static int do_read_indexdir( bool bKernel, int nFile, struct kernel_dirent *psDirEnt, int nBufSize )
{
	File_s *psFile;
	Inode_s *psInode;
	int nError;

	if ( NULL == psDirEnt )
	{
		return ( -EINVAL );
	}
	psFile = get_fd( bKernel, nFile );

	if ( NULL == psFile )
	{
		return ( -ENOENT );
	}

	psInode = psFile->f_psInode;

	LOCK_INODE_RO( psInode );

	if ( psFile->f_nType != FDT_INDEX_DIR )
	{
		nError = -ENOTDIR;
		goto error;
	}

	if ( NULL == psInode->i_psOperations->read_indexdir )
	{
		printk( "Error: do_read_indexdir() Filesystem %p does not support read_indexdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->read_indexdir( psInode->i_psVolume->v_pFSData, psFile->f_pFSCookie, psDirEnt, nBufSize );
      error:
	UNLOCK_INODE_RO( psInode );
	put_fd( psFile );

	return ( nError );
}


int sys_read_indexdir( int nDir, struct kernel_dirent *psBuffer, size_t nBufferSize )
{
	int nError;

	if ( verify_mem_area( psBuffer, nBufferSize, true ) < 0 )
	{
		return ( -EFAULT );
	}
	nError = do_read_indexdir( false, nDir, psBuffer, nBufferSize );
	return ( nError );
}

int sys_create_index( const char *pzVolume, const char *pzName, int nType, int nFlags )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzVolume, &psInode, true, true );

	if ( nError < 0 )
	{
		return ( nError );
	}

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->create_index )
	{
		printk( "Error: sys_create_index() Filesystem %p does not support create_indexdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->create_index( psInode->i_psVolume->v_pFSData, pzName, strlen( pzName ), nType, nFlags );
      error:
	UNLOCK_INODE_RO( psInode );
	put_inode( psInode );
	return ( nError );
}



int sys_remove_index( const char *pzVolume, const char *pzName )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzVolume, &psInode, true, true );

	if ( nError < 0 )
	{
		return ( nError );
	}

	LOCK_INODE_RO( psInode );
	if ( NULL == psInode->i_psOperations->remove_index )
	{
		printk( "Error: sys_remove_index() Filesystem %p does not support remove_indexdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->remove_index( psInode->i_psVolume->v_pFSData, pzName, strlen( pzName ) );
      error:
	UNLOCK_INODE_RO( psInode );
	put_inode( psInode );

	return ( nError );
}

int sys_stat_index( const char *pzVolume, const char *pzName, struct index_info *psBuffer )
{
	Inode_s *psInode;
	int nError;

	nError = get_named_inode( NULL, pzVolume, &psInode, true, true );

	if ( nError < 0 )
	{
		return ( nError );
	}

	LOCK_INODE_RO( psInode );
	if ( psInode->i_psOperations->stat_index == NULL )
	{
		printk( "Error: sys_stat_index() Filesystem %p does not support stat_indexdir\n", psInode->i_psOperations );
		nError = -EACCES;
		goto error;
	}
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nError = psInode->i_psOperations->stat_index( psInode->i_psVolume->v_pFSData, pzName, strlen( pzName ), psBuffer );
      error:
	UNLOCK_INODE_RO( psInode );
	put_inode( psInode );

	return ( nError );
}
