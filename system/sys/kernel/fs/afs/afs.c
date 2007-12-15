/*
 *  AFS - The native AtheOS file-system
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

#include <posix/fcntl.h>
#include <posix/unistd.h>
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


/** Get a block sized temporary buffer
 * \par Description:
 * Get (or allocate if there aren't any) a buffer the size of an AFS block.  This is a
 * temporary buffer internal to AFS, and not part of the file cache system.  When free,
 * the buffers are stored in a singly linked list using the first pointer-sized chunk of
 * the buffer.  Thus, av_pFirstTmpBuffer points to the first free buffer, and the first
 * pointer-sized chunk of that buffer points to the next free buffer, and so on.  Since
 * these are temporary buffers (never held across calls) there should be very few of
 * them.  This is good, because they are never freed.
 * \par Note:
 * \par Warning:
 * There is no explicit null termination on this list.  I suspect no more than one is
 * ever in use.
 * \param psVolume	AFS filesystem pointer
 * \return buffer on success, NULL on failure
 * \sa
 *****************************************************************************/
void *afs_alloc_block_buffer( AfsVolume_s * psVolume )
{
	void *pBuffer;


#ifdef AFS_PARANOIA
		if( psVolume == NULL )
	{
		panic( "afs_alloc_block_buffer() called with psVolume == NULL\n" );
		return ( NULL );
	}

#endif	/*  */
		LOCK( psVolume->av_hBlockListMutex );
	if( psVolume->av_pFirstTmpBuffer == NULL )
	{
		pBuffer = kmalloc( psVolume->av_psSuperBlock->as_nBlockSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );
		if( pBuffer != NULL )
		{
			psVolume->av_nTmpBufferCount++;
		}
	}
	else
	{
		pBuffer = psVolume->av_pFirstTmpBuffer;
		psVolume->av_pFirstTmpBuffer = *( ( void ** )pBuffer );
		psVolume->av_nTmpBufferCount++;
	} UNLOCK( psVolume->av_hBlockListMutex );
	if( psVolume->av_nTmpBufferCount > 30 )
	{
		printk( "Warning: afs_alloc_block_buffer() We got %d block buffers allocated simultanuously! "  "Could be a leak\n", psVolume->av_nTmpBufferCount );
	}
	return ( pBuffer );
}

/** Put a block sized temporary buffer
 * \par Description:
 * Put (but don't free) a buffer the size of an AFS block.  It is stored in a singly
 * linked list using the first pointer-sized chunk of the buffer.  Thus,
 * av_pFirstTmpBuffer points to the first free buffer, and the first pointer-sized chunk
 * of that buffer points to the next free buffer, and so on.  Since these are temporary
 * buffers (never held across calls) there should be very few of them.  This is good,
 * because they are never freed.
 * \par Note:
 * \par Warning:
 * There is no explicit null termination on this list.  I suspect no more than one is
 * ever in use.
 * \param psVolume	AFS filesystem pointer
 * \param pBuffer	buffer to put
 * \return none
 * \sa
 *****************************************************************************/
void afs_free_block_buffer( AfsVolume_s * psVolume, void *pBuffer )
{

#ifdef AFS_PARANOIA
		if( psVolume == NULL )
	{
		panic( "afs_free_block_buffer() called with psVolume == NULL\n" );
		return;
	}
	if( pBuffer == NULL )
	{
		panic( "afs_free_block_buffer() called with pBuffer == NULL\n" );
		return;
	}

#endif	/*  */
		LOCK( psVolume->av_hBlockListMutex );
	*( ( void ** )pBuffer ) = psVolume->av_pFirstTmpBuffer;
	psVolume->av_pFirstTmpBuffer = pBuffer;
	psVolume->av_nTmpBufferCount--;
	UNLOCK( psVolume->av_hBlockListMutex );
	if( psVolume->av_nTmpBufferCount < 0 )
	{
		panic( "afs_free_block_buffer() av_nTmpBufferCount got negative value %d\n", psVolume->av_nTmpBufferCount );
	}
}

/** Remove an Inode from the deleted file directory and delete it
 * \par Description:
 * Remove the given Inode from the deleted file list of the given volume.
 * If the volume was out of space when the Inode was deleted, then it was
 * not moved to the deleted list. In either case, the inode is then
 * permantly deleted.
 * \par Note:
 * XXX The section of code tha sets INF_NOT_IN_DELME is commented
 * out.  Check to see if this is a problem.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode to be deleted
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_remove_from_deleted_list( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	AfsSuperBlock_s * psSuperBlock;
	ino_t nInodeNum;

	AfsInode_s * psDeleteMe;
	char zFileName[32];
	int nError;


#ifdef AFS_PARANOIA
		if( psVolume == NULL )
	{
		panic( "afs_remove_from_deleted_list() called with psVolume == NULL\n" );
		return ( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "afs_remove_from_deleted_list() called with psInode == NULL\n" );
		return ( -EINVAL );
	}

#endif	/*  */
		psSuperBlock = psVolume->av_psSuperBlock;
	if( atomic_read( &psInode->ai_nLinkCount ) != 0 )
	{
		panic( "afs_remove_from_deleted_list() i-node has link count of %d\n", atomic_read( &psInode->ai_nLinkCount ) );
		return ( -EINVAL );
	}
	nInodeNum = afs_run_to_num( psSuperBlock, &psInode->ai_sInodeNum );
	if( ( psInode->ai_nFlags & INF_NOT_IN_DELME ) == 0 )
	{
		sprintf( zFileName, "%08x%08x.del", ( int )( nInodeNum >> 32 ), ( int )( nInodeNum & 0xffffffff ) );
		nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sDeletedFiles, &psDeleteMe );
		if( nError < 0 )
		{
			printk( "Error: afs_remove_from_deleted_list() Failed to load delete-me inode\n" );
			return ( nError );
		}
		nError = bt_delete_key( psVolume, psDeleteMe, zFileName, strlen( zFileName ) );
		kfree( psDeleteMe );
		if( nError < 0 )
		{
			printk( "Error: afs_remove_from_deleted_list() Failed to delete file from delete-me dir.\n" );
			return ( nError );
		}
	}
	else
	{
		printk( "afs_remove_from_deleted_list() inode %Ld not in the delete-me directory\n", nInodeNum );
	}
	nError = afs_delete_inode( psVolume, psInode );
	if( nError < 0 )
	{
		printk( "Error: afs_remove_from_deleted_list() Failed to delete inode\n" );
	}
	return ( nError );
}

/** Get the total size of a device
 * \par Description:
 * Given a device number, get the total size in bytes of the device
 * \par Note:
 * \par Warning:
 * \param nDev	Device number
 * \return Size of device in octets
 * \sa
 *****************************************************************************/
static off_t afs_get_device_size( int nDev )
{
	device_geometry sGeo;
	int nError = ioctl( nDev, IOCTL_GET_DEVICE_GEOMETRY, &sGeo );

	if( nError >= 0 )
	{
		return ( sGeo.sector_count * sGeo.bytes_per_sector );
	}
	else
	{
		struct stat sStat;

		nError = fstat( nDev, &sStat );
		if( nError < 0 )
		{
			return ( nError );
		}
		return ( sStat.st_size );
	}
}

/** Initialize an AFS volume
 * \par Description:
 * Initialzie (format) a volume represented by the given device path with
 * the AFS filesystem.
 * \par Note:
 * \par Warning:
 * \param pzDevPath	Path to device to format
 * \param pzVolName	Name of new filesystem
 * \param pArgs		Initialization arguments (? unused)
 * \param nArgLen	Size of pArgs
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_initialize( const char *pzDevPath, const char *pzVolName, void *pArgs, size_t nArgLen )
{
	AfsSuperBlock_s * psSuperBlock;
	int nBitmapBlkCnt;
	uint32 *panBitmap;

	AfsInode_s * psInode;
	int nBlockSize = 1024;
	int64 nNumBlocks = 1024 * 1024;
	int nLogSize = 2048;
	int nBootLoaderSize = 32;
	int nDev;
	off_t nBitmapPos;
	off_t nCurPos;
	int i;

	nDev = open( pzDevPath, O_RDWR );
	if( nDev < 0 )
	{
		printk( "Error: afs_initialize() failed to open device %s\n", pzDevPath );
		return ( nDev );
	}
	nNumBlocks = afs_get_device_size( nDev );
	if( nNumBlocks < 0 )
	{
		printk( "afs_initialize() failed to get device size : %Ld\n", nNumBlocks );
		close( nDev );
		return ( nNumBlocks );
	}
	nNumBlocks /= nBlockSize;
	if( nNumBlocks < 3000 )
	{
		printk( "Error: afs_initialize() Disk too small, can't initialize\n" );
		close( nDev );
		return ( -EINVAL );
	}
	kassertw( 1024 == nBlockSize || 2048 == nBlockSize || 4096 == nBlockSize || 8192 == nBlockSize );
	nBitmapBlkCnt = ( nNumBlocks + nBlockSize * 8 - 1 ) / ( nBlockSize * 8 );
	psSuperBlock = kmalloc( nBlockSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( NULL == psSuperBlock )
	{
		close( nDev );
		return ( -ENOMEM );
	}
	psInode = kmalloc( nBlockSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( NULL == psInode )
	{
		kfree( psSuperBlock );
		close( nDev );
		return ( -ENOMEM );
	}
	panBitmap = kmalloc( nBlockSize * nBitmapBlkCnt, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( NULL == panBitmap )
	{
		kfree( psSuperBlock );
		kfree( psInode );
		close( nDev );
		return ( -ENOMEM );
	}

		/* +--------------------------+
		 * |       Boot block         |
		 * |         0-1023           |
		 * +--------------------------+
		 * |       Super block        |
		 * |       1024 - 2047        |
		 * +~~~~~~~~~~~~~~~~~~~~~~~~~~+
		 * |    Boot loader area      |
		 * +~~~~~~~~~~~~~~~~~~~~~~~~~~+
		 * |      Bitmap blocks       |
		 * +--------------------------+
		 * |     Journal blocks       |
		 * +--------------------------+
		 */
		memset( psSuperBlock, 0, nBlockSize );
	memset( psInode, 0, nBlockSize );
	memset( panBitmap, 0, nBlockSize * nBitmapBlkCnt );
	strncpy( psSuperBlock->as_zName, pzVolName, 31 );
	psSuperBlock->as_zName[31] = '\0';
	psSuperBlock->as_nMagic1 = SUPER_BLOCK_MAGIC1;
	psSuperBlock->as_nByteOrder = BO_LITTLE_ENDIAN;
	psSuperBlock->as_nBlockSize = nBlockSize;
	for( psSuperBlock->as_nBlockShift = 0; ( 1 << psSuperBlock->as_nBlockShift ) < nBlockSize; ++psSuperBlock->as_nBlockShift )


	  /*** EMPTY ***/ ;
	kassertw( ( 1 << psSuperBlock->as_nBlockShift ) == nBlockSize );
	nBitmapPos = ( nBootLoaderSize * nBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + nBlockSize - 1 ) / nBlockSize;
	nCurPos = nBitmapPos + nBitmapBlkCnt;
	psSuperBlock->as_nNumBlocks = nNumBlocks;
	psSuperBlock->as_nUsedBlocks = 1 + nBitmapBlkCnt + 1;	// super block + bitmap + root dir
	psSuperBlock->as_nBootLoaderSize = nBootLoaderSize;
	psSuperBlock->as_nInodeSize = nBlockSize;
	psSuperBlock->as_nMagic2 = SUPER_BLOCK_MAGIC2;
	psSuperBlock->as_nBlockPerGroup = nBlockSize * 8;	// Number of blocks per allocation group (Max 65536)
	// Number of bits to shift a group number to get a byte address.
	for( psSuperBlock->as_nAllocGrpShift = 0; ( 1 << psSuperBlock->as_nAllocGrpShift ) < psSuperBlock->as_nBlockPerGroup * nBlockSize; psSuperBlock->as_nAllocGrpShift++ )
	{


	  /*** EMPTY ***/ ;
	}
	psSuperBlock->as_nAllocGroupCount = ( nNumBlocks + psSuperBlock->as_nBlockPerGroup - 1 ) / psSuperBlock->as_nBlockPerGroup;
	psSuperBlock->as_nFlags = 0;
	psSuperBlock->as_sLogBlock.group = 0;
	psSuperBlock->as_sLogBlock.start = nCurPos;
	psSuperBlock->as_sLogBlock.len = nLogSize;
	psSuperBlock->as_nLogStart = nCurPos;
	psSuperBlock->as_nValidLogBlocks = 0;
	psSuperBlock->as_nLogSize = nLogSize;
	nCurPos += nLogSize;
	psSuperBlock->as_nMagic3 = SUPER_BLOCK_MAGIC3;
	psSuperBlock->as_nUsedBlocks = nCurPos + 3;
	if( psSuperBlock->as_nNumBlocks < psSuperBlock->as_nBlockPerGroup * 8 + 4 )

	{
		psSuperBlock->as_sRootDir.group = 0;
		psSuperBlock->as_sRootDir.start = nCurPos++;
		psSuperBlock->as_sRootDir.len = 1;
		psSuperBlock->as_sIndexDir.group = 0;
		psSuperBlock->as_sIndexDir.start = nCurPos++;
		psSuperBlock->as_sIndexDir.len = 1;
		psSuperBlock->as_sDeletedFiles.group = 0;
		psSuperBlock->as_sDeletedFiles.start = nCurPos++;
		psSuperBlock->as_sDeletedFiles.len = 1;
	}

	else

	{
		psSuperBlock->as_sRootDir.group = 8;
		psSuperBlock->as_sRootDir.start = 0;
		psSuperBlock->as_sRootDir.len = 1;
		psSuperBlock->as_sIndexDir.group = 8;
		psSuperBlock->as_sIndexDir.start = 1;
		psSuperBlock->as_sIndexDir.len = 1;
		psSuperBlock->as_sDeletedFiles.group = 8;
		psSuperBlock->as_sDeletedFiles.start = 2;
		psSuperBlock->as_sDeletedFiles.len = 1;
	}
	printk( "NumBlocks         = %Ld\n", psSuperBlock->as_nNumBlocks );
	printk( "Block shift       = %d\n", psSuperBlock->as_nBlockShift );
	printk( "Blocks per group  = %d\n", psSuperBlock->as_nBlockPerGroup );
	printk( "Alloc group shift = %d\n", psSuperBlock->as_nAllocGrpShift );
	printk( "Alloc group count = %d\n", psSuperBlock->as_nAllocGroupCount );
	printk( "Bitmap block count= %d\n", nBitmapBlkCnt );
	printk( "Prealloc super block, bitmap and root inode\n" );

		// Allocate super block, boot-loader area, bitmap blocks and the log.
		for( i = 0; i < nCurPos; ++i )
	{
		panBitmap[i / 32] |= 1 << ( i % 32 );
	}

		// Prealloc blocks in the last bitmap block that resides outside the disk
		for( i = nNumBlocks; i < nBitmapBlkCnt * nBlockSize * 8; ++i )
	{
		panBitmap[i / 32] |= 1 << ( i % 32 );
	}
	i = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sRootDir );
	panBitmap[i / 32] |= 1 << ( i % 32 );	// Allocate root dir inode.
	i = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sIndexDir );
	panBitmap[i / 32] |= 1 << ( i % 32 );	// Allocate index dir inode.
	i = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sDeletedFiles );
	panBitmap[i / 32] |= 1 << ( i % 32 );	// Allocate delete-me dir inode.
	printk( "Write super block\n" );
	write_pos( nDev, 1024, psSuperBlock, AFS_SUPERBLOCK_SIZE );
	write_pos( nDev, nBitmapPos * nBlockSize, panBitmap, nBitmapBlkCnt * nBlockSize );
	psInode->ai_nMagic1 = INODE_MAGIC;
	psInode->ai_nUID = 0;
	psInode->ai_nGID = 0;
	psInode->ai_nFlags = INF_ATTRIBUTES | INF_LOGGED | INF_USED;
	psInode->ai_nMode = S_IFDIR | S_IRWXUGO;
	psInode->ai_nIndexType = e_KeyTypeString;
	atomic_set( &psInode->ai_nLinkCount, 1 );
	psInode->ai_nCreateTime = get_real_time();
	psInode->ai_nModifiedTime = psInode->ai_nCreateTime;
	psInode->ai_nInodeSize = nBlockSize;
	psInode->ai_sInodeNum = psSuperBlock->as_sRootDir;
	write_pos( nDev, afs_run_to_num( psSuperBlock, &psInode->ai_sInodeNum ) * nBlockSize, psInode, nBlockSize );
	psInode->ai_sInodeNum = psSuperBlock->as_sIndexDir;
	write_pos( nDev, afs_run_to_num( psSuperBlock, &psInode->ai_sInodeNum ) * nBlockSize, psInode, nBlockSize );
	psInode->ai_sInodeNum = psSuperBlock->as_sDeletedFiles;
	write_pos( nDev, afs_run_to_num( psSuperBlock, &psInode->ai_sInodeNum ) * nBlockSize, psInode, nBlockSize );

		// Clear the boot-block to avoid misdetection of the FS later.
		memset( psSuperBlock, 0, 1024 );
	write_pos( nDev, 0, psSuperBlock, 1024 );
	kfree( psSuperBlock );
	kfree( panBitmap );
	kfree( psInode );
	printk( "AFS volume %s formatted\n", pzVolName );
	close( nDev );
	return ( 0 );
}

