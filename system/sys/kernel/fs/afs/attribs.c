
/*
 *  AFS - The native AtheOS file-system
 *  Copyright(C) 1999 - 2001 Kurt Skauen
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
#include <macros.h>

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/semaphore.h>

#include "afs.h"

/** Release an AFS inode
 * \par Description:
 * If bWriteBack is true and the given Inode is dirty, write it to disk
 * and mark it clean.  Then, free the Inode.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS Inode to release
 * \param bWriteBack	Whether or not to attempt to write out the inode.
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_put_inode( AfsVolume_s * psVolume, AfsInode_s * psInode, bool bWriteBack )
{
	int nError = 0;

	if( psInode == NULL )
	{
		printk( "Panic: afs_put_inode() called with inode == NULL!\n" );
		return( -EINVAL );
	}

	if( afs_validate_inode( psVolume, psInode ) != 0 )
	{
		panic( "afs_put_inode() attempt to release an invalide inode\n" );
	}
	if( bWriteBack )
	{
		if( psInode->ai_nFlags & INF_WAS_WRITTEN )
		{
			nError = afs_logged_write( psVolume, psInode, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ) );

			if( nError < 0 )
			{
				printk( "Panic: afs_put_inode() failed to write inode to log\n" );
			}
			else
			{
				psInode->ai_nFlags &= ~INF_WAS_WRITTEN;
			}
		}
	}
	kfree( psInode );
	return( nError );
}

/** Read in an AFS Inode for an attribute
 * \par Description
 * Read the Inode from the disk for the attribute of the given name from
 * the given attribute directory.
 * \par Note:
 * \par Warning:
 * \param psVolume			AFS filesystem pointer
 * \param psAttrDirInode	AFS Inode of attribute directory
 * \param pzName			Name of attribute to read
 * \param nNameLen			Length of pzName
 * \param ppsAttrInode		Return parameter for read Inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_load_attr_inode( AfsVolume_s * psVolume, AfsInode_s * psAttrDirInode, const char *pzName, int nNameLen, AfsInode_s ** ppsAttrInode )
{
	int nError;
	ino_t nAttrInode;
	BlockRun_s sAttrInode;

	nError = afs_lookup_inode( psVolume, psAttrDirInode, pzName, nNameLen, &nAttrInode );

	if( nError < 0 )
	{
		return( nError );
	}
	afs_num_to_run( psVolume->av_psSuperBlock, &sAttrInode, nAttrInode );
	nError = afs_do_read_inode( psVolume, &sAttrInode, ppsAttrInode );
	return( nError );
}

/** Create or write a SmallData AFS attribute
 * \par Description:
 * Create/write a SmallData attribute (a small attribute in the Inode of the
 * attribute directory) in the given Inode (of the given block size) of the given
 * type, with the given name using the given data of the given size at the given
 * offset.  First, see if the attribute already exists.  If it does, see if the new
 * data fits.  If it does, write it at the given offset.  Of it doesn't fit, see if
 * the total data of the attribute will fit in an empty spot in the Inode.  If it
 * does, move the existing attribute to the new position, and write the new data.
 * If the total attribute does not fit in the Inode, error out.  If the attribute is
 * not present in the Inode, see if the attribute will fit in the empty space in the
 * Inode.  If it does, write it in the first empty spot.  If it does not, error.
 * \par Note:
 * \par Warning:
 * \param pInode		AFS Inode containing the SD attributes
 * \param nBlockSize	Total size of the Inode
 * \param nType			Type of the attribute (ATTR_TYPE_*)
 * \param pzName		Name of attribute
 * \param nNameLen		Length of pzName
 * \param pData			Data to write
 * \param nPos			Offset into attribute to write data
 * \param nSize			Ammount to write
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_sd_create_attribute( char *pInode, int nBlockSize, int nType, const char *pzName, int nNameLen, const void *pData, int nPos, int nSize )
{
	AfsSmallData_s *psEntry;
	int nTotSize = 0;

//  printk( "afs_sd_add_create_attribute() %s\n",(char*)pData );

	for( psEntry = ( AfsSmallData_s * ) ( pInode ); ( ( char * )psEntry ) < ( pInode + nBlockSize ) && 0 != psEntry->sd_nNameSize; psEntry = NEXT_SD( psEntry ) )
	{
		nTotSize += SD_SIZE( psEntry );

		if( psEntry->sd_nNameSize == nNameLen && strncmp( ( char * )( psEntry + 1 ), pzName, nNameLen ) == 0 )
		{
			if( nSize + nPos <= psEntry->sd_nDataSize )
			{
//      printk( "Will fit\n" );
				memcpy( SD_DATA( psEntry ) + nPos, pData, nSize );
				psEntry->sd_nType = nType;
				return( nSize );
			}
			else
			{
				AfsSmallData_s *psTmp;
				const int nDeltaSize = nPos + nSize - psEntry->sd_nDataSize;
				int nMoveSize = 0;

				for( psTmp = NEXT_SD( psEntry ); ( ( char * )psTmp ) < ( pInode + nBlockSize ) && 0 != psTmp->sd_nNameSize; psTmp = NEXT_SD( psTmp ) )
				{
					nMoveSize += SD_SIZE( psTmp );
				}

				if( nTotSize + nMoveSize + nDeltaSize > nBlockSize )
				{
//        printk( "Can't insert %d extra bytes. Block is full %d(%d)\n", nDeltaSize, nTotSize + nMoveSize, nBlockSize );
					return( -ENOSPC );
				}
				memmove(( ( char * )NEXT_SD( psEntry ) ) + nDeltaSize, NEXT_SD( psEntry ), nMoveSize );

//      printk( "Move %d byte by %d\n", nMoveSize, nDeltaSize );

				psEntry->sd_nDataSize = nPos + nSize;
				memcpy( SD_DATA( psEntry ) + nPos, pData, nSize );
				psEntry->sd_nType = nType;
				return( nSize );
			}
		}
//    printk( "afs_sd_add_create_attribute() scan %s\n", SD_DATA( psEntry ) );
	}
//  printk( "TotSize = %d\n", nTotSize );

	if( nTotSize + sizeof( AfsSmallData_s ) + nNameLen + nSize <= nBlockSize )
	{
		int i;

		psEntry->sd_nType = nType;
		psEntry->sd_nNameSize = nNameLen;
		psEntry->sd_nDataSize = nSize;

		kassertw(( ( uint32 )SD_DATA( psEntry ) + nSize ) <= ( ( ( uint32 )pInode ) + nBlockSize ) );
		memcpy( psEntry + 1, pzName, nNameLen );
		memcpy( SD_DATA( psEntry ), pData, nSize );


		for( i = ( ( ( uint32 )NEXT_SD( psEntry ) ) - ( ( uint32 )pInode ) ); i < nBlockSize; ++i )
		{
			if( pInode[i] != 0 )
			{
				printk( "PANIC : create : pos %d == %d\n", i, pInode[i] );
				pInode[i] = 0;
			}
		}
		return( nSize );

	}
	else
	{
//    printk( "afs_sd_add_create_attribute() inode is full %d\n", nTotSize );
		return( -ENOSPC );
	}
}

/** Find a SmallData attribute
 * \par Description:
 * Find the SmallData attribute with the given name in the given Inode with the
 * given blocksize.  Just loop through all the used entries, comparing the name of
 * the entry with the given name.
 * \par Note:
 * \par Warning:
 * \param pInode		AFS Inode to search
 * \param nBlockSize	Size of Inode
 * \param pzName		Name of attribute to find
 * \param nNameLen		Size of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static AfsSmallData_s *afs_sd_find_entry( char *pInode, int nBlockSize, const char *pzName, int nNameLen )
{
	AfsSmallData_s *psEntry;

	for( psEntry = ( AfsSmallData_s * ) pInode; ( ( char * )psEntry ) < ( pInode + nBlockSize ) && 0 != psEntry->sd_nNameSize; psEntry = NEXT_SD( psEntry ) )
	{
//    printk( "afs_sd_find_entry() scan %s\n", SD_DATA( psEntry ) );
		if( psEntry->sd_nNameSize == nNameLen && strncmp( ( char * )( psEntry + 1 ), pzName, nNameLen ) == 0 )
		{
//      printk( "afs_sd_find_entry() found %s\n", SD_DATA( psEntry ) );
			return( psEntry );
		}
	}
	return( NULL );
}

/** Delete a SmallData attribute
 * \par Description:
 * Delete the SmallData attribute with the given name from the given Inode. Find the
 * named attribute, and move all attributes after the named on down a slot.
 * Finally, zero out the freed up at the end.
 * \par Note:
 * \par Warning:
 * \param pInode		AFS Inode to delete from
 * \param nBlockSIze	Size of pInode
 * \param pzName		Name of attribute to delete
 * \param nNameLen		Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_sd_delete_entry( char *pInode, int nBlockSize, const char *pzName, int nNameLen )
{
	AfsSmallData_s *psEntry;
	AfsSmallData_s *psTmp;
	int nTotSize = 0;
	int nSize;
	int i;

//  printk( "afs_sd_delete_entry()  %s\n", pzName );
	psEntry = afs_sd_find_entry( pInode, nBlockSize, pzName, nNameLen );

	if( NULL == psEntry )
	{
//    printk( "afs_sd_delete_entry() failed to delete %s\n", pzName );
		return( -ENOENT );
	}

	nSize = SD_SIZE( psEntry );

	for( psTmp = NEXT_SD( psEntry ); ( ( char * )psTmp ) < ( pInode + nBlockSize ) && 0 != psTmp->sd_nNameSize; psTmp = NEXT_SD( psTmp ) )
	{
		nTotSize += SD_SIZE( psTmp );
//    printk( "afs_sd_delete_entry() scan %s(ts=%d s=%d ns=%d)\n",
//          SD_DATA( psTmp ), nTotSize, SD_SIZE( psTmp ), psTmp->sd_nNameSize );
	}

//  printk( "Move %d bytes , clear %d - %d\n", nTotSize,(((char*)psEntry) - pInode) + nTotSize, nSize );
	memmove( psEntry, NEXT_SD( psEntry ), nTotSize );
	memset(( ( char * )psEntry ) + nTotSize, 0, nSize );
	kassertw(( ( char * )psEntry ) + nTotSize + nSize <= pInode + nBlockSize );

	for( i = ( ( ( uint32 )psEntry ) - ( ( uint32 )pInode ) ) + nTotSize; i < nBlockSize; ++i )
	{
		if( pInode[i] != 0 )
		{
			printk( "PANIC : delete : pos %d == %d\n", i, pInode[i] );
			pInode[i] = 0;
		}
	}
	return( 0 );
}

/** Open AFS attribute directory of a file
 * \par Description:
 * Allocate an attribute directory handle for the given file
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem handle
 * \param pNode		Inode of file whose attribute directory is to be opened
 * \param ppCookie	Return argument for attribute directory handle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_open_attrdir( void *pVolume, void *pNode, void **pCookie )
{
	AfsAttrIterator_s *psIterator;

	psIterator = kmalloc( sizeof( AfsAttrIterator_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if( NULL == psIterator )
	{
		printk( "PANIC : afs_open_attrdir() no memory for cookie\n" );
		return( -ENOMEM );
	}
	psIterator->aa_sBIterator.bi_nCurNode = -1;

	*pCookie = psIterator;
	return( 0 );
}

/** Close AFS attribute directory of a file
 * \par Description:
 * Free the given attribute directory handle
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem handle
 * \param pNode		Inode of file owning attribute directory
 * \param pCookie	Attribute directory handle to free
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_close_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	kfree( pCookie );
	return( 0 );
}

/** Reset read of AFS attribute directory to the beginning
 * \par Description:
 * Reading an attribute directory is an iterative process, with successive reads
 * returning sucessive attribute names.  This will reset the read so the next read
 * will return the first attribute
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning attribute directory
 * \param pCookie	Attribute directory handle to reset
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_rewind_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	AfsAttrIterator_s *psIterator = pCookie;

	if( NULL == psIterator )
	{
		panic( "afs_rewind_attrdir() pCookie == NULL\n" );
		return( -EINVAL );
	}

	memset( psIterator, 0, sizeof( AfsAttrIterator_s ) );
	psIterator->aa_sBIterator.bi_nCurNode = -1;
	return( 0 );
}

/** Read an attribute entry out of an attribute directory
 * \par Description:
 * Read the next attribute entry out of the attribute directory for the given file
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning attribute directory
 * \param pCookie	Attribute directory handle
 * \param psBuffer	Directory entry to fill
 * \param bufsize	Size of buffer in psBuffer
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_read_attrdir( void *pVolume, void *pNode, void *pCookie, struct kernel_dirent *psBuffer, size_t bufsize )
{
	AfsVolume_s *psVolume = pVolume;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsAttrIterator_s *psIterator = pCookie;
	int nError = 0;

	if( NULL == psIterator )
	{
		panic( "afs_read_attrdir() pCookie == NULL\n" );
		return( -EINVAL );
	}

	AFS_LOCK( psVnode );

	if( -1 != psIterator->aa_nPos )
	{
		AfsSmallData_s *psEntry =( AfsSmallData_s * ) ( psInode + 1 );
		int i;

		for( i = 0; i < psIterator->aa_nPos; ++i )
		{
			if( ( ( uint32 )psEntry ) >= ( ( ( uint32 )psInode ) + nBlockSize ) || 0 == psEntry->sd_nNameSize )
			{
				break;
			}
			psEntry = NEXT_SD( psEntry );
		}
		if( ( ( uint32 )psEntry ) < ( ( ( uint32 )psInode ) + nBlockSize ) && 0 != psEntry->sd_nNameSize )
		{
			memcpy( psBuffer->d_name, psEntry + 1, psEntry->sd_nNameSize );
			psBuffer->d_name[psEntry->sd_nNameSize] = '\0';
			psBuffer->d_namlen = psEntry->sd_nNameSize;
			psIterator->aa_nPos++;
			nError = 1;
		}
	}

	if( 0 == nError )
	{
		AfsVNode_s *psAttrVnode;
		AfsInode_s *psAttrInode;
		ino_t nAttrInode;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			goto done;
		}

		nAttrInode = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sAttribDir );
		nError = get_vnode( psVolume->av_nFsID, nAttrInode,( void ** )&psAttrVnode );

		if( nError < 0 )
		{
			printk( "PANIC : afs_read_attrdir() failed to load attribdir inode\n" );
			goto done;
		}
		psAttrInode = psAttrVnode->vn_psInode;

		if( psIterator->aa_nPos > 0 )
		{
			nError = bt_find_first_key( psVolume, psAttrInode, &psIterator->aa_sBIterator );
			psIterator->aa_nPos = -1;
		}
		else
		{
			nError = bt_get_next_key( psVolume, psAttrInode, &psIterator->aa_sBIterator );
		}
		if( nError >= 0 )
		{
			memcpy( psBuffer->d_name, psIterator->aa_sBIterator.bi_anKey, psIterator->aa_sBIterator.bi_nKeySize );
			psBuffer->d_name[psIterator->aa_sBIterator.bi_nKeySize] = '\0';
			psBuffer->d_namlen = psIterator->aa_sBIterator.bi_nKeySize;
			nError = 1;
		}
		else
		{
			if( nError == -ENOENT )
			{
				nError = 0;	/* End of directory */
			}
		}
		put_vnode( psVolume->av_nFsID, nAttrInode );
	}
      done:
	AFS_UNLOCK( psVnode );

	return( nError );
}

