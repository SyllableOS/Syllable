
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
#include <posix/stat.h>
#include <macros.h>

#include <atheos/string.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include "afs.h"
#include "btree.h"
#include "atheos/bcache.h"

/** Get the number of blocks referenced by an AFS Inode
 * \par Description:
 * For the given Inode, return the number of blocks in the file referenced by that
 * Inode.
 * \par Note:
 * \par Warning:
 * \param psInode	AFS Inode of file to check
 * \return Number of blocks
 * \sa
 *****************************************************************************/
off_t afs_get_inode_block_count( const AfsInode_s * psInode )
{
	if( 0 == psInode->ai_sData.ds_nMaxDoubleIndirectRange )
	{
		if( 0 == psInode->ai_sData.ds_nMaxIndirectRange )
		{
			return( psInode->ai_sData.ds_nMaxDirectRange );
		}
		else
		{
			return( psInode->ai_sData.ds_nMaxIndirectRange );
		}
	}
	else
	{
		return( psInode->ai_sData.ds_nMaxDoubleIndirectRange );
	}
}

/** Validate consistancy of an Inode
 * \par Description:
 * Check whether the given Inode is internally consistant
 * \par Note:
 * \par Warning: XXX Some checks not done
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to check.
 * \return 0 on success, -EINVAL on error
 * \sa
 *****************************************************************************/
int afs_validate_inode( const AfsVolume_s * psVolume, const AfsInode_s * psInode )
{
	off_t nInoNum = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
	int nError = 0;

	if( psInode->ai_nMagic1 != INODE_MAGIC )
	{
		printk( "Inode %Ld has bad magic %d\n", nInoNum, psInode->ai_nMagic1 );
		nError = -EINVAL;
	}
	if( psInode->ai_sInodeNum.len != 1 )
	{
		printk( "Inode %Ld has invalid block run length %d in inode ptr\n", nInoNum, psInode->ai_sInodeNum.len );
		nError = -EINVAL;
	}
//  int32               ai_nMode;
//  int32               ai_nFlags;

	// XXX This is not atomic, copy to temp before compare
	if( atomic_read( &psInode->ai_nLinkCount ) < 0 || atomic_read( &psInode->ai_nLinkCount ) > 1 )
	{
		printk( "Inode %Ld has invalid link count %d\n", nInoNum, atomic_read( &psInode->ai_nLinkCount ) );
		nError = -EINVAL;
	}

	if( psInode->ai_sParent.len != 0 && psInode->ai_sParent.len != 1 )
	{
		printk( "Inode %Ld has invalid block run length %d in parent ptr\n", nInoNum, psInode->ai_sParent.len );
		nError = -EINVAL;
	}
	if( psInode->ai_sAttribDir.len != 0 && psInode->ai_sAttribDir.len != 1 )
	{
		printk( "Inode %Ld has invalid block run length %d in attr dir ptr\n", nInoNum, psInode->ai_sAttribDir.len );
		nError = -EINVAL;
	}


	if( S_ISDIR( psInode->ai_nMode ) && psInode->ai_nIndexType != e_KeyTypeString )
	{
		printk( "Inode %Ld is a directory, but is not keyed on strings(%d)\n", nInoNum, psInode->ai_nIndexType );
		nError = -EINVAL;
	}
	if( 0 )
	{
	// XXX Checks not done
		switch( psInode->ai_nIndexType )
		{
		case e_KeyTypeInt32:
		case e_KeyTypeInt64:
		case e_KeyTypeFloat:
		case e_KeyTypeDouble:
		case e_KeyTypeString:
			break;
		default:
			printk( "Inode %Ld is an index, has invalid key type %d\n", nInoNum, psInode->ai_nIndexType );
			nError = -EINVAL;
			break;
		}
	}

	if( psInode->ai_nInodeSize != psVolume->av_psSuperBlock->as_nBlockSize )
	{
		printk( "Inode %Ld has mismatch between size in inode (%d), and inode size in super block (%d)\n", nInoNum, psInode->ai_nInodeSize, psVolume->av_psSuperBlock->as_nBlockSize );
		nError = -EINVAL;
	}
	if( afs_get_inode_block_count( psInode ) * psVolume->av_psSuperBlock->as_nBlockSize < psInode->ai_sData.ds_nSize )
	{
		printk( "Inode %Ld has fewer block(%Ld) than indicated by size (%Ld)\n", nInoNum, afs_get_inode_block_count( psInode ), psInode->ai_sData.ds_nSize );
		nError = -EINVAL;
	}
	return( nError );
}