/** Check the validity of a superblock
 * \par Description:
 * Given an AFS superblock, check it for internal consistancy.
 * \par Note:
 * \par Warning:
 * \param psSuperBlock	AFS superblock to check
 * \param bQuiet		Whether or not to log errors
 * \return true if superblock is valid, false otherwise
 * \sa
 *****************************************************************************/
static bool afs_validate_superblock( AfsSuperBlock_s * psSuperBlock, bool bQuiet )
{
	bool bResult = true;

	if( psSuperBlock->as_nMagic1 != SUPER_BLOCK_MAGIC1 )
	{
		if( bQuiet )
			return ( false );
		printk( "Invalid super block magic1 = %x\n", psSuperBlock->as_nMagic1 );
		bResult = false;
	}
	if( psSuperBlock->as_nMagic2 != SUPER_BLOCK_MAGIC2 )
	{
		if( bQuiet )
			return ( false );
		printk( "Invalid super block magic2 = %x\n", psSuperBlock->as_nMagic2 );
		bResult = false;
	}
	if( psSuperBlock->as_nMagic3 != SUPER_BLOCK_MAGIC3 )
	{
		if( bQuiet )
			return ( false );
		printk( "Invalid super block magic3 = %x\n", psSuperBlock->as_nMagic3 );
		bResult = false;
	}
	if( psSuperBlock->as_nByteOrder != BO_LITTLE_ENDIAN && psSuperBlock->as_nByteOrder != BO_BIG_ENDIAN )
	{
		if( bQuiet )
			return ( false );
		printk( "Bad value in super block byte-order field %d\n", psSuperBlock->as_nByteOrder );
		bResult = false;
	}
	if( 1 << psSuperBlock->as_nBlockShift != psSuperBlock->as_nBlockSize )
	{
		if( bQuiet )
			return ( false );
		printk( "Inconsistent values in block-shift and block-size values of superblock (sh=%d/si=%d)\n", psSuperBlock->as_nBlockShift, psSuperBlock->as_nBlockSize );
		bResult = false;
	}
	if( psSuperBlock->as_nUsedBlocks > psSuperBlock->as_nNumBlocks )
	{
		if( bQuiet )
			return ( false );
		printk( "Super block has more free blocks than actual blocks (%Ld - %Ld)\n", psSuperBlock->as_nUsedBlocks, psSuperBlock->as_nNumBlocks );
		bResult = false;
	}
	if( psSuperBlock->as_nInodeSize != psSuperBlock->as_nBlockSize )
	{
		if( bQuiet )
			return ( false );
		printk( "Inconsistency between block-size (%d) and inode-size (%d)\n", psSuperBlock->as_nBlockSize, psSuperBlock->as_nInodeSize );
		bResult = false;
	}
	if( 0 == psSuperBlock->as_nBlockSize || ( 1 << psSuperBlock->as_nAllocGrpShift ) / psSuperBlock->as_nBlockSize != psSuperBlock->as_nBlockPerGroup )
	{
		if( bQuiet )
			return ( false );
		printk( "Inconsistency between blocks per ag (%d) and ag shift count (%d)\n", psSuperBlock->as_nBlockPerGroup, psSuperBlock->as_nAllocGrpShift );
		bResult = false;
	}
	if( psSuperBlock->as_nAllocGroupCount * psSuperBlock->as_nBlockPerGroup < psSuperBlock->as_nNumBlocks )
	{
		if( bQuiet )
			return ( false );
		printk( "Alloc group count (%d) does not cover entire disk (%Ld)\n", psSuperBlock->as_nAllocGroupCount, psSuperBlock->as_nNumBlocks );
		bResult = false;
	}
	if( psSuperBlock->as_sLogBlock.len != psSuperBlock->as_nLogSize )
	{
		if( bQuiet )
			return ( false );
		printk( "Mismatch between length in journal run %d and the length recorded in the superblock %d\n", psSuperBlock->as_sLogBlock.len, psSuperBlock->as_nLogSize );
		bResult = false;
	}
	if( psSuperBlock->as_nValidLogBlocks > psSuperBlock->as_nLogSize )
	{
		if( bQuiet )
			return ( false );
		printk( "Superblock has more valid journal blocks (%d) than the total journal size (%d)\n", psSuperBlock->as_nValidLogBlocks, psSuperBlock->as_nLogSize );
		bResult = false;
	}
	return ( bResult );
}

/** Clean out deleted file directory
 * \par Description:
 * When files are deleted, they are moved to a hidden directory of deleted
 * files, rather than being deleted entirely.  This will delete the entire
 * contents of the deleted file directory for the given volume.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_empty_delme_dir( AfsVolume_s * psVolume )
{
	AfsSuperBlock_s * psSuperBlock;
	AfsInode_s * psDeleteMe;
	int nError;


#ifdef AFS_PARANOIA
		if( psVolume == NULL )
	{
		panic( "afs_empty_delme_dir() called with psVolume == NULL\n" );
		return ( -EINVAL );
	}

#endif	/*  */
		psSuperBlock = psVolume->av_psSuperBlock;
	nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sDeletedFiles, &psDeleteMe );
	if( nError < 0 )
	{
		return ( nError );
	}
	for( ;; )

	{
		BIterator_s sIterator;
		ino_t nInodeNum;

		BlockRun_s sInodeNum;
		AfsInode_s * psInode;
		char zFileName[32];
		off_t nOldSize;

		nError = afs_begin_transaction( psVolume );
		if( nError < 0 )
		{
			printk( "Error: afs_empty_delme_dir() failed to start transaction %d\n", nError );
			break;
		}
		nError = bt_find_first_key( psVolume, psDeleteMe, &sIterator );
		if( nError < 0 )
		{
			if( nError != -ENOENT )
			{
				printk( "PANIC: afs_empty_delme_dir() bt_find_first_key() failed %d\n", nError );
			}
			else
			{
				nError = 0;
			}
			afs_end_transaction( psVolume, false );
			break;
		}
		if( psVolume->av_nFlags & FS_IS_READONLY )
		{
			nError = -EROFS;
			break;
		}
		nInodeNum = sIterator.bi_nCurValue;
		afs_num_to_run( psVolume->av_psSuperBlock, &sInodeNum, nInodeNum );
		nError = afs_do_read_inode( psVolume, &sInodeNum, &psInode );
		if( nError < 0 )
		{
			printk( "Error: afs_empty_delme_dir() failed to load inode %Ld %d\n", nInodeNum, nError );
			afs_end_transaction( psVolume, false );
			break;
		}
		kassertw( atomic_read( &psInode->ai_nLinkCount ) == 0 );
		printk( "afs_empty_delme_dir() delete inode %Ld (size=%Ld)\n", nInodeNum, psInode->ai_sData.ds_nSize );
		sprintf( zFileName, "%08x%08x.del", ( int )( nInodeNum >> 32 ), ( int )( nInodeNum & 0xffffffff ) );
		if( atomic_read( &psInode->ai_nLinkCount ) != 0 )
		{
			printk( "PANIC: Inode has link count of %d\n", atomic_read( &psInode->ai_nLinkCount ) );
			printk( "PANIC: Parent is %d:%d:%d\n", psInode->ai_sParent.group, psInode->ai_sParent.start, psInode->ai_sParent.len );
			printk( "PANIC: UID=%d GID=%d mode=%08x\n", psInode->ai_nUID, psInode->ai_nGID, psInode->ai_nMode );
			afs_end_transaction( psVolume, false );
			kfree( psInode );
			break;
		}
		nError = afs_delete_file_attribs( psVolume, psInode, true );
		if( nError < 0 )
		{
			printk( "PANIC : afs_empty_delme_dir() Failed to delete file attributes %d\n", nError );
			afs_end_transaction( psVolume, false );
			kfree( psInode );
			break;
		}
		nOldSize = psInode->ai_sData.ds_nSize;
		psInode->ai_sData.ds_nSize = 0;
		nError = afs_truncate_stream( psVolume, psInode );
		if( nError < 0 )
		{
			psInode->ai_sData.ds_nSize = nOldSize;
			printk( "Error: afs_empty_delme_dir() failed to truncate file %d\n", nError );
			afs_end_transaction( psVolume, false );
			kfree( psInode );
			break;
		}
		nError = bt_delete_key( psVolume, psDeleteMe, zFileName, strlen( zFileName ) );
		if( nError < 0 )
		{
			printk( "Error: afs_empty_delme_dir() Failed to delete file. %d\n", nError );
			afs_end_transaction( psVolume, false );
			kfree( psInode );
			break;
		}
		nError = afs_delete_inode( psVolume, psInode );
		if( nError < 0 )
		{
			printk( "Error: afs_empty_delme_dir() Failed to delete inode %d\n", nError );
			afs_end_transaction( psVolume, false );
			kfree( psInode );
			break;
		}
		afs_end_transaction( psVolume, true );
	}
	kfree( psDeleteMe );
	return ( nError );
}