/** Remove an AFS attribute
 * \par Description:
 * Delete and free the attribute of the given file with the given name.  First,
 * assume it's a SmallData attribute, and try to delete it.  If the return value
 * indicates it was not found, assume it was in the attribute directory of the given
 * file.  Remove the name from the btree of the attribute directory, truncate the
 * attribute to zero, and free all it's owned blocks.  Write out the inode of both
 * the attribute (freeing the Inode), and the attribute directory.
 * \par Note: Locking must be done before calling.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode of file owning attribute to be deleted
 * \param pzName	Name of attribute to delete
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_do_remove_attr( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pzName, int nNameLen )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsInode_s *psAttrDirInode = NULL;
	int nError;

	nError = afs_sd_delete_entry(( char * )( psInode + 1 ), nBlockSize - sizeof( AfsInode_s ), pzName, nNameLen );

	if( -ENOENT == nError )
	{
		AfsInode_s *psAttrInode = NULL;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			nError = -ENOENT;
			goto done;
		}

		nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );
		if( nError < 0 )
		{
			printk( "PANIC : afs_do_remove_attr() failed to load attribdir inode\n" );
			psAttrDirInode = NULL;
			goto done;
		}
		nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

		if( nError < 0 )
		{
			printk( "PANIC : afs_do_remove_attr() failed to load attrib inode\n" );
			goto done;
		}

		// Decrease count and make sure it doesn't go negative.
		if( !atomic_add_negative( &psAttrInode->ai_nLinkCount, -1 ) )
		{
			nError = bt_delete_key( psVolume, psAttrDirInode, pzName, nNameLen );

			if( nError < 0 )
			{
				panic( "afs_do_remove_attr() failed to remove attribute from attribute directory %d\n", nError );
			}
			else
			{
				psAttrInode->ai_sData.ds_nSize = 0;
				nError = afs_truncate_stream( psVolume, psAttrInode );

				if( nError < 0 )
				{
					printk( "PANIC : afs_do_remove_attr() Something went wrong during file truncation! Err = %d\n", nError );
				}
				psAttrInode->ai_nFlags &= ~INF_WAS_WRITTEN;	// Make sure afs_put_inode don't write to the released block

				afs_free_blocks( psVolume, &psAttrInode->ai_sInodeNum );
			}
		}
		else
		{
			atomic_inc( &psAttrInode->ai_nLinkCount );
			printk( "afs_do_remove_attr() Seems like someone else got the same idea!\n" );
			nError = -ENOENT;
		}
		afs_put_inode( psVolume, psAttrInode, true );
	}
      done:
	if( psAttrDirInode != NULL )
	{
		afs_put_inode( psVolume, psAttrDirInode, true );
	}

	return( nError );
}

/** Remove an AFS attribute
 * \par Description:
 * Remove the named attribute from the attribute directory of the given
 * file
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning attribute
 * \param pzName	Name of attribute to delete
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_remove_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen )
{
	AfsVolume_s *psVolume = pVolume;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return( -EROFS );

	nError = afs_begin_transaction( psVolume );

	if( nError < 0 )
	{
		printk( "PANIC : afs_remove_attr() failed to start transaction. Err = %d\n", nError );
		return( nError );
	}

	AFS_LOCK( psVnode );
	nError = afs_do_remove_attr( psVolume, psInode, pzName, nNameLen );

	afs_end_transaction( psVolume, true );

	if( nError > 0 )
	{
		notify_node_monitors( NWEVENT_ATTR_DELETED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), pzName, nNameLen );
	}
	AFS_UNLOCK( psVnode );

	return( nError );
}

/** Delete all attributes from a file
 * \par Description:
 * Delete all the attributes of the given file.  For each entry in the attribute
 * directory of this file, delete that attribute.  Then, free the inode for the
 * attribute directory itself.
 * \par Note: XXX Does not delete SD attributes
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_delete_file_attribs( AfsVolume_s * psVolume, AfsInode_s * psInode, bool bNoLock )
{
	AfsVNode_s *psVnode = psInode->ai_psVNode;
	AfsInode_s *psAttrDirInode;
	BIterator_s sIterator;
	int i;
	int nError;

	if( psInode->ai_sAttribDir.len == 0 )
	{
		return( 0 );
	}

	if ( !bNoLock )
		AFS_LOCK( psVnode );

	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "Error: afs_delete_file_attribs() failed to load attribdir inode\n" );
		goto error1;
	}

	i = 0;
	while( bt_find_first_key( psVolume, psAttrDirInode, &sIterator ) >= 0 )
	{
		char zBuffer[512];

		strncpy( zBuffer, sIterator.bi_anKey, sIterator.bi_nKeySize );
		zBuffer[sIterator.bi_nKeySize] = '\0';

//    printk( "afs_delete_file_attribs() delete %s\n", zBuffer );
		nError = afs_do_remove_attr( psVolume, psInode, sIterator.bi_anKey, sIterator.bi_nKeySize );

		if( nError < 0 )
		{
			printk( "Error: afs_delete_file_attribs() failed to delete attribute %s\n", zBuffer );
			goto error2;
		}

		if( ( i++ % 5 ) == 0 )
		{
			nError = afs_checkpoint_journal( psVolume );
			if( nError < 0 )
			{
				printk( "Error: afs_delete_file_attribs() failed to checkpoint journal\n" );
				goto error2;
			}
		}

	}

	psAttrDirInode->ai_sData.ds_nSize = 0;
	nError = afs_truncate_stream( psVolume, psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_delete_file_attribs() Something went wrong during file truncation! Err = %d\n", nError );
	}
	afs_free_blocks( psVolume, &psAttrDirInode->ai_sInodeNum );
	psAttrDirInode->ai_nFlags &= ~INF_WAS_WRITTEN;	// Make sure afs_put_inode don't write to the released block
	// XXX DFG is this necessary?  We pass in false to afs_put_inode...
      error2:
	afs_put_inode( psVolume, psAttrDirInode, false );
      error1:
	if ( !bNoLock )
		AFS_UNLOCK( psVnode );

	return( nError );
}

/** Read the stats of an AFS attribute
 * \par Description:
 * Read the stat information (size, index type, etc.) of the attribute of the given
 * file with the given name
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning the attribute
 * \param pzName	Name of attribute to stat
 * \param nNameLen	Length of pzName
 * \param psBuffer	Attribute info structure to fill
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_stat_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, struct attr_info *psBuffer )
{
	AfsVolume_s *psVolume = pVolume;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsInode_s *psAttrDirInode;
	AfsInode_s *psAttrInode;
	AfsSmallData_s *psEntry;
	int nError;

	AFS_LOCK( psVnode );

	psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nBlockSize - sizeof( AfsInode_s ), pzName, nNameLen );

	if( NULL != psEntry )
	{
		psBuffer->ai_size = psEntry->sd_nDataSize;
		psBuffer->ai_type = psEntry->sd_nType;
		nError = 0;
		goto done;
	}


	if( psInode->ai_sAttribDir.len == 0 )
	{
		nError = -ENOENT;
		goto done;
	}

	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_stat_attr() failed to load attribdir inode\n" );
		goto done;
	}
	nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );
	afs_put_inode( psVolume, psAttrDirInode, false );

	if( nError < 0 )
	{
		goto done;
	}

	psBuffer->ai_size = psAttrInode->ai_sData.ds_nSize;
	psBuffer->ai_type = psAttrInode->ai_nIndexType;

	afs_put_inode( psVolume, psAttrInode, false );

	nError = 0;
      done:
	AFS_UNLOCK( psVnode );
	return( nError );
}

/** Write into a big attribute
 * \par Description:
 * Write from the given buffer into the given attribute at the given offset for the
 * given length.  First, check to see if the new data will fit into the existing
 * attribute.  If not, expand the attribute to fit the data.  Then, write the data
 * into the attribute.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_write_big_attr( AfsVolume_s * psVolume, AfsInode_s * psAttrInode, int nFlags, const void *pBuffer, off_t nPos, size_t nLen )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	off_t nOldBlockCount;
	int nError = 0;

//  printk( "write big attrib\n" );

	if( nFlags & O_TRUNC && psAttrInode->ai_sData.ds_nSize > 0 )
	{
		psAttrInode->ai_sData.ds_nSize = 0;
		afs_truncate_stream( psVolume, psAttrInode );
	}

	nOldBlockCount = afs_get_inode_block_count( psAttrInode );

	if( ( nOldBlockCount * nBlockSize ) < ( nPos + nLen ) )
	{
		off_t nNewBlockCount =( nPos + nLen + nBlockSize - 1 ) / nBlockSize;

		kassertw( nNewBlockCount > nOldBlockCount );

		nError = afs_expand_stream( psVolume, psAttrInode, nNewBlockCount - nOldBlockCount );
		if ( nError < 0 )
		{
			return ( nError );
		}

		nNewBlockCount = afs_get_inode_block_count( psAttrInode );

		if( ( nNewBlockCount * nBlockSize ) < ( nPos + nLen ) )
		{
			printk( "Error: afs_write_big_attr() failed to expand file to fit %d bytes. Scrinked to %Ld\n", nLen, nNewBlockCount * nBlockSize - nPos );

			nLen = nNewBlockCount * nBlockSize - nPos;

			if( nLen == 0 )
			{
				nError = -ENOSPC;
			}
		}
		kassertw( nLen >= 0 );
	}

	if( nError >= 0 )
	{
		nError = afs_do_write( psVolume, psAttrInode, pBuffer, nPos, nLen );
		if( nError >= 0 )
		{
			nError = nLen;
		}
		else
		{
			printk( "PANIC : afs_write_big_attr() failed to write buffer to file %d\n", nError );
		}
	}
	return( nError );
}

/** Write into a small attribute
 * \par Description:
 * Write from the given buffer into the small attribute with the given name of the
 * given file, from the given buffer at the given starting position for the given
 * length.  Basically just a wrapper around afs_sd_create_attribute.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to write small attribute to
 * \param pzName	Name of attribute to write to
 * \param nNameLen	Length of pzName
 * \param nType		Type of attribute
 * \param a_pBuffer	Buffer to write from
 * \param nPos		Position in attribute to write to
 * \param nLen		Number of octets to write
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_write_small_attr( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pzName, int nNameLen, int nType, const char *a_pBuffer, int nPos, size_t nLen )
{
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	char *pBuffer =( char * )a_pBuffer;

	return( afs_sd_create_attribute( ( char * )( psInode + 1 ), nSDSize, nType, pzName, nNameLen, pBuffer, nPos, nLen ) );
}

/** Write to an AFS attribute
 * \par Description:
 * Write the contents of the given buffer to the named attribute of the
 * given file at the given offset for the given length.  First, see if the attibute
 * is already in the attribute directory.  If it is, write it.  If it is not, see if
 * the new attribute data fits into the SmallData area.  If it does, write it there
 * (overwriting the existing attribute, if there is one).  If it does not fit, see
 * if the attribute exists in the SD area, and if it does move it to the attribute
 * directory, possibly creating the attribute directory.  Then, write the new
 * data to the attribute in the attribute directory. Finally, notify any monitors
 * listening for events on this file.
 * \par Note: Check for notification on queries/live queries
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning the attribute
 * \param pzName	Name of attribute to write to
 * \param nNameLen	Length of pzName
 * \param nFlags	Write flags (O_TRUNC, etc.)
 * \param nType		Type of attribute to write
 * \param pBuffer	Buffer to write from
 * \param nPos		Position in attribute to write at
 * \param nLen		Length of pBuffer
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_write_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nLen )
{
	AfsVolume_s *psVolume = pVolume;
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsInode_s *psAttrDirInode = NULL;
	bool bInodeDirty = false;
	int nError;
	int nSmallDataSize = nPos + nLen + nNameLen + sizeof( AfsSmallData_s ) + sizeof( AfsInode_s );

	if( nType >= ATTR_TYPE_COUNT )
	{
		return( -EINVAL );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return( -EROFS );

	if( nPos > nSDSize )
	{
		nSmallDataSize = nSDSize + 1;
	}

//  printk( "afs_write_attr(%s)\n", pzName );

	nError = afs_begin_transaction( psVolume );

	if( nError < 0 )
	{
		printk( "PANIC : afs_write_attr() failed to start transaction. Err = %d\n", nError );
		return( nError );
	}

	AFS_LOCK( psVnode );

	if( psInode->ai_sAttribDir.len != 0 )
	{
		AfsInode_s *psAttrInode;

		nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

		if( nError < 0 )
		{
			printk( "PANIC: afs_write_attr() failed to load attribute directory.\n" );
			psAttrDirInode = NULL;	// Just in case...
			goto done;
		}
		nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

		if( nError < 0 && nError != -ENOENT )
		{
			printk( "Error : afs_write_attr() failed to lookup attrib inode. Err = %d\n", nError );
			goto done;
		}
		if( nError >= 0 )
		{
			nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, pBuffer, nPos, nLen );
			afs_put_inode( psVolume, psAttrInode, true );
			goto done;
		}
	}

	// If we got this far the node does not exist in the attrib dir
	// We first attempt to store it in the small data area...

	if( nFlags & O_TRUNC )
	{
		nError = afs_sd_delete_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );
		if( nError >= 0 )
		{
			bInodeDirty = true;
		}
	}

	nError = afs_write_small_attr( psVolume, psInode, pzName, nNameLen, nType, pBuffer, nPos, nLen );

	// ... if that failed we writes it to a file.
	if( nError < 0 )
	{
		AfsInode_s *psAttrInode = NULL;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			kassertw( NULL == psAttrDirInode );
			nError = afs_create_inode( psVolume, NULL, S_IFDIR | 0777, INF_LOGGED | INF_ATTRIBUTES, e_KeyTypeString, "", 0, &psAttrDirInode );
			if( nError < 0 )
			{
				printk( "Error : afs_write_attr() failed to create attribute directory. Err = %d\n", nError );
				psAttrDirInode = NULL;
				goto done;
			}
			psInode->ai_sAttribDir = psAttrDirInode->ai_sInodeNum;
			bInodeDirty = true;
		}
		nError = afs_create_inode( psVolume, psAttrDirInode, S_IFREG | 0777, INF_LOGGED, nType, pzName, nNameLen, &psAttrInode );
		if( nError < 0 )
		{
			printk( "Error : afs_write_attr() failed to create attribute i-node\n" );
			goto done;
		}

		if( ( nFlags & O_TRUNC ) == 0 )
		{
			AfsSmallData_s *psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );

			if( NULL != psEntry )
			{
//      printk( "Move attrib from small to big area\n" );
				nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, SD_DATA( psEntry ), 0, psEntry->sd_nDataSize );
				if( nError < 0 )
				{
					afs_put_inode( psVolume, psAttrInode, true );
					goto done;
				}
			}
			afs_sd_delete_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );
			bInodeDirty = true;
		}
		nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, pBuffer, nPos, nLen );
		afs_put_inode( psVolume, psAttrInode, true );
	}
	else
	{
		bInodeDirty = true;
	}
      done:
	if( NULL != psAttrDirInode )
	{
		afs_put_inode( psVolume, psAttrDirInode, true );
	}
	if( bInodeDirty )
	{
		afs_do_write_inode( psVolume, psInode );
	}
	afs_end_transaction( psVolume, true );

	if( nError > 0 )
	{
		notify_node_monitors( NWEVENT_ATTR_WRITTEN, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), pzName, nNameLen );
	}
	AFS_UNLOCK( psVnode );

	return( nError );
}

/** Read an AFS attribute
 * \par Description:
 * Read from the named attribute of the given file, into the given buffer starting
 * at the given position up to the given length.  First, see if the attribute is in
 * the SmallData area.  If so, read into the buffer, at most up the the end of the
 * attribute.  If not, check to see if it's in the attribute directory.  If so, read
 * into the buffer at most up to the end of the attribute.
 * \par Note: XXX Attributes reads appear to not be cached
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file owning the attribute
 * \param pzName	Name of the attribute to read
 * \param nNameLen	Length of pzName
 * \param nType		Type of attribute
 * \param pBuffer	Buffer to read into
 * \param nPos		Position to read from
 * \param nLen		Length to read (pBuffer is at least this long)
 * \return number of octets read on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_read_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nType, void *pBuffer, off_t nPos, size_t nLen )
{
	AfsVolume_s *psVolume = pVolume;
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsSmallData_s *psEntry;
	AfsInode_s *psAttrDirInode = NULL;
	AfsInode_s *psAttrInode = NULL;
	int nError;

	AFS_LOCK( psVnode );

	psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );

	if( NULL != psEntry )
	{
		if( nType != psEntry->sd_nType )
		{
			nError = -EINVAL;
			goto done;
		}
		if( nPos + nLen > psEntry->sd_nDataSize )
		{
			nLen =( psEntry->sd_nDataSize > nPos ) ? ( psEntry->sd_nDataSize - nPos ) : 0;
		}
		if( nLen > 0 )
		{
			memcpy( pBuffer, SD_DATA( psEntry ) + nPos, nLen );
		}
		nError = nLen;
		goto done;
	}

	if( psInode->ai_sAttribDir.len == 0 )
	{
		nError = -ENOENT;
		goto done;
	}
	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_read_attr() failed to load attribdir i-node\n" );
		psAttrDirInode = NULL;	// Just in case...
		goto done;
	}

	nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

	if( nError < 0 )
	{
		printk( "Error : afs_read_attr() failed to lookup attrib i-node. Err = %d\n", nError );
		psAttrInode = NULL;	// Just in case...
		goto done;
	}
	if( psAttrInode->ai_nIndexType == nType )
	{
		nError = afs_read_pos( pVolume, psAttrInode, pBuffer, nPos, nLen );
	}
	else
	{
		nError = -EINVAL;
	}
      done:
	if( NULL != psAttrInode )
	{
		afs_put_inode( psVolume, psAttrInode, false );
	}
	if( NULL != psAttrDirInode )
	{
		afs_put_inode( psVolume, psAttrDirInode, false );
	}
	AFS_UNLOCK( psVnode );
	return( nError );
}