/** Validate an AFS file
 * \par Description:
 * Verify that the file pointed to by the given Inode is internally consistant
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode of file to check
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
/*
int afs_validate_file( AfsVolume_s* psVolume, AfsInode_s* psInode )
{
  afs_validate_inode( psVolume, psInode );

  if( afs_is_block_free( psVolume, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ) ) ) {
    printk( "PANIC : Inode block marked as free!!\n" );
  }
  return( afs_validate_stream( psVolume, psInode, NULL ) );
}
*/

/** Create and insert a new Inode
 * \par Description:
 * Create in Inode with the given mode, flags, index type, and name, add it to the
 * given parent directory, and write it to the given volume.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psParent		AFS Inode of directory to contain new Inode
 * \param nMode			Type of inode (normal file, directory, etc. S_IF*)
 * \param nFlags		Inode flags (INF_*)
 * \param nIndexType	Key type of index for inode (e_KeyType*)
 * \param pzName		Name of file associated with Inode
 * \param nNameLen		Length of pzName
 * \param ppsRes		Return argument for new Inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_create_inode( AfsVolume_s * psVolume, AfsInode_s * psParent, int nMode, int nFlags, int nIndexType, const char *pzName, int nNameLen, AfsInode_s ** ppsRes )
{
	AfsSuperBlock_s *const psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	int nAllocGroup = 0;
	BIterator_s sIterator;
	BlockRun_s sInodeNum;
	off_t nInodeNum;
	AfsInode_s *psInode;
	int nError;


	if( NULL != psParent )
	{
		if( nNameLen <= 0 )
		{
			return( -EINVAL );
		}
		// FIXME: Check if names realy can be B_MAX_KEY_SIZE long(or do we need to subtract 1 like here)
		if( nNameLen >= B_MAX_KEY_SIZE )
		{
			return( -ENAMETOOLONG );
		}
		if( S_ISDIR( psParent->ai_nMode ) == false )
		{
			printk( "Error: afs_create_inode() parent is not a directory\n" );
			return( -ENOTDIR );
		}
		nError = bt_lookup_key( psVolume, psParent, pzName, nNameLen, &sIterator );
		if( 1 == nError )
		{
			return( -EEXIST );
		}
		else
		{
			if( nError < 0 )
			{
				printk( "Error : afs_create_inode() Failed to search parent for duplicate filenames %d\n", nError );
				return( nError );
			}
		}
		nAllocGroup = psParent->ai_sInodeNum.group + 8;
	}

	psInode = kmalloc( nBlockSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if( psInode == NULL )
	{
		printk( "Error: afs_create_inode() no memory for inode\n" );
		return( -ENOMEM );
	}

	nError = afs_alloc_blocks( psVolume, &sInodeNum, NULL, nAllocGroup, 1, 1 );

	if( nError < 0 )
	{
		printk( "Error : Failed to alloc space for new inode %d\n", nError );
		kfree( psInode );
		return( nError );
	}
	nInodeNum = afs_run_to_num( psSuperBlock, &sInodeNum );

	memset( psInode, 0, nBlockSize );

	psInode->ai_nMagic1 = INODE_MAGIC;
	psInode->ai_sInodeNum = sInodeNum;
	psInode->ai_nUID = getfsuid();
	psInode->ai_nGID = getfsgid();
	psInode->ai_nMode = nMode;
	psInode->ai_nFlags = nFlags | INF_USED;
	psInode->ai_nIndexType = nIndexType;
	atomic_set( &psInode->ai_nLinkCount, 1 );
	psInode->ai_nCreateTime = get_real_time();
	psInode->ai_nModifiedTime = psInode->ai_nCreateTime;
	psInode->ai_nInodeSize = nBlockSize;

	if( psParent != NULL )
	{
		psInode->ai_sParent = psParent->ai_sInodeNum;
	}

	if( psParent != NULL )
	{
		nError = bt_insert_key( psVolume, psParent, pzName, nNameLen, nInodeNum );
		if( nError < 0 )
		{
			afs_free_blocks( psVolume, &sInodeNum );
			printk( "Error : Failed to insert new inode into parent %d\n", nError );
			kfree( psInode );
			return( nError );
		}
	}
	nError = afs_logged_write( psVolume, psInode, nInodeNum );

	if( nError >= 0 && ppsRes != NULL )
	{
		*ppsRes = psInode;
	}
	else
	{
		kfree( psInode );
	}
	return( nError );
}

/** Remove and delete an AFS Inode
 * \par Description:
 * Remove the given Inode from the given volume and free it's disk space and memory
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to remove
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_delete_inode( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	off_t nOldSize;
	int nError;

	if( atomic_read( &psInode->ai_nLinkCount ) != 0 )
	{
		panic( "afs_delete_inode() called on inode with link count of %d!!!\n", atomic_read( &psInode->ai_nLinkCount ) );
		return( -EINVAL );
	}

	nOldSize = psInode->ai_sData.ds_nSize;
	psInode->ai_sData.ds_nSize = 0;
	nError = afs_truncate_stream( psVolume, psInode );
	if( nError < 0 )
	{
		psInode->ai_sData.ds_nSize = nOldSize;
		printk( "Error: afs_delete_inode() Something went wrong during file truncation! Err = %d\n", nError );
		return( nError );
	}
	nError = afs_free_blocks( psVolume, &psInode->ai_sInodeNum );
	if( nError < 0 )
	{
		return( nError );
	}
	kfree( psInode );
	return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/*
int afs_mkdir( AfsVolume_s* psVolume, AfsInode_s* psParent, const char* pzName, int nNameLen, int nMode )
{
    
    return( afs_create_inode( psVolume, psParent, nMode | S_IFDIR, INF_LOGGED,
			      e_KeyTypeString, pzName, nNameLen, NULL ) );
}
*/