/** Determine if a filesystem is AFS
 * \par Description:
 * Given a device containing a filesystem, determine if that filesystem is
 * AFS and controllable by this driver.  If this is an AFS filesystem,
 * then psInfo is filled in with appropriate information.
 * \par Note:
 * \par Warning:
 * \param pzDevPath	The path to the device to probe
 * \param psInfo	fs_info structure to fill on success
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_probe( const char *pzDevPath, fs_info * psInfo )
{
	AfsSuperBlock_s * psSuperBlock;
	int nDev;
	int nError = 0;

	nDev = open( pzDevPath, O_RDONLY );
	if( nDev < 0 )
	{
		printk( "Error: afs_probe() failed to open device %s\n", pzDevPath );
		return ( nDev );
	}
	psSuperBlock = kmalloc( AFS_SUPERBLOCK_SIZE, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( psSuperBlock == NULL )
	{
		printk( "Error : afs_probe() Failed to alloc memory for super block\n" );
		nError = -ENOMEM;
		goto error1;
	}
	if( read_pos( nDev, 1024, psSuperBlock, AFS_SUPERBLOCK_SIZE ) != AFS_SUPERBLOCK_SIZE )
	{
		nError = -EIO;
		goto error2;
	}
	if( afs_validate_superblock( psSuperBlock, true ) == false )
	{
		if( read_pos( nDev, 0, psSuperBlock, AFS_SUPERBLOCK_SIZE ) != AFS_SUPERBLOCK_SIZE )
		{
			printk( "afs_probe() Failed to read super block\n" );
			nError = -EIO;
			goto error2;
		}
	}
	if( afs_validate_superblock( psSuperBlock, true ) == false )
	{
		nError = -EINVAL;
		goto error2;
	}
	psInfo->fi_dev = -1;
	psInfo->fi_root = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sRootDir );
	psInfo->fi_flags = FS_IS_PERSISTENT | FS_IS_BLOCKBASED | FS_CAN_MOUNT | FS_HAS_MIME | FS_HAS_ATTR;
	psInfo->fi_block_size = psSuperBlock->as_nBlockSize;
	psInfo->fi_io_size = 65536;
	psInfo->fi_total_blocks = psSuperBlock->as_nNumBlocks;
	psInfo->fi_free_blocks = psSuperBlock->as_nNumBlocks - psSuperBlock->as_nUsedBlocks;
	psInfo->fi_free_user_blocks = psInfo->fi_free_blocks;
	psInfo->fi_total_inodes = -1;
	psInfo->fi_free_inodes = -1;
	strncpy( psInfo->fi_volume_name, psSuperBlock->as_zName, 255 );
	psInfo->fi_volume_name[255] = '\0';
	nError = 0;
      error2:kfree( psSuperBlock );
      error1:close( nDev );
	return ( nError );
}

/** Mount an AFS filesystem
 * \par Description:
 * Attempt to mount an AFS filesystem.
 * \par Note:
 * \par Warning:
 * \param nFsID		Filesystem handle for filesystem to mount
 * \param pzDevPath	Path to device to mount
 * \param nFlags	Mount flags (MNTF_*)
 * \param pArgs		Mount argument (? unused)
 * \param pArgLen	Mount argument length
 * \param ppVolData	AFS volume pointer to fill
 * \param pnRootIno	Root inode of new filesystem
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_mount( kdev_t nFsID, const char *pzDevPath, uint32 nFlags, const void *pArgs, int nArgLen, void **ppVolData, ino_t *pnRootIno )
{
	AfsVolume_s * psVolume;
	AfsSuperBlock_s * psSuperBlock;
	int64 nUsedBlocks;
	int nBlockSize;
	int nDev;
	int nError = 0;

	printk( "Mount afs on '%s' flags %08lx\n", pzDevPath, ( unsigned int )nFlags );
	nDev = open( pzDevPath, O_RDWR );
	if( nDev < 0 )
	{
		printk( "Error: afs_mount() failed to open device %s\n", pzDevPath );
		return ( nDev );
	}
	psVolume = kmalloc( sizeof( AfsVolume_s ), MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( psVolume == NULL )
	{
		printk( "Error : Failed to alloc memory for volume\n" );
		nError = -ENOMEM;
		goto error1;
	}
	memset( psVolume, 0, sizeof( AfsVolume_s ) );
	nError = afs_init_hash_table( &psVolume->av_sLogHashTable );
	if( nError < 0 )
	{
		goto error2;
	}
	psSuperBlock = kmalloc( AFS_SUPERBLOCK_SIZE, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( psSuperBlock == NULL )
	{
		printk( "Error : Failed to alloc memory for super block\n" );
		nError = -ENOMEM;
		goto error3;
	}
	psVolume->av_psSuperBlock = psSuperBlock;
	printk( "Load super block\n" );
	psVolume->av_nSuperBlockPos = 1024;
	if( read_pos( nDev, psVolume->av_nSuperBlockPos, psSuperBlock, AFS_SUPERBLOCK_SIZE ) != AFS_SUPERBLOCK_SIZE )
	{
		nError = -EIO;
		printk( "Failed to read super block\n" );
		goto error4;
	}
	if( afs_validate_superblock( psSuperBlock, true ) == false )
	{
		psVolume->av_nSuperBlockPos = 0;
		if( read_pos( nDev, psVolume->av_nSuperBlockPos, psSuperBlock, AFS_SUPERBLOCK_SIZE ) != AFS_SUPERBLOCK_SIZE )
		{
			printk( "Failed to read super block\n" );
			nError = -EIO;
			goto error4;
		}
	}
	printk( "Validate super block\n" );
	if( afs_validate_superblock( psSuperBlock, false ) == false )

	{
		printk( "Invalid super block\n" );
		nError = -EINVAL;
		goto error4;
	}
	nBlockSize = psSuperBlock->as_nBlockSize;
	if( psVolume->av_nSuperBlockPos == 0 )
	{
		printk( "Warning: afs_mount() old filesystem format\n" );
		psVolume->av_nBitmapStart = 1;
	}
	else
	{
		psVolume->av_nBitmapStart = ( psSuperBlock->as_nBootLoaderSize * nBlockSize + AFS_SUPERBLOCK_SIZE + 1024 + nBlockSize - 1 ) / nBlockSize;
	}
	psVolume->av_nJournalSize = psSuperBlock->as_nLogSize;
	psVolume->av_nLoggStart = psSuperBlock->as_nLogStart;
	psVolume->av_nLoggEnd = psVolume->av_nLoggStart + psVolume->av_nJournalSize - 1;
	printk( "Log start: %Ld BitmapStart : %Ld\n", psVolume->av_nLoggStart, psVolume->av_nBitmapStart );

//    printk( "Bitmap : %Ld -> %Ld  Log : %Ld -> %Ld\n", psVolume->av_nBitmapStart, psVolume->av_nBitmapStart + psSuperBlock->as_nAllocGroupCount );
		psVolume->av_nFsID = nFsID;
	psVolume->av_nDevice = nDev;
	nError = setup_device_cache( nDev, nFsID, psSuperBlock->as_nNumBlocks );
	if( nError < 0 )
	{
		printk( "Error: afs_mount() failed to setup device cache\n" );
		goto error4;
	}
	psVolume->av_hJournalLock = create_semaphore( "afs_journal_mutex", 1, SEM_RECURSIVE );
	if( psVolume->av_hJournalLock < 0 )
	{
		printk( "Error : afs_mount() failed to create journal semaphore\n" );
		nError = psVolume->av_hJournalLock;
		goto error4;
	}
	psVolume->av_hJournalListsLock = create_semaphore( "afs_journal_lists_mutex", 1, SEM_RECURSIVE );
	if( psVolume->av_hJournalLock < 0 )
	{
		printk( "Error : afs_mount() failed to create journal lists semaphore\n" );
		nError = psVolume->av_hJournalLock;
		goto error5;
	}
	psVolume->av_hBlockListMutex = create_semaphore( "afs_buffer_lock", 1, SEM_RECURSIVE );
	if( psVolume->av_hBlockListMutex < 0 )
	{
		printk( "Error : afs_mount() failed to create block list semaphore\n" );
		nError = psVolume->av_hBlockListMutex;
		goto error6;
	}
	psVolume->av_hIndexDirMutex = create_semaphore( "afs_buffer_lock", 1, SEM_RECURSIVE );
	if( psVolume->av_hIndexDirMutex < 0 )
	{
		printk( "Error : afs_mount() failed to create file-index semaphore\n" );
		nError = psVolume->av_hIndexDirMutex;
		goto error7;
	}
	if( nFlags & MNTF_READONLY )
	{
		psVolume->av_nFlags |= FS_IS_READONLY;
		printk( "Info: Setting fs read only\n" );
	}
	nError = afs_replay_log( psVolume );
	if( nError < 0 )
	{
		printk( "Error: afs_mount() failed to replay journal\n" );
		goto error8;
	}
	*ppVolData = psVolume;
	*pnRootIno = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sRootDir );
	nUsedBlocks = afs_count_used_blocks( psVolume );
	if( psSuperBlock->as_nUsedBlocks != nUsedBlocks )
	{
		printk( "Error: afs_mount() Inconsistency between super-block and allocation bitmap!\n" );
		printk( "Superblock: %Ld used blocks\n", psSuperBlock->as_nUsedBlocks );
		printk( "Bitmap:     %Ld used blocks\n", nUsedBlocks );
		psSuperBlock->as_nUsedBlocks = nUsedBlocks;
	}
	printk( "afs_mount() : filesystem has %Ld free blocks (of %Ld)\n", ( psSuperBlock->as_nNumBlocks - psSuperBlock->as_nUsedBlocks ), psSuperBlock->as_nNumBlocks );
	nError = afs_empty_delme_dir( psVolume );
	if( nError < 0 )
	{
		printk( "Error: failed to cleanup delete-me directory! Degrading mount to read only\n" );
		psVolume->av_nFlags |= FS_IS_READONLY;
	}
	psVolume->av_bRunJournalFlusher = true;
	nError = afs_start_journal_flusher( psVolume );
	if( nError < 0 )
	{
		printk( "afs_mount() failed to start journal flushing thread\n" );
		goto error8;
	}
	printk( "AFS filesystem successfully mounted\n" );
	return ( 0 );
      error8:delete_semaphore( psVolume->av_hIndexDirMutex );
      error7:delete_semaphore( psVolume->av_hBlockListMutex );
      error6:delete_semaphore( psVolume->av_hJournalListsLock );
      error5:delete_semaphore( psVolume->av_hJournalLock );
      error4:kfree( psSuperBlock );
      error3:afs_delete_hash_table( &psVolume->av_sLogHashTable );
      error2:kfree( psVolume );
      error1:printk( "afs_mount() Flush cache blocks for our device\n" );
	flush_device_cache( nDev, false );
	printk( "afs_mount() Shut down cache for our device\n" );
	shutdown_device_cache( nDev );
	close( nDev );
	return ( nError );
}



/** Unmount an AFS filesystem
 * \par Description:
 * Unmount an AFS filesystem.  Flush all outstanding IO, free all state.
 * \par Note:
 * \par Warning:
 * \param pVolume	Pointer to private filesystem data of filesystem to unmount
 * \return 0 (success)
 * \sa
 *****************************************************************************/
static int afs_unmount( void *pVolume )
{
	AfsVolume_s * psVolume = pVolume;
	off_t nUsedBlocks;

	nUsedBlocks = afs_count_used_blocks( psVolume );
	if( psVolume->av_psSuperBlock->as_nUsedBlocks != nUsedBlocks )
	{
		printk( "Error: afs_unmount() Inconsistency between super-block and allocation bitmap!\n" );
		printk( "Superblock: %Ld used blocks\n", psVolume->av_psSuperBlock->as_nUsedBlocks );
		printk( "Bitmap:     %Ld used blocks\n", nUsedBlocks );
	}
	printk( "afs_unmount() : filesystem has %Ld free blocks\n", ( psVolume->av_psSuperBlock->as_nNumBlocks - psVolume->av_psSuperBlock->as_nUsedBlocks ) );
	printk( "afs_unmount() stop the journal flushing thread\n" );
	psVolume->av_bRunJournalFlusher = false;
	printk( "afs_unmount() wait for the journal flushing thread to finish\n" );
	while( psVolume->av_bRunJournalFlusher == false )
	{
		snooze( 100000 );
	}
	if( atomic_read( &psVolume->av_nOpenFiles ) > 0 )
	{
		printk( "WARNING : Attempt to unmount AFS filesystem while %d files are open!\n", atomic_read( &psVolume->av_nOpenFiles ) );
	}
	printk( "afs_unmount() Shutting down the journal\n" );
	afs_flush_journal( psVolume );
	printk( "afs_unmount() Flush cache blocks for our device %d\n", psVolume->av_nDevice );
	flush_device_cache( psVolume->av_nDevice, false );
	printk( "afs_unmount() Shut down cache for our device\n" );
	shutdown_device_cache( psVolume->av_nDevice );
	afs_delete_hash_table( &psVolume->av_sLogHashTable );
	delete_semaphore( psVolume->av_hJournalLock );
	delete_semaphore( psVolume->av_hJournalListsLock );
	delete_semaphore( psVolume->av_hBlockListMutex );
	delete_semaphore( psVolume->av_hIndexDirMutex );
	close( psVolume->av_nDevice );
	kfree( psVolume->av_psSuperBlock );
	kfree( psVolume );
	return ( 0 );
}

