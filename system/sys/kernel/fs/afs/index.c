
/*
 *  AFS - The native AtheOS file-system
 *  Copyright(C) 1999 - 2000 Kurt Skauen
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

#include <posix/fcntl.h>
#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/ioctl.h>
#include <macros.h>

#include <atheos/string.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>

#include "afs.h"
#include "btree.h"

/** Open an AFS attribute index directory
 * \par Description:
 * Reading an attribute directory is an iterative process.  Successive
 * reads of the directory return sucessive entries.  This will set up the
 * state necessary to do such a read, chiefly allocating an attribute
 * directory handle
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param ppCookie	Return parameter for the AFS attribute directory handle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_open_indexdir( void *pVolume, void **ppCookie )
{
	AfsFileCookie_s *psCookie;

	psCookie = kmalloc( sizeof( AfsFileCookie_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if( NULL == psCookie )
	{
		return( -ENOMEM );
	}
	psCookie->fc_nOMode = O_RDONLY;
	psCookie->fc_sBIterator.bi_nCurNode = -1;

	*ppCookie = psCookie;

	return( 0 );
}

/** Close an AFS attribute index directory
 * \par Description:
 * Reading an attribute directory is an iterative process.  Successive
 * reads of the directory return sucessive entries.  This will free up the
 * state necessary to do such a read, chiefly freeing an attribute
 * directory handle.  After this call, a call to afs_open_indexdir must be
 * made before the handle can be used again.
 * \par Note:
 * Must be called on an open handle
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pCookie	AFS attribute directory handle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_close_indexdir( void *pVolume, void *pCookie )
{
	kfree( pCookie );
	return( 0 );
}

/** Rewind an AFS attribute directory
 * \par Description:
 * Reading an attribute directory is an iterative process.  Successive
 * reads of the directory return sucessive entries.  This will reset the
 * handle so that the next read on the directory will return the first
 * entry in the directory, rather than the next.
 * \par Note:
 * Must be called on an open handle
 * \par Warning:
 * XXX This does not do anything.  It won't work.  Need to set
 * bi_nCurNode to -1
 * \param pVolume	AFS filesystem pointer
 * \param pCookie	AFS attribute directory handle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_rewind_indexdir( void *pVolume, void *pCookie )
{
	return( 0 );
}

/** Read from an AFS attribute directory
 * \par Description:
 * Reading an attribute directory is an iterative process.  Successive
 * reads of the directory return sucessive entries.  This will read the
 * next entry in the attribute directory into the given buffer.
 * \par Note:
 * Must be called on an open handle
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pCookie	AFS attribute directory handle
 * \param psBuffer	Entry buffer to fill
 * \param nBufferSize	Size of psBuffer
 * \return number of entries read on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_read_indexdir( void *pVolume, void *pCookie, struct kernel_dirent *psBuffer, size_t nBufferSize )
{
	AfsVolume_s *psVolume = pVolume;
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	AfsFileCookie_s *psCookie = pCookie;
	AfsInode_s *psIndexDir;
	int nError;

	LOCK( psVolume->av_hIndexDirMutex );
	nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sIndexDir, &psIndexDir );

	if( nError < 0 )
	{
		printk( "Error: afs_create_index() failed to read index directory inode %d\n", nError );
		goto error;
	}
	nError = afs_do_read_dir( psVolume, psIndexDir, &psCookie->fc_sBIterator, -1, psBuffer, nBufferSize );

	kfree( psIndexDir );
      error:
	UNLOCK( psVolume->av_hIndexDirMutex );
	return( nError );
}

/** Create an AFS attribute index
 * \par Description:
 * Attributes can be indexed for fast lookup.  This will create an index
 * with the given name and type on the given volume.
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pzName	Name of index
 * \param nNameLen	Length of pzName
 * \param nType		Type of the attriute being indexed
 * \param nFlags	Creation flags for index
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_create_index( void *pVolume, const char *pzName, int nNameLen, int nType, int nFlags )
{
	AfsVolume_s *psVolume = pVolume;
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	AfsInode_s *psIndexDir;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return( -EROFS );

	LOCK( psVolume->av_hIndexDirMutex );

	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "PANIC : afs_create_index() failed to start transaction %d\n", nError );
		goto error1;
	}

	nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sIndexDir, &psIndexDir );

	if( nError < 0 )
	{
		printk( "Error: afs_create_index() failed to read index directory inode %d\n", nError );
		goto error2;
	}
	nError = afs_create_inode( psVolume, psIndexDir, 0777 | S_IFDIR, 0, 0, pzName, nNameLen, NULL );
	kfree( psIndexDir );
      error2:
	afs_end_transaction( psVolume, true );
      error1:
	UNLOCK( psVolume->av_hIndexDirMutex );

	return( nError );
}

/** Delete an AFS attribute index
 * \par Description:
 * Delete the named index from the given volume
 * \par Note:
 * \par Warning:
 * XXX Not implemented yet
 * \param pVolume	AFS filesystem pointer
 * \param pzName	Name of index to delete
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_remove_index( void *pVolume, const char *pzName, int nNameLen )
{
	return( 0 );
}

/** Get the stats for an index
 * \par Description:
 * Get the stat information (type, size, owner, etc.) for an attribute index.
 * \par Note:
 * \par Warning:
 * XXX Not implemented yet
 * \param pVolume	AFS filesystem pointer
 * \param pzName	Name of index to stat
 * \param nNameLen	Length of pzName
 * \param psBuffer	Index information to fill
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_stat_index( void *pVolume, const char *pzName, int nNameLen, struct index_info *psBuffer )
{
	return( 0 );
}