/** Find an Inode number by name
 * \par Description:
 * Find the Inode for the given name in the given parent directory on the given
 * volume and return it's number
 * \par Note: XXX why not return it?
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psParent		AFS Inode of parent directory
 * \param pzName		Name of Inode to look up
 * \param nNameLen		Length of pzName
 * \param pnInodeNum	Return argument for found Inode number
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_lookup_inode( AfsVolume_s * psVolume, AfsInode_s * psParent, const char *pzName, int nNameLen, off_t *pnInodeNum )
{
	BIterator_s sIterator;
	int nError;

	if( S_ISDIR( psParent->ai_nMode ) == false )
	{
		return( -ENOTDIR );
	}
	nError = bt_lookup_key( psVolume, psParent, pzName, nNameLen, &sIterator );
	if( nError == 0 )
	{
		return( -ENOENT );
	}
	if( nError < 0 )
	{
		return( nError );
	}
	*pnInodeNum = sIterator.bi_nCurValue;
	return( 0 );
}

/** Read an AFS Inode from the disk
 * \par Description:
 * Read the Inode with the given pointer from the given volume and return it.
 * Allocates memory for the returned Inode.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInodePtr	Pointer to disk location of Inode
 * \param ppsRes		Return argument for read Inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
status_t afs_do_read_inode( AfsVolume_s * psVolume, BlockRun_s * psInodePtr, AfsInode_s ** ppsRes )
{
	AfsSuperBlock_s *const psSuperBlock = psVolume->av_psSuperBlock;
	AfsInode_s *psInode;
	const off_t nBlockNum = afs_run_to_num( psSuperBlock, psInodePtr );
	int nError;

	psInode = kmalloc( psSuperBlock->as_nBlockSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psInode == NULL )
	{
		printk( "Error : afs_do_read_inode() no memory for inode\n" );
		nError = -ENOMEM;
		goto error1;
	}

	nError = afs_logged_read( psVolume, psInode, nBlockNum );

	if( nError < 0 )
	{
		printk( "Error: afs_do_read_inode() failed to load inode from disk\n" );
		goto error2;
	}

	nError = afs_validate_inode( psVolume, psInode );

	if( nError != 0 )
	{
		printk( "afs_do_read_inode() invalid inode\n" );
		nError = -EINVAL;
		goto error2;
	}

	// Verify that the inode number stored in the inode matches the one we tried to load.

	if( psInode->ai_sInodeNum.group != psInodePtr->group || psInode->ai_sInodeNum.start != psInodePtr->start || psInode->ai_sInodeNum.len != psInodePtr->len )
	{
		printk( "Error : afs_do_read_inode() mismatch betwen loaded block ptr and inode ptr in inode(%d:%d:%d) (%d:%d:%d)\n", psInodePtr->group, psInodePtr->start, psInodePtr->len, psInode->ai_sInodeNum.group, psInode->ai_sInodeNum.start, psInode->ai_sInodeNum.len );

		nError = -EINVAL;
		goto error2;
	}
	psInode->ai_nFlags &= INF_PERMANENT_MASK;

	psInode->ai_psVNode = NULL;
	*ppsRes = psInode;

	return( 0 );
      error2:
	kfree( psInode );
      error1:
	return( nError );
}

/** Revert AFS Inode to on-disk version
 * \par Description:
 * Revert the given Inode to the version on the disk.  In particular,
 * revert Link Count, Parent, Attribute directory, Index type, Inode size,
 * and the Small data.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to revert
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
status_t afs_revert_inode( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	AfsVNode_s *psVNode = psInode->ai_psVNode;
	int32 nUID = psInode->ai_nUID;
	int32 nGID = psInode->ai_nGID;
	int32 nMode = psInode->ai_nMode;
	uint32 nFlags = psInode->ai_nFlags;
	bigtime_t nCreateTime = psInode->ai_nCreateTime;
	bigtime_t nModifiedTime = psInode->ai_nModifiedTime;
	off_t nSize = psInode->ai_sData.ds_nSize;

	status_t nError;

	nError = afs_logged_read( psVolume, psInode, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ) );

	psInode->ai_psVNode = psVNode;
	psInode->ai_nUID = nUID;
	psInode->ai_nGID = nGID;
	psInode->ai_nMode = nMode;
	psInode->ai_nFlags = nFlags;
	psInode->ai_nCreateTime = nCreateTime;
	psInode->ai_nModifiedTime = nModifiedTime;
	psInode->ai_sData.ds_nSize = nSize;

	return( nError );
}

/** Write an AFS Inode to disk
 * \par Description:
 * Validate the given Inode, and, if valid, write it to disk.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to write
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
status_t afs_do_write_inode( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	const off_t nInodeBlock = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );

	if( afs_validate_inode( psVolume, psInode ) != 0 )
	{
		printk( "PANIC : afs_do_write_inode() attempt to release an invalide inode\n" );
		return( -EINVAL );
	}
	psInode->ai_nFlags &= ~INF_WAS_WRITTEN;
	return( afs_logged_write( psVolume, psInode, nInodeBlock ) );
}

/** Read from an AFS file
 * \par Description:
 * Read from the given file into the given buffer starting at the given offset for
 * the given length.  First, if the start position is not block aligned, read the
 * first block/extant and copy from it into the destination buffer.  Next, loop
 * through the blocks/extants, reading until there are no more full blocks, copying
 * into the destination buffer.  Finally, if there's any data left to read, read the
 * last block/extant and copy the correct data into the destination buffer.
 * \par Note: Directory data does not appear to be cached. This could be a huge slowdown.
 * \par Warning: Does not check for a read past the end of the file.  Use afs_read_pos
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to read from
 * \param pBuffer	Destination buffer for the data
 * \param nPos		Start position in file
 * \param nSize		Number of octets to read
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_do_read( AfsVolume_s * psVolume, AfsInode_s * psInode, char *pBuffer, off_t nPos, size_t nSize )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	off_t nFirstBlock = nPos / nBlockSize;
	off_t nLastBlock =( nPos + nSize + nBlockSize - 1 ) / nBlockSize;
	off_t nNumBlocks = nLastBlock - nFirstBlock + 1;
	int nOffset = nPos % nBlockSize;
	off_t nBlockAddr;
	char *pBlock = NULL;
	int nRunLength;
	int nError;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		pBlock = afs_alloc_block_buffer( psVolume );
		if( pBlock == NULL )
		{
			printk( "Error: afs_do_read() no memory for data buffer\n" );
			return( -ENOMEM );
		}
	}

	nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );

	if( nError < 0 )
	{
		printk( "Error: afs_do_read() 1 afs_get_stream_blocks() failed with code %d\n", nError );
	}
	if( nError >= 0 && nOffset != 0 )
	{
		if( S_ISDIR( psInode->ai_nMode ) )
		{
			nError = afs_logged_read( psVolume, pBlock, nBlockAddr );
		}
		else
		{
			pBlock =( char * )get_cache_block( psVolume->av_nDevice, nBlockAddr, nBlockSize );
			if( pBlock == NULL )
			{
				nError = -EIO;
			}
		}
		if( nError >= 0 )
		{
			off_t nLen = min( nSize, nBlockSize - nOffset );

			memcpy( pBuffer, pBlock + nOffset, nLen );
			pBuffer += nLen;
			nSize -= nLen;
			if( S_ISDIR( psInode->ai_nMode ) == false )
			{
				release_cache_block( psVolume->av_nDevice, nBlockAddr );
			}
		}
		nBlockAddr++;
		nFirstBlock++;
		nRunLength--;
		nNumBlocks--;
	}

	for( ; nSize >= nBlockSize && nError >= 0; )
	{
		if( nRunLength == 0 )
		{
			nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );
			if( nError < 0 )
			{
				printk( "Error: afs_do_read() 2 afs_get_stream_blocks( %Ld, %Ld ) failed with code %d\n", nFirstBlock, nNumBlocks, nError );
			}
		}
		if( nError >= 0 )
		{
			off_t nLen = min( nSize / nBlockSize, nRunLength );

			if( S_ISDIR( psInode->ai_nMode ) )
			{
				int i;

				nError = 0;
				for( i = 0; i < nLen; ++i )
				{
					nError = afs_logged_read( psVolume, pBuffer, nBlockAddr );
					if( nError < 0 )
					{
						printk( "Error: afs_do_read() failed to read directory data block\n" );
						break;
					}
					nRunLength--;
					nNumBlocks--;
					nFirstBlock++;
					nBlockAddr++;
					nSize -= nBlockSize;
					pBuffer += nBlockSize;
					kassertw( nRunLength >= 0 );
					kassertw( nSize >= 0 );
				}
			}
			else
			{
				nError = cached_read( psVolume->av_nDevice, nBlockAddr, pBuffer, nLen, nBlockSize );
				if( nError >= 0 )
				{
					nRunLength -= nLen;
					nNumBlocks -= nLen;
					nFirstBlock += nLen;
					nBlockAddr += nLen;
					nSize -= nLen * nBlockSize;
					pBuffer += nLen * nBlockSize;

					kassertw( nRunLength >= 0 );
					kassertw( nSize >= 0 );
				}
			}
		}
	}
	if( nSize > 0 && nError >= 0 )
	{
		if( nRunLength == 0 )
		{
			nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );
			if( nError < 0 )
			{
				printk( "Error: afs_do_read() 3 afs_get_stream_blocks() failed with code %d\n", nError );
			}
		}
		if( nError >= 0 )
		{
			if( S_ISDIR( psInode->ai_nMode ) )
			{
				nError = afs_logged_read( psVolume, pBlock, nBlockAddr );
			}
			else
			{
				pBlock =( char * )get_cache_block( psVolume->av_nDevice, nBlockAddr, nBlockSize );

//      printk( "read:3 %d:%d -> %d blocks at %d\n",
//              psInode->ai_sInodeNum.group, psInode->ai_sInodeNum.start, 1, nBlockAddr);

				if( pBlock == NULL )
				{
					nError = -EIO;
				}
			}
			kassertw( nSize < nBlockSize );
			if( nError >= 0 )
			{
				off_t nLen = min( nSize, nBlockSize );

				memcpy( pBuffer, pBlock, nSize );

				pBuffer += nLen;
				nSize -= nLen;
				if( S_ISDIR( psInode->ai_nMode ) == false )
				{
					release_cache_block( psVolume->av_nDevice, nBlockAddr );
				}
			}
		}
		else
		{
			printk( "Error : afs_do_read() Failed to locate last data block\n" );
		}
	}
	if( S_ISDIR( psInode->ai_nMode ) )
	{
		afs_free_block_buffer( psVolume, pBlock );
	}
	return( nError );
}

/** Write to an AFS file
 * \par Description:
 * Write from the given buffer into the given file starting at the given offset for
 * the given length.  First, if the offset is not block aligned, read the first
 * block/extent, write from the buffer into it starting at offset, and write it back
 * out.  Next, for all the complete blocks/extents, in the range, write to them from
 * the buffer.  Finally, if there is more to write, read the last extent/buffer,
 * write from the buffer into the beginning of the block/extent, and write it out.
 * \par Note: Directory data does not appear to be cached. This could be a huge slowdown.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to write to
 * \param pBuffer	Buffer to write from
 * \param nPos		Start position in file to write at
 * \param a_nSize	Number of octets to write
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_do_write( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pBuffer, off_t nPos, size_t a_nSize )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	int nSize = a_nSize;
	off_t nFirstBlock = nPos / nBlockSize;
	off_t nLastBlock =( nPos + nSize + nBlockSize - 1 ) / nBlockSize;
	off_t nNumBlocks = nLastBlock - nFirstBlock + 1;
	int nOffset = nPos % nBlockSize;
	off_t nBlockAddr;
	char *pBlock = NULL;
	int nRunLength;
	int nError;

	if( ( nPos + nSize ) > ( afs_get_inode_block_count( psInode ) * nBlockSize ) )
	{
		if( S_ISDIR( psInode->ai_nMode ) )
		{
			printk( "Panic: afs_do_write() dir to small!!\n" );
		}
		else
		{
			printk( "Panic: afs_do_write() file to small!!\n" );
		}
		return( -ENOSPC );
	}

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		pBlock = afs_alloc_block_buffer( psVolume );
		if( pBlock == NULL )
		{
			printk( "Error: afs_do_write() no memory for data buffer\n" );
			return( -ENOMEM );
		}
	}

	nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );

	if( nError < 0 )
	{
		printk( "Error: afs_do_write() 1 afs_get_stream_blocks() failed with code %d\n", nError );
	}

	if( nError >= 0 && nOffset != 0 )
	{
		if( S_ISDIR( psInode->ai_nMode ) )
		{
			nError = afs_logged_read( psVolume, pBlock, nBlockAddr );
		}
		else
		{
			pBlock =( char * )get_cache_block( psVolume->av_nDevice, nBlockAddr, nBlockSize );
			if( pBlock == NULL )
			{
				nError = -EIO;
			}
		}
		if( nError >= 0 )
		{
			off_t nLen = min( nSize, nBlockSize - nOffset );

			memcpy( pBlock + nOffset, pBuffer, nLen );
			if( S_ISDIR( psInode->ai_nMode ) )
			{
				nError = afs_logged_write( psVolume, pBlock, nBlockAddr );
			}
			else
			{
				mark_blocks_dirty( psVolume->av_nDevice, nBlockAddr, 1 );
				release_cache_block( psVolume->av_nDevice, nBlockAddr );
			}
			if( nError >= 0 )
			{
				pBuffer += nLen;
				nSize -= nLen;
				nBlockAddr++;
				nFirstBlock++;
				nRunLength--;
				nNumBlocks--;
			}
		}
	}
	while( nSize >= nBlockSize && nError >= 0 )
	{
		off_t nLen;

		if( nRunLength == 0 )
		{
			nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );
			if( nError < 0 )
			{
				printk( "Error: afs_do_write() 2 afs_get_stream_blocks() failed with code %d\n", nError );
			}
		}
		if( nError >= 0 )
		{
			int i;

			nLen = min( nSize / nBlockSize, nRunLength );
			if( S_ISDIR( psInode->ai_nMode ) )
			{
				nError = 0;
				for( i = 0; i < nLen; ++i )
				{
					nError = afs_logged_write( psVolume, pBuffer, nBlockAddr );
					if( nError < 0 )
					{
						printk( "Error: afs_do_write() failed to write directory data block\n" );
						break;
					}
					nRunLength--;
					nNumBlocks--;
					nFirstBlock++;
					nBlockAddr++;
					nSize -= nBlockSize;
					pBuffer += nBlockSize;
					kassertw( nRunLength >= 0 );
					kassertw( nSize >= 0 );
				}
			}
			else
			{
				nError = cached_write( psVolume->av_nDevice, nBlockAddr, pBuffer, nLen, nBlockSize );
				if( nError >= 0 )
				{
					nRunLength -= nLen;
					nNumBlocks -= nLen;
					nFirstBlock += nLen;
					nBlockAddr += nLen;
					nSize -= nLen * nBlockSize;
					pBuffer += nLen * nBlockSize;

					kassertw( nRunLength >= 0 );
					kassertw( nSize >= 0 );
				}
			}
		}
	}
	if( nSize > 0 && nError >= 0 )
	{
		if( nRunLength == 0 )
		{
			nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nFirstBlock, nNumBlocks, &nBlockAddr, &nRunLength );
			if( nError < 0 )
			{
				printk( "Error: afs_do_write() 3 afs_get_stream_blocks() failed with code %d\n", nError );
			}
		}
		if( nError >= 0 )
		{
			if( S_ISDIR( psInode->ai_nMode ) )
			{
				if( psInode->ai_sData.ds_nSize > ( nPos + a_nSize ) )
				{
					nError = afs_logged_read( psVolume, pBlock, nBlockAddr );
				}
			}
			else
			{
				if( psInode->ai_sData.ds_nSize > ( nPos + a_nSize ) )
				{
					pBlock =( char * )get_cache_block( psVolume->av_nDevice, nBlockAddr, nBlockSize );
				}
				else
				{
					pBlock =( char * )get_empty_block( psVolume->av_nDevice, nBlockAddr, nBlockSize );
				}
				if( pBlock == NULL )
				{
					nError = -EIO;
				}
			}
			kassertw( nSize < nBlockSize );
			if( nError >= 0 )
			{
				off_t nLen = min( nSize, nBlockSize );

				memcpy( pBlock, pBuffer, nSize );
				pBuffer += nLen;
				nSize -= nLen;
				if( S_ISDIR( psInode->ai_nMode ) )
				{
					nError = afs_logged_write( psVolume, pBlock, nBlockAddr );
					if( nError < 0 )
					{
						printk( "Error: afs_do_write() failed to write last partial block to directory\n" );
					}
				}
				else
				{
					mark_blocks_dirty( psVolume->av_nDevice, nBlockAddr, 1 );
					release_cache_block( psVolume->av_nDevice, nBlockAddr );
				}
			}
		}
	}

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		afs_free_block_buffer( psVolume, pBlock );
	}

	if( nError >= 0 )
	{
		if( ( nPos + a_nSize ) > psInode->ai_sData.ds_nSize )
		{
			psInode->ai_sData.ds_nSize = nPos + a_nSize;
		}
		psInode->ai_nModifiedTime = get_real_time();
		psInode->ai_nFlags |= INF_WAS_WRITTEN | INF_STAT_CHANGED;
	}

	return( nError );
}

/** Read from an AFS file
 * \par Description:
 * Read from the given file into the given buffer starting at the given position for
 * the given length.  This is a wrapper for afs_do_read that will adjust the length
 * to not read past the end of the file.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS Inode to read from
 * \param pBuffer	Buffer to read into
 * \param nPos		Read start position
 * \param nSize		Amount to read.
 * \return amount read on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_read_pos( AfsVolume_s * psVolume, AfsInode_s * psInode, void *pBuffer, off_t nPos, size_t nSize )
{
	int nError = 0;

	if( nPos > psInode->ai_sData.ds_nSize )
	{
		return( 0 );
	}
	if( ( nPos + nSize ) > psInode->ai_sData.ds_nSize )
	{
		nSize = psInode->ai_sData.ds_nSize - nPos;
	}
	if( nSize > 0 )
	{
		nError = afs_do_read( psVolume, psInode, pBuffer, nPos, nSize );
	}
	else
	{
		nSize = 0;
	}
	return( ( nError < 0 ) ? nError : nSize );
}