/** Read filesystem info of an AFS filesystem
 * \par Description:
 * Read the filesystem info (device, free blocks, etc.) from the given
 * volume into the given filesystem info structure
 * \par Note:
 * \par Warning:
 * XXX should check to see if arguments are valid
 * \param pVolume	AFS filesystem pointer
 * \param psInfo	Filesystem info structure to fill
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_rfsstat( void *pVolume, fs_info * psInfo )
{
	AfsVolume_s * psVolume = pVolume;
	AfsSuperBlock_s * psSuperBlock = psVolume->av_psSuperBlock;
	psInfo->fi_dev = psVolume->av_nFsID;
	psInfo->fi_root = afs_run_to_num( psSuperBlock, &psSuperBlock->as_sRootDir );
	psInfo->fi_flags = FS_IS_PERSISTENT | FS_IS_BLOCKBASED | FS_HAS_MIME  |FS_HAS_ATTR | psVolume->av_nFlags;
	psInfo->fi_block_size = psSuperBlock->as_nBlockSize;
	psInfo->fi_io_size = 65536;
	psInfo->fi_total_blocks = psSuperBlock->as_nNumBlocks;
	psInfo->fi_free_blocks = psSuperBlock->as_nNumBlocks - psSuperBlock->as_nUsedBlocks;
	psInfo->fi_free_user_blocks = psInfo->fi_free_blocks;
	psInfo->fi_total_inodes = -1;
	psInfo->fi_free_inodes = -1;
	strncpy( psInfo->fi_volume_name, psSuperBlock->as_zName, 255 );
	ioctl( psVolume->av_nDevice, IOCTL_GET_DEVICE_PATH, psInfo->fi_device_path );
	psInfo->fi_volume_name[255] = '\0';
	return ( 0 );
}

/** Sync an AFS volume
 * \par Description:
 * Write out all outstainding IO of the given AFS volume, and flush all
 * outstanding journal transactions.
 * \par Note:
 * XXX Should the journal flush be after the cache flush?
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \return 0 (success)
 * \sa
 *****************************************************************************/
int afs_sync( void *pVolume )
{
	AfsVolume_s * psVolume = pVolume;
	afs_flush_journal( psVolume );
	flush_device_cache( psVolume->av_nDevice, false );
	return ( 0 );
}

/** Read stats of an AFS file
 * \par Description:
 * Read the stat information (owner, modification time, etc.) of the given
 * file into the given stat structure
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to stat
 * \param psStat	stat structure to fill
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	AfsVolume_s * psVolume = pVolume;
	AfsVNode_s *psVNode = pNode;

	AfsInode_s * psInode = psVNode->vn_psInode;
	if( NULL == psVolume )
	{
		printk( "PANIC : afs_rstat() called with psVolume == NULL!\n" );
		return ( -EINVAL );
	}
	if( NULL == psVNode )
	{
		printk( "PANIC : afs_rstat() called with psVNode == NULL!\n" );
		return ( -EINVAL );
	}
	if( NULL == psInode )
	{
		printk( "PANIC : afs_rstat() called with psVNode->vn_psInode == NULL!\n" );
		return ( -EINVAL );
	}
	AFS_LOCK( psVNode );
	psStat->st_dev = psVolume->av_nFsID;
	psStat->st_ino = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
	psStat->st_size = psInode->ai_sData.ds_nSize;
	psStat->st_mode = psInode->ai_nMode;
	psStat->st_atime = psInode->ai_nModifiedTime / 1000000;
	psStat->st_mtime = psInode->ai_nModifiedTime / 1000000;
	psStat->st_ctime = psInode->ai_nCreateTime / 1000000;
	psStat->st_nlink = atomic_read( &psInode->ai_nLinkCount );
	psStat->st_uid = psInode->ai_nUID;
	psStat->st_gid = psInode->ai_nGID;
	psStat->st_blksize = 16384;	//psVolume->av_psSuperBlock->as_nBlockSize;
	AFS_UNLOCK( psVNode );


/*
  if ( psInode->ai_sParent.len == 0 ) {
  printk( "afs_rstat() : filesystem has %d free blocks\n",
  (int)(psVolume->av_psSuperBlock->as_nNumBlocks - psVolume->av_psSuperBlock->as_nUsedBlocks) );
  }
  */
		return ( 0 );
}

/** Write the stats of an AFS file
 * \par Description:
 * Write the given stat info (owner, modification time, etc.) into the
 * given file
 * \par Note:
 * Atime is not currently kept
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to write to
 * \param psStat	Stat structure to write
 * \param nMask		Mask of attributes to actually write (WSTAT_*)
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_wstat( void *pVolume, void *pNode, const struct stat *psStat, uint32 nMask )
{
	AfsVolume_s * psVolume = pVolume;
	AfsVNode_s *psVNode = pNode;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	AFS_LOCK( psVNode );
	if( nMask & WSTAT_MODE )
	{
		psVNode->vn_psInode->ai_nMode = ( psVNode->vn_psInode->ai_nMode & S_IFMT ) | ( psStat->st_mode & ~S_IFMT );
	}

//  if ( nMask & WSTAT_ATIME ) {
//    psVNode->vn_psInode->ai_nModifiedTime = (bigtime_t)psStat->st_atime * 1000000LL;
//  }
		if( nMask & WSTAT_MTIME )
	{
		psVNode->vn_psInode->ai_nModifiedTime = ( bigtime_t )psStat->st_mtime * 1000000LL;
	}
	if( nMask & WSTAT_UID )
	{
		psVNode->vn_psInode->ai_nUID = psStat->st_uid;
	}
	if( nMask & WSTAT_GID )
	{
		psVNode->vn_psInode->ai_nGID = psStat->st_gid;
	}
	AFS_UNLOCK( psVNode );
	nError = afs_begin_transaction( psVolume );
	if( nError >= 0 )
	{
		nError = afs_do_write_inode( psVolume, psVNode->vn_psInode );
		notify_node_monitors( NWEVENT_STAT_CHANGED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psVNode->vn_psInode->ai_sInodeNum ), NULL, 0 );
		psVNode->vn_psInode->ai_nFlags &= ~INF_STAT_CHANGED;
		afs_end_transaction( psVolume, nError >= 0 );
	}
	else
	{
		printk( "PANIC : afs_wstat() failed to start new transaction! Err = %d\n", nError );
	}
	return ( nError );
}

/** Find an Inode on an AFS filesystem
 * \par Description:
 * Given a parent Inode and a filename, get the I-Node number of the file
 * \par Note:
 * \par Warning:
 * \param pVolume		AFS volume pointer
 * \param pParent		Inode for parent directory
 * \param pzName		Filename
 * \param nNameLen		Length of filename
 * \param pnInodeNum	Inode number to return
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_lookup( void *pVolume, void *pParent, const char *pzName, int nNameLen, ino_t *pnInodeNum )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	int nError = 0;

	if( nNameLen == 1 && pzName[0] == '.' )
	{
		*pnInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum );
		if( *pnInodeNum >= psVolume->av_psSuperBlock->as_nNumBlocks )
		{
			printk( "PANIC : afs_lookup( \".\" ) parent inode = %Ld (larger than the disk!)\n", ( *pnInodeNum ) );
			nError = -ENOENT;
		}
	}
	else if( nNameLen == 2 && pzName[0] == '.' && pzName[1] == '.' )
	{
		*pnInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sParent );
		if( *pnInodeNum >= psVolume->av_psSuperBlock->as_nNumBlocks )
		{
			printk( "PANIC : afs_lookup( \"..\" ) parent's parent inode = %Ld (larger than the disk!)\n", *pnInodeNum );
			nError = -ENOENT;
		}
		if( 0 == *pnInodeNum )
		{
			printk( "Error : afs_lookup( \"..\" ) called on root!\n" );
			nError = -ENOENT;
		}
	}
	else if( nNameLen == 13 && strncmp( pzName, "@@delete-me$$", 13 ) == 0 )
	{
		*pnInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psVolume->av_psSuperBlock->as_sDeletedFiles );
		printk( "WARNING : somebody plan to enter the magic delete-me directory!!\n" );
	}
	else
	{
		AFS_LOCK( psParent->ai_psVNode );
		nError = afs_lookup_inode( psVolume, psParent, pzName, nNameLen, pnInodeNum );
		AFS_UNLOCK( psParent->ai_psVNode );
	}
	return ( nError );
}

/** Move a file/directory to the deleted file directory
 * \par Description:
 * When files or directories are deleted, they are not acutally deleted
 * but are moved to a hidden directory of deleted files for later removal.
 * This will move the given file/directory to the deleted files directory.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psParent	Inode of parent directory
 * \param psInode	Inode to be moved
 * \param pzName	Name of file/directory to be moved
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_move_to_delme( AfsVolume_s * psVolume, AfsInode_s * psParent, AfsInode_s * psInode,  const char *pzName, int nNameLen )
{
	AfsSuperBlock_s * psSuperBlock = psVolume->av_psSuperBlock;
	ino_t nInodeNum;

	AfsInode_s * psDeleteMe;
	char zNewName[32];
	int nError;

	kassertw( atomic_read( &psInode->ai_nLinkCount ) == 0 );
	nInodeNum = afs_run_to_num( psSuperBlock, &psInode->ai_sInodeNum );
	sprintf( zNewName, "%08x%08x.del", ( int )( nInodeNum >> 32 ), ( int )( nInodeNum & 0xffffffff ) );
	nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sDeletedFiles, &psDeleteMe );
	if( nError < 0 )
	{
		printk( "Error: afs_move_to_delme() failed to read delme directory inode %d\n", nError );
		goto error;
	}
	nError = bt_insert_key( psVolume, psDeleteMe, zNewName, strlen( zNewName ), nInodeNum );
	kfree( psDeleteMe );
	if( nError < 0 )
	{
		printk( "Error: afs_move_to_delme() failed to insert file into delete-me directory\n" );


/*	if ( nError == -ENOSPC ) {
	    psInode->ai_nFlags |= INF_NOT_IN_DELME;
	    printk( "The disk is to full to move the file %s into the delete-me directory!\n", pzName );
	    printk( "This means that if the machine crash before the last process close it,\n" );
	    printk( "the blocks consumed by it will be lost :(\n" );
	} else {
	    goto error;
	}*/
			goto error;
	}
	nError = bt_delete_key( psVolume, psParent, pzName, nNameLen );
	if( nError < 0 )
	{
		printk( "Error: afs_move_to_delme() failed to delete old key %d\n", nError );
		goto error;
	}
	psInode->ai_sParent.len = 0;
	psInode->ai_sParent.group = 0;
	psInode->ai_sParent.start = 0;
	psInode->ai_nModifiedTime = get_real_time();
	nError = afs_do_write_inode( psVolume, psInode );
	if( nError < 0 )
	{
		psInode->ai_sParent = psParent->ai_sInodeNum;
		printk( "Error: afs_move_to_delme() failed to write inode: %d\n", nError );
	}
      error:return ( nError );
}

/** Create a new AFS file
 * \par Description:
 * Create a new file with the given name in the given directory, with the
 * given perms, and return the new inode and filehandle
 * \par Note:
 * \par Warning:
 * \param pVolume		AFS volume pointer
 * \param pParent		Inode of parent directory of new file
 * \param pzName		Name of new file
 * \param nNameLen		Length of pzName
 * \param nMode			Open mode flags
 * \param nPerms		Permission of new file
 * \param pnInodeNum	Returned inode number
 * \param ppCookie		Returned filehandle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_create( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nMode, int nPerms, ino_t *pnInodeNum, void **ppCookie )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	AfsInode_s * psInode;
	AfsFileCookie_s * psCookie;
	bool bParentTouched = false;
	int nError;

	if( pzName[0] == '.' && ( nNameLen == 1 || ( nNameLen == 2 && pzName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	psCookie = kmalloc( sizeof( AfsFileCookie_s ), MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( psCookie == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}
	psCookie->fc_sBIterator.bi_nCurNode = -1;
	psCookie->fc_nOMode = nMode;
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error: afs_create() failed to start transaction %d\n", nError );
		goto error2;
	}
	AFS_LOCK( psParent->ai_psVNode );
	if( atomic_read( &psParent->ai_nLinkCount ) > 0 )
	{
		bParentTouched = true;
		nError = afs_create_inode( psVolume, psParent, ( nPerms & 0777 ) | S_IFREG, 0, 0, pzName, nNameLen, &psInode );
	}
	else
	{
		nError = -ENOENT;
	}
	if( nError < 0 )
	{
		afs_end_transaction( psVolume, false );
		if( bParentTouched )
		{
			afs_revert_inode( psVolume, psParent );
		}
	}
	else
	{
		afs_end_transaction( psVolume, true );
	}
	AFS_UNLOCK( psParent->ai_psVNode );
	if( nError < 0 )
	{
		goto error2;
	}
	*pnInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
	*ppCookie = psCookie;
	kfree( psInode );
	if( nError >= 0 )
	{
		notify_node_monitors( NWEVENT_CREATED, psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum ), 0, *pnInodeNum, pzName, nNameLen );
	}
	return ( 0 );
      error2:kfree( psCookie );
      error1:return ( nError );
}

/** Create an AFS symlink
 * \par Description:
 * Create a symlink on the given volume in the given parent directory with
 * the given name pointing to the given path
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pParent	Inode of parent directory
 * \param pzName	Name of new symlink
 * \param nNameLen	Length of pzName
 * \param pzNewPath	Path to destination of symlink
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_symlink( void *pVolume, void *pParent, const char *pzName, int nNameLen, const char *pzNewPath )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	const int nLen = strlen( pzNewPath );

	AfsInode_s * psInode;
	off_t nBlockCount;
	bool bParentTouched = false;
	bool bInodeCreated = false;
	int nError;
	ino_t nParent;
	ino_t nInode = 0;

	if( pzName[0] == '.' && ( nNameLen == 1 || ( nNameLen == 2 && pzName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error: afs_symlink() failed to start transaction %d\n", nError );
		return ( nError );
	}
	AFS_LOCK( psParent->ai_psVNode );
	nParent = afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum );
	if( atomic_read( &psParent->ai_nLinkCount ) > 0 )
	{
		bParentTouched = true;
		nError = afs_create_inode( psVolume, psParent, S_IFLNK | 0777, 0, 0, pzName, nNameLen, &psInode );
		if( nError >= 0 )
		{
			nInode = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
			bInodeCreated = true;
		}
	}
	else
	{
		nError = -ENOENT;
	}
	if( nError < 0 )
	{
		goto error;
	}
	nBlockCount = afs_get_inode_block_count( psInode );
	if( nLen > ( nBlockCount * nBlockSize ) )
	{
		nError = afs_expand_stream( psVolume, psInode, ( nLen + nBlockSize - 1 ) / nBlockSize - nBlockCount );
	}
	if( nError >= 0 )
	{
		nBlockCount = afs_get_inode_block_count( psInode );
		if( nLen <= ( nBlockCount * nBlockSize ) )
		{
			nError = afs_do_write( psVolume, psInode, pzNewPath, 0, nLen );
		}
		else
		{
			nError = -ENOSPC;
		}
	}
	if( nError >= 0 )
	{
		nError = afs_do_write_inode( psVolume, psInode );
	}
      error:if( bInodeCreated )
	{
		kfree( psInode );
	}
	if( nError < 0 )
	{
		afs_end_transaction( psVolume, false );
		if( bParentTouched )
		{
			afs_revert_inode( psVolume, psParent );
		}
	}
	else
	{
		afs_end_transaction( psVolume, true );
	}
	AFS_UNLOCK( psParent->ai_psVNode );
	if( nError >= 0 )
	{
		notify_node_monitors( NWEVENT_CREATED, psVolume->av_nFsID, nParent, 0, nInode, pzName, nNameLen );
		nError = 0;
	}
	return ( nError );
}

/** Make an AFS special node
 * \par Description:
 * Create a special node on an AFS filesystem.  This is typically a device
 * node.  Creates node in given parent directory, with given name and
 * mode, and for the given device.
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pParent	Inode of parent directory
 * \param pzName	Name of node to create
 * \param nNameLen	Length of pzName
 * \param nMode		Mode of node to create
 * \param nDev		Device to create node for
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_mknod( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nMode, dev_t nDev )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	ino_t nParent = 0;
	ino_t nInode = 0;
	bool bParentTouched = false;
	int nError;

	if( !( S_ISFIFO( nMode ) || S_ISREG( nMode ) ) )
	{
		printk( "Error: afs_mknod() can't make node with mode %08x\n", nMode );
		return ( -EINVAL );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	if( pzName[0] == '.' && ( nNameLen == 1 || ( nNameLen == 2 && pzName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error: afs_mknod() failed to start transaction %d\n", nError );
		return ( nError );
	}
	AFS_LOCK( psParent->ai_psVNode );
	if( atomic_read( &psParent->ai_nLinkCount ) > 0 )
	{
		AfsInode_s * psInode;
		bParentTouched = true;
		nError = afs_create_inode( psVolume, psParent, nMode & ( S_IFMT | S_IRWXUGO ), 0, 0, pzName, nNameLen, &psInode );
		if( nError >= 0 )
		{
			nParent = afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum );
			nInode = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
			kfree( psInode );
		}
	}
	else
	{
		nError = -ENOENT;
	}
	if( nError < 0 )
	{
		afs_end_transaction( psVolume, false );
		if( bParentTouched )
		{
			afs_revert_inode( psVolume, psParent );
		}
	}
	else
	{
		afs_end_transaction( psVolume, true );
	}
	AFS_UNLOCK( psParent->ai_psVNode );
	if( nError >= 0 )
	{
		notify_node_monitors( NWEVENT_CREATED, psVolume->av_nFsID, nParent, 0, nInode, pzName, nNameLen );
	}
	return ( nError );
}

/** Read an AFS symlink
 * \par Description:
 * Read the given symlink into the given buffer
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pNode		Inode of symlink to read
 * \param pzBuffer	Buffer to fill with symlink contents
 * \param nBufSize	Size of pzBuffer
 * \return amount read on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_read_link( void *pVolume, void *pNode, char *pzBuffer, size_t nBufSize )
{
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError;

	if( S_ISLNK( psInode->ai_nMode ) == false )
	{
		return ( -EINVAL );
	}
	AFS_LOCK( psInode->ai_psVNode );
	nError = afs_read_pos( pVolume, psInode, pzBuffer, 0, nBufSize );
	AFS_UNLOCK( psInode->ai_psVNode );
	return ( nError );
}

/** Read an AFS file
 * \par Description:
 * Read from the given file at the given position into the given buffer
 * the given amount.
 * \par Note:
 * File must be open
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to read
 * \param pCookie	AFS filehandle of file to read
 * \param nPos		Position to read from
 * \param pBuffer	Buffer to read into
 * \param nLen		number of octets to read
 * \return number of octets read on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nLen )
{
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		return ( -EISDIR );
	}
	AFS_LOCK( psInode->ai_psVNode );
	nError = afs_read_pos( pVolume, psInode, pBuffer, nPos, nLen );
	AFS_UNLOCK( psInode->ai_psVNode );
	return ( nError );
}

/** Read from an AFS file into a vector
 * \par Description:
 * Read from the given file the given amount into the given IO vector
 * \par Note:
 * File must be open
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to read from
 * \param pCookie	AFS filehandle of file to read from
 * \param nPos		Position to read from
 * \param psVector	Array of vectors to read into
 * \param nCount	Number of vectors to fill
 * \return number of octets read on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_readv( void *pVolume, void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError = 0;
	int nBytesRead = 0;
	int i;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		return ( -EISDIR );
	}
	AFS_LOCK( psInode->ai_psVNode );
	for( i = 0; i < nCount; ++i )
	{
		if( psVector[i].iov_len < 0 )
		{
			nError = -EINVAL;
			break;
		}
		nError = afs_read_pos( pVolume, psInode, psVector[i].iov_base, nPos, psVector[i].iov_len );
		if( nError < 0 )
		{
			break;
		}
		nBytesRead += nError;
		nPos += nError;
	}
	AFS_UNLOCK( psInode->ai_psVNode );
	return ( ( nError < 0 ) ? nError : nBytesRead );
}

/** Expand an AFS file
 * \par Description:
 * This will expand the given file to the length indicated by the given
 * offset and length.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode of file to expand
 * \param psCookie	AFS file handle for file to expand
 * \param pnPos		Position to add to (and returned position used)
 * \param pnLen		Length to add (and returned length added)
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_expand_file( AfsVolume_s * psVolume, AfsInode_s * psInode, AfsFileCookie_s * psCookie, off_t *pnPos, off_t *pnLen )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	off_t nPos = *pnPos;
	off_t nLen = *pnLen;
	int nError = 0;

	if( ( nPos + nLen ) > ( afs_get_inode_block_count( psInode ) * nBlockSize ) )

	{
		off_t nNewBlockCount = ( nPos + nLen + nBlockSize - 1 ) / nBlockSize;
		off_t nOldBlockCount;


			// We always take the journal lock before the inode lock.
			// Since we already have the inode locked we must unlock it
			// while starting the transaction (which will aquire the journal lock)
			// and then aquire the inode lock again.
			AFS_UNLOCK( psInode->ai_psVNode );
		nError = afs_begin_transaction( psVolume );
		AFS_LOCK( psInode->ai_psVNode );
		if( nError < 0 )
		{
			printk( "Error: afs_expand_file() failed to start a transaction %d\n", nError );
			*pnLen = 0;
			return ( nError );
		}
		if( psCookie != NULL && psCookie->fc_nOMode & O_APPEND )
		{
			nPos = psInode->ai_sData.ds_nSize;
			nNewBlockCount = ( nPos + nLen + nBlockSize - 1 ) / nBlockSize;
		}
		nOldBlockCount = afs_get_inode_block_count( psInode );
		if( nNewBlockCount > nOldBlockCount )
		{		// Check that nobody else expanded it while we unlocked it.
			off_t nDeltaSize = nNewBlockCount - nOldBlockCount + 64LL;

			nError = afs_expand_stream( psVolume, psInode, nDeltaSize );
			if( nError >= 0 /*|| -EFBIG == nError || -ENOSPC == nError */  )
			{
				nNewBlockCount = afs_get_inode_block_count( psInode );
				if( ( nNewBlockCount * nBlockSize ) < ( nPos + nLen ) )
				{
					off_t nTmp = nLen;

					if( nPos < nNewBlockCount * nBlockSize )
					{
						nLen = nNewBlockCount * nBlockSize - nPos;
					}
					else
					{
						nLen = 0;
					}
					printk( "Failed to expand file with %Ld bytes. Shrunk to %Ld\n", nTmp, nLen );
				}
			}
			else
			{
				printk( "afs_expand_file() failed to expand file %d\n", nError );
				nLen = 0;
			}
		}
		if( nLen <= 0 )
		{
			afs_end_transaction( psVolume, false );
			afs_revert_inode( psVolume, psInode );
		}
		else
		{
			afs_end_transaction( psVolume, true );
			notify_node_monitors( NWEVENT_STAT_CHANGED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), NULL, 0 );
			psInode->ai_nFlags &= ~INF_STAT_CHANGED;
		}
		*pnPos = nPos;
		*pnLen = nLen;
	}
	return ( nError );
}

/** Write to an AFS file
 * \par Description:
 * Write to the given file at the given location from the given buffer
 * \par Note:
 * File must be open
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of the file to write to
 * \param pCookie	AFS filehandle of file to write to
 * \param nPos		Position to write at
 * \param pBuffer	Buffer to write from
 * \param nLen		Amount to write (length of data in pBuffer)
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nLen )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	AfsFileCookie_s * psCookie = pCookie;
	int nError = 0;
	off_t nAFSLen = nLen;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		return ( -EISDIR );
	}
	if( S_ISREG( psInode->ai_nMode ) == false )
	{
		printk( "Error: afs_write() attempt to write to non-file (%08x)\n", psInode->ai_nMode );
		return ( -EINVAL );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	AFS_LOCK( psInode->ai_psVNode );

	// Set the position to the end of the file if opened in append mode
	if( psCookie != NULL && psCookie->fc_nOMode & O_APPEND )
	{
		nPos = psInode->ai_sData.ds_nSize;
	}

	// Check if we must expand the file.
	nError = afs_expand_file( psVolume, psInode, psCookie, &nPos, &nAFSLen );
	if( nAFSLen > 0 )
	{
		nLen = nAFSLen;
		nError = afs_do_write( pVolume, psInode, pBuffer, nPos, nLen );
		if( nError >= 0 )
		{
			nError = nLen;
		}
	}
	AFS_UNLOCK( psInode->ai_psVNode );
	return ( nError );
}

/** Write into an AFS file from a vector
 * \par Description:
 * Write into the given file at the given position the contents of the
 * given vectors
 * \par Note:
 * File must be open
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to write to
 * \param pCookie	AFS filehandle of file to write to
 * \param nPos		Position to write at
 * \param psVector	Array of IO vectors to write from
 * \param nCount	Number of vectors
 * \return number of octets written on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_writev( void *pVolume, void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	AfsFileCookie_s * psCookie = pCookie;
	int nError = 0;
	int nBytesWritten = 0;
	off_t nLen = 0;
	int i;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		return ( -EISDIR );
	}
	if( S_ISREG( psInode->ai_nMode ) == false )
	{
		printk( "Error: afs_writev() attempt to write to non-file (%08x)\n", psInode->ai_nMode );
		return ( -EINVAL );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	for( i = 0; i < nCount; ++i )
	{
		if( psVector[i].iov_len < 0 )
		{
			return ( -EINVAL );
		}
		nLen += psVector[i].iov_len;
	}
	AFS_LOCK( psInode->ai_psVNode );

		// Set the position to the end of the file if opened in append mode
		if( psCookie != NULL && psCookie->fc_nOMode & O_APPEND )
	{
		nPos = psInode->ai_sData.ds_nSize;
	}

		// Check if we must expand the file.
		nError = afs_expand_file( psVolume, psInode, psCookie, &nPos, &nLen );
	for( i = 0; i < nCount && nLen > 0; ++i )
	{
		size_t nCurLen = min( nLen, psVector[i].iov_len );

		nError = afs_do_write( pVolume, psInode, psVector[i].iov_base, nPos, nCurLen );
		if( nError < 0 )
		{
			break;
		}
		nBytesWritten += nCurLen;
		nPos += nCurLen;
		nLen -= nCurLen;
	}
	if( nError >= 0 )
	{
		nError = nBytesWritten;
	}
	AFS_UNLOCK( psInode->ai_psVNode );
	return ( nError );
}

/** Read the next entry from a directory
 * \par Description:
 * This will read the next entry from the given directory, as indicated by
 * the position or iterator into the given entry buffer
 * \par Note:
 * XXX No locking is done.
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		Inode of directory to read
 * \param psIterator	Iterator indicating next entry to read
 * \param nPos			Position of next entry to read
 * \param psFileInfo	Directory entry buffer to fill
 * \param nBufSize		Size of psFileInfo
 * \return
 * \sa
 *****************************************************************************/
int afs_do_read_dir( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator,  int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	int nError = 0;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		return ( -ENOTDIR );
	}
	if( 0 == nPos )
	{
		psFileInfo->d_name[0] = '.';
		psFileInfo->d_name[1] = '\0';
		psFileInfo->d_namlen = 1;
		psFileInfo->d_ino = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
		nError = 1;
	}
	else if( 1 == nPos )
	{
		psFileInfo->d_name[0] = '.';
		psFileInfo->d_name[1] = '.';
		psFileInfo->d_name[2] = '\0';
		psFileInfo->d_namlen = 2;
		psFileInfo->d_ino = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sParent );
		nError = 1;
	}
	else
	{
		if( 2 == nPos || ( nPos == -1 && psIterator->bi_nCurNode == -1 ) )
		{
			nError = bt_find_first_key( psVolume, psInode, psIterator );
		}
		else
		{
			nError = bt_get_next_key( psVolume, psInode, psIterator );
		}
		if( nError >= 0 )
		{
			memcpy( psFileInfo->d_name, psIterator->bi_anKey, psIterator->bi_nKeySize );
			psFileInfo->d_name[psIterator->bi_nKeySize] = '\0';
			psFileInfo->d_namlen = psIterator->bi_nKeySize;
			psFileInfo->d_ino = psIterator->bi_nCurValue;
			nError = 1;
		}
		else
		{
			if( nError == -ENOENT )
			{
				nError = 0;	/* End of directory */
			}
		}
	}
	return ( nError );
}

/** Read next entry from AFS directory
 * \par Description:
 * Reading a directory is an iterative process, with successive readdir
 * calls returning seccussive entries.  This will return that next entry.
 * \par Note:
 * \par Warning:
 * \param pVolume		AFS volume pointer
 * \param pNode			Inode of directory to read
 * \param pCookie		Pre-allocated directory reading cookie
 * \param nPos			Directory read position
 * \param psFileInfo	directory entry stucture to fill with read
 * \param nBufSize		Size of psFileInfo
 * \return 0 if directory empty, Positive number of entries read on success,
 * negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_read_dir( void *pVolume, void *pNode, void *pCookie, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	AfsVolume_s * psVolume = pVolume;
	AfsVNode_s *psVNode = pNode;

	AfsInode_s * psInode = psVNode->vn_psInode;
	AfsFileCookie_s * psCookie = pCookie;
	int nError;

	if( pCookie == NULL )
	{
		panic( "afs_read_dir() called with pCookie == NULL!\n" );
		return ( -EINVAL );
	}
	AFS_LOCK( psVNode );
	nError = afs_do_read_dir( psVolume, psInode, &psCookie->fc_sBIterator, nPos, psFileInfo, nBufSize );
	AFS_UNLOCK( psVNode );
	return ( nError );
}

/** Reset read of AFS directory to beginning
 * \par Description:
 * Reading a directory is an iterative process, with successive readdir
 * calls returning seccussive entries.  This will reset the read so the
 * next read will return the first entry.
 * \par Note:
 * XXX This does not currently do anything.  Check to see why (see comment
 * on vn_nBTreeVersion
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pNode		Inode of direcory to read
 * \param pCookie	Cookie for this directory read
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_rewinddir( void *pVolume, void *pNode, void *pCookie )
{
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		return ( -ENOTDIR );
	}
	else
	{
		return ( 0 );
	}
}

/** Open an AFS file
 * \par Description:
 * Open the given file in the given directory and return a filehandle
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pNode		Inode of parent directory
 * \param nMode		Open mode of file
 * \param ppCookie	Return parameter for AFS filehandle
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_open( void *pVolume, void *pNode, int nMode, void **ppCookie )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	AfsFileCookie_s * psCookie;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY && ( ( nMode & O_TRUNC ) || ( nMode & O_RWMASK ) || ( nMode & O_APPEND ) ) )
		return ( -EROFS );
	if( nMode & O_TRUNC && psInode->ai_sData.ds_nSize != 0 )
	{
		off_t nOldSize;

		if( S_ISDIR( psInode->ai_nMode ) )
		{
			nError = -EISDIR;
			goto error;
		}
		nError = afs_begin_transaction( psVolume );
		if( nError < 0 )
		{
			printk( "Error : afs_open() failed to start transaction\n" );
			goto error;
		}
		AFS_LOCK( psInode->ai_psVNode );
		nOldSize = psInode->ai_sData.ds_nSize;
		psInode->ai_sData.ds_nSize = 0;
		nError = afs_truncate_stream( psVolume, psInode );
		if( nError < 0 )
		{
			psInode->ai_sData.ds_nSize = nOldSize;
			afs_end_transaction( psVolume, false );
			afs_revert_inode( psVolume, psInode );
		}
		else
		{
			afs_end_transaction( psVolume, true );
		}
		AFS_UNLOCK( psInode->ai_psVNode );
		if( nError < 0 )
		{
			printk( "PANIC : afs_open() failed to truncate file!\n" );
			goto error;
		}
		else
		{
			notify_node_monitors( NWEVENT_STAT_CHANGED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), NULL, 0 );
			psInode->ai_nFlags &= ~INF_STAT_CHANGED;
		}
	}
	psCookie = kmalloc( sizeof( AfsFileCookie_s ), MEMF_KERNEL | MEMF_OKTOFAILHACK | MEMF_CLEAR );
	if( psCookie == NULL )
	{
		nError = -ENOMEM;
		goto error;
	}
	psCookie->fc_nOMode = nMode;
	psCookie->fc_sBIterator.bi_nCurNode = -1;
	*ppCookie = psCookie;
	return ( 0 );
      error:return ( nError );
}

/** Close an AFS file
 * \par Description:
 * Close a file that was previously opened.
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to close
 * \param pCookie	AFS filehandle of file to close
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_close( void *pVolume, void *pNode, void *pCookie )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError;

	if( pCookie != NULL )
	{
		kfree( pCookie );
	}
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error : afs_close() failed to start transaction\n" );
		return ( nError );
	}
	AFS_LOCK( psInode->ai_psVNode );
	nError = afs_truncate_stream( psVolume, psInode );
	if( nError < 0 )
	{
		afs_end_transaction( psVolume, false );
		afs_revert_inode( psVolume, psInode );
	}
	else
	{
		afs_end_transaction( psVolume, true );
	}
	AFS_UNLOCK( psInode->ai_psVNode );
	if( psInode->ai_nFlags & INF_STAT_CHANGED )
	{
		notify_node_monitors( NWEVENT_STAT_CHANGED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), NULL, 0 );
		psInode->ai_nFlags &= ~INF_STAT_CHANGED;
	}
	return ( nError );
}

/** Read an inode from an AFS filesystem
 * \par Description:
 * Given an Inode number, read the given inode in and pass it back
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param nInodeNum	The number of the Inode to read
 * \param ppNode	Pointer to return read Inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	AfsVolume_s * psVolume = pVolume;
	BlockRun_s sInodeNum;
	AfsVNode_s *psVNode;
	int nError;
	psVNode = kmalloc( sizeof( AfsVNode_s ), MEMF_KERNEL | MEMF_OKTOFAILHACK | MEMF_CLEAR );
	if( psVNode == NULL )
	{
		printk( "Error: afs_read_inode() failed to alloc memory for vnode!\n" );
		nError = -ENOMEM;
		goto error1;
	}
	psVNode->vn_nBTreeVersion = 0;
	afs_num_to_run( psVolume->av_psSuperBlock, &sInodeNum, nInodeNum );
	nError = afs_do_read_inode( psVolume, &sInodeNum, &psVNode->vn_psInode );
	if( nError < 0 )
	{
		printk( "Error: afs_read_inode() failed to load inode!\n" );
		goto error2;
	}
	psVNode->vn_hMutex = create_semaphore( "afs_inode_lock", 1, SEM_RECURSIVE );
	if( psVNode->vn_hMutex < 0 )
	{
		printk( "Error: afs_read_inode() failed to create semaphore!\n" );
		nError = psVNode->vn_hMutex;
		goto error3;
	}
	psVNode->vn_psInode->ai_psVNode = psVNode;
	*ppNode = psVNode;
	atomic_inc( &psVolume->av_nOpenFiles );
	return ( 0 );
      error3:kfree( psVNode->vn_psInode );
      error2:kfree( psVNode );
      error1:return ( nError );
}

/** Write an inode to an AFS filesystem
 * \par Description:
 * Given an Inode, write it to disk
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pNode		Inode to write
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_write_inode( void *pVolume, void *pNode )
{
	AfsVolume_s * psVolume = pVolume;
	AfsVNode_s *psVnode = pNode;

	AfsInode_s * psInode = psVnode->vn_psInode;
	int nError = 0;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error: afs_write_inode() failed to begin transaction: %d\n", nError );
		return ( nError );
	}
	if( atomic_read( &psInode->ai_nLinkCount ) == 0 )
	{
		nError = afs_delete_file_attribs( psVolume, psInode, false );
		if( nError < 0 )
		{
			printk( "Error: afs_write_inode() failed to delete attributes: %d\n", nError );
			goto error;
		}
		nError = afs_remove_from_deleted_list( psVolume, psInode );
		if( nError < 0 )
		{
			printk( "Error: afs_write_inode() failed to remove file from deleted-list: %d\n", nError );
			goto error;
		}
	}
	else
	{
		AFS_LOCK( psVnode );
		nError = afs_truncate_stream( psVolume, psInode );
		if( nError < 0 )
		{
			printk( "Error: afs_write_inode() failed to truncate stream: %d\n", nError );
			goto error;
		}
		if( psInode->ai_nFlags & INF_WAS_WRITTEN )
		{
			nError = afs_do_write_inode( psVolume, psInode );
			if( nError < 0 )
			{
				printk( "Error: afs_write_inode() failed to write inode to disk: %d\n", nError );
				goto error;
			}
		}
		AFS_UNLOCK( psVnode );
		kfree( psInode );
	}

//  afs_validate_file( psVolume, psInode );
		afs_end_transaction( psVolume, true );
	delete_semaphore( psVnode->vn_hMutex );
	atomic_dec( &psVolume->av_nOpenFiles );
	if( atomic_read( &psVolume->av_nOpenFiles ) < 0 )
	{
		printk( "PANIC : afs_write_inode() psVolume->av_nOpenFiles got a count of %d\n", atomic_read( &psVolume->av_nOpenFiles ) );
		atomic_set( &psVolume->av_nOpenFiles, 0 );
	}
	kfree( pNode );
	return ( 0 );
      error:afs_end_transaction( psVolume, false );
	afs_revert_inode( psVolume, psInode );
	return ( nError );
}

/** Create a new AFS directory
 * \par Description:
 * Create a directory with the given name in the given parent directory on
 * the given volume.
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pParent	Inode of parent directory
 * \param pzName	Name of new directory
 * \param nNameLen	Length of pzName
 * \param nMode		Type of directory to create
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_make_dir( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nMode )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	int nError;

	if( pzName[0] == '.' && ( nNameLen == 1 || ( nNameLen == 2 && pzName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	nError = afs_begin_transaction( psVolume );
	if( nError >= 0 )
	{
		AfsInode_s * psInode;
		ino_t nParent = 0;
		ino_t nInode = 0;

		AFS_LOCK( psParent->ai_psVNode );
		nError = afs_create_inode( psVolume, psParent, ( nMode & 0777 ) | S_IFDIR, INF_LOGGED, e_KeyTypeString, pzName, nNameLen, &psInode );
		if( nError >= 0 )
		{
			nParent = afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum );
			nInode = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
			kfree( psInode );
			afs_end_transaction( psVolume, true );
		}
		else
		{
			afs_end_transaction( psVolume, false );
			afs_revert_inode( psVolume, psParent );
		}
		AFS_UNLOCK( psParent->ai_psVNode );
		if( nError >= 0 )
		{
			notify_node_monitors( NWEVENT_CREATED, psVolume->av_nFsID, nParent, 0, nInode, pzName, nNameLen );
		}
	}
	else
	{
		printk( "PANIC : afs_make_dir() failed to start new transaction! Err = %d\n", nError );
	}
	return ( nError );
}

/** Lock four inodes for a file move
 * \par Description:
 * This will lock the source and destination directory, and the source
 * and destinations file inodes preperatory to moving a file.
 * \par Note:
 * \par Warning:
 * \param psOldDir	Inode of source directory
 * \param psNewDir	Inode of destination directory
 * \param psSrcInode	Inode of source file
 * \param psDstInode	Inode of destination file
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static status_t afs_lock_multiple_inodes( AfsInode_s * psOldDir, AfsInode_s * psNewDir, AfsInode_s * psSrcInode, AfsInode_s * psDstInode )
{
	sem_id anSemaphores[4];
	int nSemCount;
	int nStart = 0;

	anSemaphores[0] = psOldDir->ai_psVNode->vn_hMutex;
	anSemaphores[1] = psNewDir->ai_psVNode->vn_hMutex;
	if( psDstInode != NULL )
	{
		anSemaphores[2] = psSrcInode->ai_psVNode->vn_hMutex;
		anSemaphores[3] = psDstInode->ai_psVNode->vn_hMutex;
		nSemCount = 4;
	}
	else if( psSrcInode != NULL )
	{
		anSemaphores[2] = psSrcInode->ai_psVNode->vn_hMutex;
		nSemCount = 3;
	}
	else
	{
		nSemCount = 2;
	}
	for( ;; )

	{
		status_t nError;
		int i;

		nError = lock_semaphore( anSemaphores[nStart % nSemCount], SEM_NOSIG, INFINITE_TIMEOUT );
		for( i = 1; i < nSemCount; ++i )
		{
			nError = lock_semaphore( anSemaphores[( nStart + i ) % nSemCount], SEM_NOSIG, 0 );
			if( nError < 0 )
			{
				int j;

				for( j = 0; j < i; ++j )
				{
					unlock_semaphore( anSemaphores[( nStart + j ) % nSemCount] );
				}
				if( nError != -EWOULDBLOCK )
				{
					printk( "afs_lock_multiple_inodes() possible deadlock detected. Retry lock\n" );
					return ( nError );
				}
				else
				{
					break;
				}
			}
		}
		if( i == nSemCount )
		{
			return ( 0 );
		}
		nStart = ( nStart + 1 ) % nSemCount;
	}
}

/** Remove an AFS directory
 * \par Description:
 * Remove the given directory from the given parent directory
 * \par Note:
 * Directory must be empty
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pParent	Inode of parent directory
 * \param pzName	Name of directory to remove
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_rmdir( void *pVolume, void *pParent, const char *pzName, int nNameLen )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	AfsVNode_s *psVnode;

	AfsInode_s * psInode;
	ino_t nInodeNum;

	BIterator_s sIterator;
	bool bParentInodeTouched = false;
	bool bInodesLocked = false;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	AFS_LOCK( psParent->ai_psVNode );
	nError = afs_lookup_inode( psVolume, psParent, pzName, nNameLen, &nInodeNum );
	if( nError >= 0 )
	{
		nError = get_vnode( psVolume->av_nFsID, nInodeNum, ( void ** )&psVnode );
	}
	AFS_UNLOCK( psParent->ai_psVNode );
	if( nError < 0 )
	{
		return ( nError );
	}
	psInode = psVnode->vn_psInode;
	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		nError = -ENOTDIR;
		goto error1;
	}
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		goto error1;
	}
	nError = afs_lock_multiple_inodes( psParent, psInode, NULL, NULL );
	if( nError < 0 )
	{
		goto error2;
	}
	bInodesLocked = true;
	if( atomic_read( &psInode->ai_nLinkCount ) <= 0 )
	{
		printk( "afs_rmdir() Directory already deleted\n" );
		nError = -ENOENT;
		goto error2;
	}
	if( psInode->ai_sParent.group != psParent->ai_sInodeNum.group || psInode->ai_sParent.start != psParent->ai_sInodeNum.start )
	{
		printk( "afs_rmdir() inode changed parent when we unlocked\n" );
		nError = -ENOENT;
		goto error2;
	}
	nError = bt_find_first_key( psVolume, psInode, &sIterator );
	if( nError == -ENOENT )
	{
		atomic_dec( &psInode->ai_nLinkCount );
		nError = 0;
	}
	else if( nError >= 0 )
	{
		nError = -ENOTEMPTY;
		goto error2;
	}
	bParentInodeTouched = true;
	nError = afs_move_to_delme( psVolume, psParent, psInode, pzName, nNameLen );
	if( nError < 0 )
	{
		atomic_inc( &psInode->ai_nLinkCount );
		goto error2;
	}
	afs_end_transaction( psVolume, true );
	notify_node_monitors( NWEVENT_DELETED, psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum ), 0, nInodeNum, pzName, nNameLen );
	set_inode_deleted_flag( psVolume->av_nFsID, nInodeNum, true );
	AFS_UNLOCK( psInode->ai_psVNode );
	AFS_UNLOCK( psParent->ai_psVNode );
	put_vnode( psVolume->av_nFsID, nInodeNum );
	return ( 0 );
      error2:afs_end_transaction( psVolume, false );
	if( bInodesLocked )
	{
		if( bParentInodeTouched )
		{
			afs_revert_inode( psVolume, psParent );
		}
		AFS_UNLOCK( psInode->ai_psVNode );
		AFS_UNLOCK( psParent->ai_psVNode );
	}
      error1:put_vnode( psVolume->av_nFsID, nInodeNum );
	return ( nError );
}

/** Unlink an AFS file
 * \par Description:
 * Unlink the given file.  If this is the last link, the file is removed
 * \par Note:
 * \par Warning:
 * \param pVolume	AFS volume pointer
 * \param pParent	Inode of parent directory
 * \param pzName	Name of file to unlink
 * \param nNameLen	Length of pzName
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_unlink( void *pVolume, void *pParent, const char *pzName, int nNameLen )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psParent = ( ( AfsVNode_s * )pParent )->vn_psInode;
	AfsVNode_s *psVnode;

	AfsInode_s * psInode;
	ino_t nInodeNum;
	bool bParentTouched = false;
	bool bInodesLocked = false;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	AFS_LOCK( psParent->ai_psVNode );
	nError = afs_lookup_inode( psVolume, psParent, pzName, nNameLen, &nInodeNum );
	if( nError >= 0 )
	{
		nError = get_vnode( psVolume->av_nFsID, nInodeNum, ( void ** )&psVnode );
	}
	AFS_UNLOCK( psParent->ai_psVNode );
	if( nError < 0 )
	{
		return ( nError );
	}
	psInode = psVnode->vn_psInode;
	if( S_ISDIR( psInode->ai_nMode ) )
	{
		printk( "Error : afs_unlink() attempt to remove directory\n" );
		nError = -EISDIR;
		goto error1;
	}
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		goto error1;
	}
	nError = afs_lock_multiple_inodes( psParent, psInode, NULL, NULL );
	if( nError < 0 )
	{
		goto error2;
	}
	bInodesLocked = true;
	if( atomic_read( &psInode->ai_nLinkCount ) <= 0 )
	{
		printk( "afs_unlink() File already deleted\n" );
		nError = -ENOENT;
		goto error2;
	}
	if( psInode->ai_sParent.group != psParent->ai_sInodeNum.group || psInode->ai_sParent.start != psParent->ai_sInodeNum.start )
	{
		printk( "afs_unlink() inode changed parent when we unlocked\n" );
		nError = -ENOENT;
		goto error2;
	}
	atomic_dec( &psInode->ai_nLinkCount );
	bParentTouched = true;
	nError = afs_move_to_delme( psVolume, psParent, psInode, pzName, nNameLen );
	if( nError < 0 )
	{
		printk( "Error: afs_unlink() failed to move file into delete-me directory! (%d)\n", nError );
		atomic_inc( &psInode->ai_nLinkCount );
		goto error2;
	}
	afs_end_transaction( psVolume, true );
	notify_node_monitors( NWEVENT_DELETED, psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum ), 0, nInodeNum, pzName, nNameLen );
	set_inode_deleted_flag( psVolume->av_nFsID, nInodeNum, true );
	AFS_UNLOCK( psInode->ai_psVNode );
	AFS_UNLOCK( psParent->ai_psVNode );
	put_vnode( psVolume->av_nFsID, nInodeNum );
	return ( 0 );
      error2:afs_end_transaction( psVolume, false );
	if( bInodesLocked )
	{
		if( bParentTouched )
		{
			afs_revert_inode( psVolume, psParent );
		}
		AFS_UNLOCK( psInode->ai_psVNode );
		AFS_UNLOCK( psParent->ai_psVNode );
	}
      error1:put_vnode( psVolume->av_nFsID, nInodeNum );
	return ( nError );
}

/** Check to see if one inode is a child of the other
 * \par Description:
 * When renaming a file/directory, it must be ensured that the destination
 * is not a child of the source, as this would result in a disconnected
 * cycle.  This will make that check.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psDest	Destination Inode
 * \param psInode	Source Inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static status_t afs_rename_is_child( AfsVolume_s * psVolume, AfsInode_s * psParent, AfsInode_s * psInode )
{
	ino_t nInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
	ino_t nParentIno;

	BlockRun_s sParentIno;
	status_t nError = 0;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		return ( 0 );
	}
	if( nInodeNum == afs_run_to_num( psVolume->av_psSuperBlock, &psParent->ai_sInodeNum ) )
	{
		printk( "Error : Some dude tried to move a directory into itself.\n" );
		return ( -EINVAL );
	}
	sParentIno = psParent->ai_sParent;
	nParentIno = afs_run_to_num( psVolume->av_psSuperBlock, &sParentIno );

		// Traverse from new-dir to root, to verify that we aint about to move a dir into one of it's childs
		while( nParentIno != 0 )
	{
		AfsInode_s * psTmpInode;
		if( nParentIno == nInodeNum )
		{
			printk( "Error : Some dude tried to move a directory into one of its children\n" );
			return ( -EINVAL );
		}
		nError = afs_do_read_inode( psVolume, &sParentIno, &psTmpInode );
		if( nError < 0 )
		{
			return ( nError );
		}
		sParentIno = psTmpInode->ai_sParent;
		nParentIno = afs_run_to_num( psVolume->av_psSuperBlock, &sParentIno );
		kfree( psTmpInode );
	}
	return ( 0 );
}

/** Get the Inode assiociated with the name
 * \par Description:
 * Look up the given name in the given directory and get it's Inode
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psParent	Inode of parent directory
 * \param pzName	Name of file to lookup
 * \param nNameLen	Length of pzName
 * \param ppsRes	Return argument of found inode
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static status_t afs_rename_load_inode( AfsVolume_s * psVolume, AfsInode_s * psParent, const char *pzName, int nNameLen, AfsInode_s ** ppsRes )
{
	AfsVNode_s *psVnode;
	ino_t nInode;
	status_t nError;

	*ppsRes = NULL;
	nError = afs_lookup_inode( psVolume, psParent, pzName, nNameLen, &nInode );
	if( nError < 0 )
	{
		if( nError == -ENOENT )
		{
			return ( 0 );
		}
		else
		{
			return ( nError );
		}
	}
	nError = get_vnode( psVolume->av_nFsID, nInode, ( void ** )&psVnode );
	if( nError < 0 )
	{
		*ppsRes = NULL;
		return ( nError );
	}
	*ppsRes = psVnode->vn_psInode;
	return ( 0 );
}

/** Determine if a directory is empty
 * \par Description:
 * Check to see if the given directory is empty
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode of directory to check
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static status_t afs_rename_is_dir_empty( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	BIterator_s sIterator;
	status_t nError;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		return ( 0 );
	}
	nError = bt_find_first_key( psVolume, psInode, &sIterator );
	if( nError == -ENOENT )
	{
		return ( 0 );
	}
	else if( nError >= 0 )
	{
		return ( -ENOTEMPTY );
	}
	else
	{
		return ( nError );
	}
}

/** Rename an AFS file
 * \par Description:
 * Rename the given file to the new name in the new directory
 * \par Note:
 * \par Warning:
 * \param pVolume		AFS volume pointer
 * \param pOldDir		Inode of directory containing file to be moved
 * \param pzOldName		Name of file to be moved
 * \param nOldNameLen	Length of pzOldName
 * \param pNewDir		Inode of destination directory
 * \param pzNewName		New name of file
 * \param nNewNameLen	Length of pzNewName
 * \param bMustBeDir	If true, file to be moved must be a directory
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_rename( void *pVolume, void *pOldDir, const char *pzOldName, int nOldNameLen, void *pNewDir, const char *pzNewName, int nNewNameLen, bool bMustBeDir )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psOldDir = ( ( AfsVNode_s * )pOldDir )->vn_psInode;
	AfsInode_s * psNewDir = ( ( AfsVNode_s * )pNewDir )->vn_psInode;
	AfsInode_s * psInode = NULL;
	AfsInode_s * psDstInode = NULL;
	ino_t nInodeNum;
	bigtime_t nOldDateStamp;
	bool bOldDirTouched = false;
	bool bNewDirTouched = false;
	bool bDstInodeTouched = false;
	bool bInodesLocked = false;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return ( -EROFS );
	if( nOldNameLen <= 0 || nNewNameLen <= 0 )
	{
		return ( -EINVAL );
	}

		// FIXME: Check if names realy can be B_MAX_KEY_SIZE long (or do we need to subtract 1 like here)
		if( nOldNameLen >= B_MAX_KEY_SIZE || nNewNameLen >= B_MAX_KEY_SIZE )
	{
		return ( -ENAMETOOLONG );
	}
	if( pzOldName[0] == '.' && ( nOldNameLen == 1 || ( nOldNameLen == 2 && pzOldName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	if( pzNewName[0] == '.' && ( nNewNameLen == 1 || ( nNewNameLen == 2 && pzNewName[1] == '.' ) ) )
	{
		return ( -EEXIST );
	}
	nError = afs_begin_transaction( psVolume );
	if( nError < 0 )
	{
		printk( "Error : afs_rename() failed to start transaction: %d\n", nError );
		return ( nError );
	}

		// Lookup the file we are about to rename
		AFS_LOCK( psOldDir->ai_psVNode );
	nError = afs_rename_load_inode( psVolume, psOldDir, pzOldName, nOldNameLen, &psInode );
	AFS_UNLOCK( psOldDir->ai_psVNode );
	if( nError < 0 )
	{
		goto error;
	}
	else if( psInode == NULL )
	{
		nError = -ENOENT;
		goto error;
	}
	nInodeNum = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum );
	if( bMustBeDir && S_ISDIR( psInode->ai_nMode ) == false )
	{
		nError = -ENOTDIR;
		goto error;
	}

		// Check if the destination already exists.
		AFS_LOCK( psNewDir->ai_psVNode );
	nError = afs_rename_load_inode( psVolume, psNewDir, pzNewName, nNewNameLen, &psDstInode );
	AFS_UNLOCK( psNewDir->ai_psVNode );
	if( nError < 0 )
	{
		goto error;
	}
	if( psDstInode != NULL && S_ISDIR( psDstInode->ai_nMode ) && S_ISDIR( psInode->ai_nMode ) == false )
	{

			// we are not allowed to move a file over a directory.
			nError = -EISDIR;
		goto error;
	}
	nError = afs_lock_multiple_inodes( psOldDir, psNewDir, psInode, psDstInode );
	if( nError < 0 )
	{
		goto error;
	}
	bInodesLocked = true;

		// Check that none of the inodes in question are about to be
		// deleted by another thread.
		if( atomic_read( &psNewDir->ai_nLinkCount ) < 1 || atomic_read( &psInode->ai_nLinkCount ) < 1 || ( psDstInode != NULL && atomic_read( &psDstInode->ai_nLinkCount ) < 1 ) )
	{
		nError = -ENOENT;
		goto error;
	}

		// If we are about to overwrite a directory, make sure it is empty
		if( psDstInode != NULL )
	{
		nError = afs_rename_is_dir_empty( psVolume, psDstInode );
		if( nError < 0 )
		{
			goto error;
		}
	}

		// If we are about to move a directory, we must verify that
		// we aren't moving it into itself or one of its children.
		nError = afs_rename_is_child( psVolume, psNewDir, psInode );
	if( nError < 0 )
	{
		goto error;
	}
	if( psDstInode != NULL )
	{
		if( atomic_read( &psDstInode->ai_nLinkCount ) != 1 )
		{
			panic( "afs_rename_is_child() about to overwrite an inode with link-count = %d\n", atomic_read( &psDstInode->ai_nLinkCount ) );
			goto error;
		}
		bNewDirTouched = true;
		atomic_dec( &psDstInode->ai_nLinkCount );
		nError = afs_move_to_delme( psVolume, psNewDir, psDstInode, pzNewName, nNewNameLen );
		if( nError < 0 )
		{
			atomic_inc( &psDstInode->ai_nLinkCount );
			goto error;
		}
		bDstInodeTouched = true;
	}
	bNewDirTouched = true;
	nError = bt_insert_key( psVolume, psNewDir, pzNewName, nNewNameLen, nInodeNum );
	if( nError < 0 )
	{
		goto error;
	}
	bOldDirTouched = true;
	nError = bt_delete_key( psVolume, psOldDir, pzOldName, nOldNameLen );
	if( nError < 0 )
	{
		goto error;
	}
	nOldDateStamp = psInode->ai_nModifiedTime;
	psInode->ai_nModifiedTime = get_real_time();
	psInode->ai_sParent = psNewDir->ai_sInodeNum;
	nError = afs_do_write_inode( psVolume, psInode );
	if( nError < 0 )
	{
		psInode->ai_nModifiedTime = nOldDateStamp;
		psInode->ai_sParent = psOldDir->ai_sInodeNum;
		goto error;
	}
	nError = afs_end_transaction( psVolume, true );
	AFS_UNLOCK( psOldDir->ai_psVNode );
	AFS_UNLOCK( psNewDir->ai_psVNode );
	AFS_UNLOCK( psInode->ai_psVNode );
	put_vnode( psVolume->av_nFsID, nInodeNum );
	if( psDstInode != NULL )
	{
		AFS_UNLOCK( psDstInode->ai_psVNode );
		put_vnode( psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psDstInode->ai_sInodeNum ) );
	}
	notify_node_monitors( NWEVENT_MOVED, psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psOldDir->ai_sInodeNum ), afs_run_to_num( psVolume->av_psSuperBlock, &psNewDir->ai_sInodeNum ), nInodeNum, pzNewName, nNewNameLen );
	return ( 0 );
      error:afs_end_transaction( psVolume, false );
	if( bOldDirTouched )
	{
		afs_revert_inode( psVolume, psOldDir );
	}
	if( bNewDirTouched )
	{
		afs_revert_inode( psVolume, psNewDir );
	}
	if( psDstInode != NULL && bDstInodeTouched )
	{
		afs_revert_inode( psVolume, psDstInode );
	}
	if( bInodesLocked )
	{
		AFS_UNLOCK( psOldDir->ai_psVNode );
		AFS_UNLOCK( psNewDir->ai_psVNode );
		AFS_UNLOCK( psInode->ai_psVNode );
		if( psDstInode != NULL )
		{
			AFS_UNLOCK( psDstInode->ai_psVNode );
		}
	}
	if( psInode != NULL )
	{
		put_vnode( psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ) );
	}
	if( psDstInode != NULL )
	{
		put_vnode( psVolume->av_nFsID, afs_run_to_num( psVolume->av_psSuperBlock, &psDstInode->ai_sInodeNum ) );
	}
	return ( nError );
}

/** Get the number of contiguous blocks from an AFS file
 * \par Description:
 * For the given file, starting at the given file position, return the
 * number of contiguous blocks (a run), and the actual disk offset of the set of
 * contiguous blocks.
 * \par Note:
 * \par Warning:
 * \param pVolume		AFS filesystem pointer
 * \param pNode			Inode of file to search
 * \param nPos			File position to start looking for run
 * \param nBlockCount	Maximum number of blocks to find
 * \param pnStart		Return parameter for disk offset of start of run
 * \param pnActualCount	Actual number of blocks in the run
 * \return device number on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afsi_get_stream_blocks( void *pVolume, void *pNode, off_t nPos, int nBlockCount, off_t *pnStart, int *pnActualCount )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError;

	AFS_LOCK( psInode->ai_psVNode );
	nError = afs_get_stream_blocks( psVolume, &psInode->ai_sData, nPos, nBlockCount, pnStart, pnActualCount );
	if( nError < 0 )
	{
		printk( "Error: afsi_get_stream_blocks() afs_get_stream_blocks() failed with code %d\n", nError );
	}
	AFS_UNLOCK( psInode->ai_psVNode );
	if( nError >= 0 )
	{
		nError = psVolume->av_nDevice;
	}
	return ( nError );
}

/** Truncate a file to a specific length
 * \par Description:
 * Truncate the given file on the give volume to the given length.  If nLen is shorter
 * than the current length, the file is shortened.  Otherwise, it is lengthened and zero
 * filled.
 * \par Note:
 * This could be improved by making the file sparse.  I suspect that needs VFS support
 * \par Warning:
 * \param pVolume	AFS filesystem pointer
 * \param pNode		Inode of file to truncate
 * \param nLen		Length to truncate to
 * \return 0 on success negative error code on failure
 * \sa
 ****************************************************************************/
int afs_truncate( void *pVolume, void *pNode, off_t nLen )
{
	AfsVolume_s * psVolume = pVolume;
	AfsInode_s * psInode = ( ( AfsVNode_s * )pNode )->vn_psInode;
	int nError;
	off_t nOldSize, nPos, nBufSize;
	void * pBuffer = NULL;

	if( S_ISDIR( psInode->ai_nMode ) )
	{
		return ( -EISDIR );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
	{
		return ( -EROFS );
	}
	if( nLen < 0 || nLen > SIZE_MAX )
	{
		return ( -EINVAL );
	}
	nOldSize = psInode->ai_sData.ds_nSize;
	if (nLen == nOldSize)
		return ( 0 );

	if (nLen < nOldSize)
	{
		// Shrink
		nError = afs_begin_transaction( psVolume );
		if( nError < 0 )
		{
			printk( "Error : %s failed to start transaction\n", __FUNCTION__ );
			goto error;
		}
		AFS_LOCK( psInode->ai_psVNode );
		psInode->ai_sData.ds_nSize = nLen;
		nError = afs_truncate_stream( psVolume, psInode );
		if( nError < 0 )
		{
			printk( "Error : %s truncate stream failed; reverting\n", __FUNCTION__ );
			psInode->ai_sData.ds_nSize = nOldSize;
			afs_end_transaction( psVolume, false );
			afs_revert_inode( psVolume, psInode );
		}
		else
		{
			afs_end_transaction( psVolume, true );
		}
		AFS_UNLOCK( psInode->ai_psVNode );
	}
	else
	{
		// Expand
		// Will need a buffer to write zeros.  Get it now so that we can handle failure
		// gracefully
		pBuffer = afs_alloc_block_buffer( psVolume );
		if ( !pBuffer )
		{
			printk( "PANIC : %s failed to truncate file! No memory!\n", __FUNCTION__ );
			nError = -ENOMEM;
			goto error;
		}
		AFS_LOCK( psInode->ai_psVNode );
		// Set the position to the end of the file if opened in append mode
		nPos = psInode->ai_sData.ds_nSize;
		// Expand file
		nError = afs_expand_file( psVolume, psInode, NULL, &nPos, &nLen );
		if( nLen > 0 )
		{
			// Need to write zeros into expanded space.
			nBufSize = psVolume->av_psSuperBlock->as_nBlockSize;
			memset(pBuffer, 0, nBufSize);
			while (nLen > 0)
			{
				if (nLen >= nBufSize)
				{
					nError = afs_do_write( pVolume, psInode, pBuffer, nPos, nBufSize );
					nPos += nBufSize;
					nLen -= nBufSize;
				}
				else
				{
					nError = afs_do_write( pVolume, psInode, pBuffer, nPos, nLen );
					nPos += nLen;
					nLen -= nLen;
				}
				if (nError < 0)
				{
					printk( "%s Uh oh.  nError < 0 \n", __FUNCTION__ );
					break;
				}
			}
		}
		afs_free_block_buffer( psVolume, pBuffer );
		if( nError < 0 )
		{
			printk( "%s Something failed... %d \n", __FUNCTION__, nError );
			psInode->ai_sData.ds_nSize = nOldSize;
			afs_revert_inode( psVolume, psInode );
		}
		AFS_UNLOCK( psInode->ai_psVNode );
	}
	if( nError < 0 )
	{
		printk( "PANIC : %s failed to truncate file!\n", __FUNCTION__ );
	}
	else
	{
		notify_node_monitors( NWEVENT_STAT_CHANGED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), NULL, 0 );
		psInode->ai_nFlags &= ~INF_STAT_CHANGED;
	}
error:
	return ( nError );
}

/** AFS instance of FSOperations_s
 * \par Description:
 * The instance of FSOperations_s allows the VFS to call into the
 * filesystem for filesystem specific functions.
 * \par Note:
 * \par Warning:
 * \sa FSOperations_s
 *****************************************************************************/
static FSOperations_s g_sOperations =
{
	afs_probe,		// op_probe
	afs_mount,		// op_mount
	afs_unmount,		// op_unmount
	afs_read_inode,		// op_read_inode
	afs_write_inode,	// op_write_inode
	afs_lookup,		// op_locate_inode
	NULL,			// op_access
	afs_create,		// op_create
	afs_make_dir,		// op_mkdir
	afs_mknod,		// op_mknod
	afs_symlink,		// op_symlink
	NULL,			// op_link
	afs_rename,		// op_rename
	afs_unlink,		// op_unlink
	afs_rmdir,		// op_rmdir
	afs_read_link,		// op_readlink
	NULL,			// op_opendir
	NULL,			// op_closedir
	afs_rewinddir,		// op_rewinddir
	afs_read_dir,		// op_readdir
	afs_open,		// op_open
	afs_close,		// op_close
	NULL,			// op_free_cookie
	afs_read,		// op_read
	afs_write,		// op_write
	afs_readv,		// op_readv
	afs_writev,		// op_writev
	NULL,			// op_ioctl
	NULL,			// op_setflags
	afs_rstat,		// op_rstat
	afs_wstat,		// op_wstat
	NULL,			// op_fsync
	afs_initialize,		// op_initialize
	afs_sync,		// op_sync
	afs_rfsstat,		// op_rfsstat
	NULL,			// op_wfsstat
	NULL,			// op_isatty
	NULL,			// op_add_select_req
	NULL,			// op_rem_select_req
	afs_open_attrdir,	// op_open_attrdir
	afs_close_attrdir,	// op_close_attrdir
	afs_rewind_attrdir,	// op_rewind_attrdir
	afs_read_attrdir,	// op_read_attrdir
	afs_remove_attr,	// op_remove_attr
	NULL,			// op_rename_attr
	afs_stat_attr,		// op_stat_attr
	afs_write_attr,		// op_write_attr
	afs_read_attr,		// op_read_attr
	afs_open_indexdir,	// op_open_indexdir
	afs_close_indexdir,	// op_close_indexdir
	afs_rewind_indexdir,	// op_rewind_indexdir
	afs_read_indexdir,	// op_read_indexdir
	afs_create_index,	// op_create_index
	afs_remove_index,	// op_remove_index
	NULL,			// op_rename_index
	afs_stat_index,		// op_stat_index
	afsi_get_stream_blocks,	// op_get_file_blocks
	afs_truncate		// op_truncate
};

/** Initialize the AFS filesystem driver
 * \par Description:
 * Tell the VFS about our FSOperations_s structure so the VFS can
 * communicate with AFS.
 * \par Note:
 * \par Warning:
 * \param	pzName	The name of the filesystem
 * \param	ppsOps	The FSOperations_s pointer to fill
 * \return	FSDRIVER_API_VERSION on success, -1 on failure
 * \sa
 *****************************************************************************/
int fs_init( const char *pzName, FSOperations_s ** ppsOps )
{
	printk( "initialize_fs called in afs\n" );
	*ppsOps = &g_sOperations;
	return ( FSDRIVER_API_VERSION );
}
