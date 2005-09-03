
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

#include <posix/errno.h>
#include <atheos/string.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <macros.h>

#include "afs.h"

#if 0
int afs_validate_stream( AfsVolume_s * psVolume, AfsInode_s * psInode, uint32 *pBitMap )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	DataStream_s *psStream = &psInode->ai_sData;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBlocksPerGrp = psSuperBlock->as_nBlockPerGroup;
	const int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );
	int i;

	for( i = 0; i < DIRECT_BLOCK_COUNT; ++i )
	{
		int nGroup = psStream->ds_asDirect[i].group;
		int nStart = psStream->ds_asDirect[i].start;
		int j;

		for( j = 0; j < psStream->ds_asDirect[i].len; ++j )
		{
			off_t nBlock = nGroup * nBlocksPerGrp + nStart + j;

			if( afs_is_block_free( psVolume, nBlock ) )
			{
				printk( "PANIC : Block %d in direct run %d is free!(%d)\n", j, i, psStream->ds_asDirect[i].len );
				break;
			}
			if( NULL != pBitMap )
			{
				if( GETBIT( pBitMap, nBlock ) )
				{
					printk( "PANIC: Stream direct data block %d is double bucked!\n",( int )nBlock );
				}
				SETBIT( pBitMap, nBlock, true );
			}
		}
	}

	if( 0 != psStream->ds_sIndirect.len )
	{
		off_t nIndirectBlock = afs_run_to_num( psSuperBlock, &psStream->ds_sIndirect );

		for( i = 0; i < psStream->ds_sIndirect.len; ++i )
		{
			BlockRun_s *pasBlock = get_cache_block( psVolume->av_nDevice, nIndirectBlock + i, nBlockSize );
			int j;

			if( pBitMap != NULL )
			{
				if( GETBIT( pBitMap, nIndirectBlock + i ) )
				{
					printk( "PANIC: Indirect pointer block %d(%d) is double bucked!", i, ( int )nIndirectBlock + i );
				}
				SETBIT( pBitMap, nIndirectBlock + i, true );
			}

			kassertw( false == afs_is_block_free( psVolume, nIndirectBlock + i ) );
			kassertw( NULL != pasBlock );

			for( j = 0; j < nPtrsPerBlk; ++j )
			{
				int nGroup = pasBlock[j].group;
				int nStart = pasBlock[j].start;
				int k;

				for( k = 0; k < pasBlock[j].len; ++k )
				{
					off_t nBlock = nGroup * nBlocksPerGrp + nStart + k;

					if( pBitMap != NULL )
					{
						if( GETBIT( pBitMap, nBlock ) )
						{
							printk( "PANIC: Indirect data block %d is double bucked!",( int )nBlock );
						}
						SETBIT( pBitMap, nBlock, true );
					}
					if( afs_is_block_free( psVolume, nBlock ) )
					{
						printk( "PANIC : Block %d in indirect run %d is free!\n", k, i * nBlocksPerGrp + j );
						break;
					}
				}
			}
			release_cache_block( psVolume->av_nDevice, nIndirectBlock + i );
		}
	}

	if( 0 != psStream->ds_sDoubleIndirect.len )
	{
		off_t nIndirectBlock = afs_run_to_num( psSuperBlock, &psStream->ds_sDoubleIndirect );

		for( i = 0; i < psStream->ds_sDoubleIndirect.len; ++i )
		{
			BlockRun_s *pasIndirectBlock = get_cache_block( psVolume->av_nDevice, nIndirectBlock + i, nBlockSize );
			int j;

			if( pBitMap != NULL )
			{
				if( GETBIT( pBitMap, nIndirectBlock + i ) )
				{
					printk( "PANIC: Double indirect pointer block %d(%d) is double bucked!", i, ( int )nIndirectBlock + i );
				}
				SETBIT( pBitMap, nIndirectBlock + i, true );
			}

			kassertw( false == afs_is_block_free( psVolume, nIndirectBlock + i ) );
			kassertw( NULL != pasIndirectBlock );

			for( j = 0; j < nPtrsPerBlk; ++j )
			{
				off_t nDirectBlock = afs_run_to_num( psSuperBlock, &pasIndirectBlock[j] );
				int k;

				for( k = 0; k < pasIndirectBlock[j].len; ++k )
				{
					BlockRun_s *pasDirectBlock = get_cache_block( psVolume->av_nDevice, nDirectBlock + k, nBlockSize );
					int l;

					if( pBitMap != NULL )
					{
						if( GETBIT( pBitMap, nDirectBlock + k ) )
						{
							printk( "PANIC: Direct pointer block in double-indirect area %d(%d) is double bucked!", k, ( int )nDirectBlock + k );
						}
						SETBIT( pBitMap, nDirectBlock + k, true );
					}

					kassertw( false == afs_is_block_free( psVolume, nDirectBlock + k ) );
					kassertw( NULL != pasDirectBlock );


					for( l = 0; l < nPtrsPerBlk; ++l )
					{
						int nGroup = pasDirectBlock[l].group;
						int nStart = pasDirectBlock[l].start;
						int m;

						for( m = 0; m < pasDirectBlock[l].len; ++m )
						{
							off_t nBlock = nGroup * nBlocksPerGrp + nStart + m;

							if( pBitMap != NULL )
							{
								if( GETBIT( pBitMap, nBlock ) )
								{
									printk( "PANIC: Double indirect data block %d is double bucked!",( int )nBlock );
								}
								SETBIT( pBitMap, nBlock, true );
							}
							if( afs_is_block_free( psVolume, nBlock ) )
							{
								printk( "PANIC : Block %d in double indirect run %d is free!\n", m, k * nBlocksPerGrp + l );
							}
						}
					}
					release_cache_block( psVolume->av_nDevice, nDirectBlock + k );
				}
			}
			release_cache_block( psVolume->av_nDevice, nIndirectBlock + i );
		}
	}
	return( 0 );
}
#endif

/** Allocate metadata disk blocks
 * \par Description:
 * Allocate and zero out the disk blocks into the given block run.  First, get a temporary
 * block-sized buffer, and zero it.  Next, allocate the disk blocks for the given run.
 * Loop through the disk blocks, and write the zeroed temporary buffer to each block in
 * the run.  Free the temporary buffer.  If an error occures, free the disk blocks for
 * the run, and set it's length to zero.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psRun		Block run to allocate and zero
 * \param psPrevRun	Block run after which to allocate the new one
 * \param nGroup	Group in which to allocate the block run
 * \param nSize		Size of block run to allocate
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int afs_alloc_meta_blocks( AfsVolume_s * psVolume, BlockRun_s * psRun, BlockRun_s * psPrevRun, int nGroup, int nSize )
{
	off_t nBlkNum;
	int nError;
	int i;
	void *pBlock;

	pBlock = afs_alloc_block_buffer( psVolume );

	if( pBlock == NULL )
	{
		printk( "Error: afs_alloc_meta_blocks() no memory for temporary buffer\n" );
		return( -ENOMEM );
	}
	memset( pBlock, 0, psVolume->av_psSuperBlock->as_nBlockSize );

	nError = afs_alloc_blocks( psVolume, psRun, psPrevRun, nGroup, nSize, BLOCKS_PER_DI_RUN );

	if( nError < 0 )
	{
		printk( "Error: afs_alloc_meta_blocks() failed to alloc any blocks\n" );
		goto error1;
	}
	if( psRun->len != nSize )
	{
		printk( "Error: afs_alloc_meta_blocks() failed to alloc %d contigious blocks\n", nSize );
		nError = -ENOSPC;
		goto error2;
	}

	nBlkNum = afs_run_to_num( psVolume->av_psSuperBlock, psRun );

	for( i = 0; i < nSize; ++i )
	{
		nError = afs_logged_write( psVolume, pBlock, nBlkNum + i );

		if( nError < 0 )
		{
			printk( "Panic: afs_alloc_meta_blocks() failed to write zeros into the newly allocated block\n" );
			goto error2;
		}
	}
	afs_free_block_buffer( psVolume, pBlock );
	return( 0 );
      error2:
	afs_free_blocks( psVolume, psRun );
      error1:
	afs_free_block_buffer( psVolume, pBlock );
	psRun->len = 0;
	return( nError );
}

/** Allocate data blocks
 * \par Description:
 * Allocate a run of disk blocks for data for the given inode.  Allocate a run of blocks,
 * storing it in the given run.  Then, if the inode points to a directory, wait for each
 * block in the run to clear the log.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode owning data space
 * \param psRun		Block run to allocate and zero
 * \param psPrevRun	Block run after which to allocate the new one
 * \param nGroup	Group in which to allocate the block run
 * \param nSize		Size of block run to allocate
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static status_t afs_alloc_data_blocks( AfsVolume_s * psVolume, AfsInode_s * psInode, BlockRun_s * psNewRun, BlockRun_s * psPrevRun, int nDataGroup, int nSize )
{
	status_t nError;

	nError = afs_alloc_blocks( psVolume, psNewRun, psPrevRun, nDataGroup, nSize, 1 );
	if( nError < 0 )
	{
//      printk( "afs_expand_indirect_range() failed to alloc datablocks for indirect range %d\n", nError );
//      if( bSomeBlocksAllocated ) {
//          if( afs_logged_write( psVolume, psBlock, nBlkNum + i ) < 0 ) {
//              panic( "afs_expand_indirect_range:1() failed to write back indirect block %d\n", j);
//          }
//      }
		return( nError );
	}
	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		off_t nRunStart = afs_run_to_num( psVolume->av_psSuperBlock, psNewRun );
		int k;

		for( k = 0; k < psNewRun->len; ++k )
		{
			afs_wait_for_block_to_leave_log( psVolume, nRunStart + k );
		}
	}
	return( 0 );
}

/** Expand an inode's indirect range
 * \par Description:
 * Expand the indirect range of the given inode by the given amount.  The indirect range of an inode
 * is a set of blocks containing pointers to data blockruns.  If the given inode does not already
 * have an indirect range, create a new (empty) one.  Then, call afs_expand_indirect_blockrun() to
 * expand it.
 * \par Note:
 * \par Warning:
 * nDeltaSize, although signed, cannot be negative (unhandled)
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode owning indirect range
 * \param nDeltaSize	Amount by which to increase indirect range
 * \param pbDiskFull	(out) set to true if disk becomes full
 * \return number of blocks added on success, negative error code on failure
 * \sa
 ****************************************************************************/
static off_t afs_expand_indirect_range( AfsVolume_s * psVolume, AfsInode_s * psInode, int nDeltaSize, bool *pbDiskFull )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	DataStream_s *psStream = &psInode->ai_sData;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );
	const int nMetaGroup = psInode->ai_sInodeNum.group;
	int nDataGroup = nMetaGroup;
	BlockRun_s *psBlock;
	BlockRun_s sNewRun;
	BlockRun_s sPrevRun;
	off_t nBlkNum;
	off_t nOldSize;
	off_t nBlocksAdded = 0;
	int nSize = nDeltaSize;
	int nError = 0;
	int i;

	sNewRun.len = 0;
	sPrevRun.len = 0;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		nDataGroup++;
	}

	kassertw( 0 == psStream->ds_nMaxDoubleIndirectRange );

	psBlock = afs_alloc_block_buffer( psVolume );

	if( psBlock == NULL )
	{
		printk( "Error : afs_expand_indirect_range() no memory for pointer block\n" );
		return( -ENOMEM );
	}

	if( psStream->ds_sIndirect.len == 0 )
	{
		nError = afs_alloc_meta_blocks( psVolume, &psStream->ds_sIndirect, NULL, nMetaGroup, BLOCKS_PER_DI_RUN );
		if( nError < 0 )
		{
			printk( "Error : afs_expand_indirect_range() failed to alloc indirect pointer block\n" );
			goto error;
		}
	}
	if( psStream->ds_nMaxIndirectRange == 0 )
	{
		psStream->ds_nMaxIndirectRange = psStream->ds_nMaxDirectRange;
	}
	nOldSize = psStream->ds_nMaxIndirectRange - psStream->ds_nMaxDirectRange;
	nBlkNum = afs_run_to_num( psSuperBlock, &psStream->ds_sIndirect );

	for( i = 0; i < psStream->ds_sIndirect.len && nSize > 0; ++i )
	{
		bool bSomeBlocksAllocated = true;
		int j;

		nError = afs_logged_read( psVolume, psBlock, nBlkNum + i );

		if( nError < 0 )
		{
			printk( "Error : afs_expand_indirect_range() Failed to load indirect pointer block\n" );
			if( sNewRun.len > 0 )
			{
				afs_free_blocks( psVolume, &sNewRun );
			}
			goto error;
		}
		for( j = 0; j < nPtrsPerBlk && nSize > 0; ++j )
		{
			if( nOldSize > 0 )
			{
				nOldSize -= psBlock[j].len;
				if( nOldSize > 0 )
				{
					if( psBlock[j].len == 0 )
					{
						panic( "afs_expand_indirect_range:1() inconsistency between ds_nMaxIndirectRange and block runs\n" );
					}
					sPrevRun = psBlock[j];
					continue;
				}
				if( nOldSize < 0 )
				{
					panic( "afs_expand_indirect_range:2() inconsistency between ds_nMaxIndirectRange and block runs.\n" );
				}
			}
			if( sNewRun.len == 0 )
			{
				BlockRun_s *psPrevRun = NULL;

				if( psBlock[j].len > 0 )
				{
					psPrevRun = &psBlock[j];
				}
				else if( sPrevRun.len > 0 )
				{
					psPrevRun = &sPrevRun;
				}
				nError = afs_alloc_data_blocks( psVolume, psInode, &sNewRun, psPrevRun, nDataGroup, nSize );
				if( nError < 0 )
				{
					printk( "Error: afs_expand_indirect_range() failed to alloc datablocks for indirect range %d(%Ld)\n", nError, nBlocksAdded );
					nSize = 0;
					if( nError == -ENOSPC && nBlocksAdded > 0 )
					{
						*pbDiskFull = true;
						nError = 0;
						break;
					}
					goto error;
				}
			}
			if( psBlock[j].len > 0 )
			{
				if( psBlock[j].group == sNewRun.group && ( psBlock[j].start + psBlock[j].len ) == sNewRun.start )
				{
					uint nUsed;

					if( psBlock[j].start + psBlock[j].len + sNewRun.len > psSuperBlock->as_nBlockPerGroup )
					{
						nUsed = psSuperBlock->as_nBlockPerGroup -( psBlock[j].start + psBlock[j].len );
						if( nUsed > AFS_MAX_RUN_LENGTH )
							nUsed = AFS_MAX_RUN_LENGTH;
					}
					else
					{
						nUsed = sNewRun.len;
					}
					if( nUsed > sNewRun.len )
					{
						panic( "afs_expand_indirect_range() to many blocks taken from run(%d of %d)\n", nUsed, sNewRun.len );
					}
					if( nUsed > 0 )
					{
						psBlock[j].len += nUsed;
						sNewRun.start += nUsed;
						sNewRun.len -= nUsed;
						nSize -= nUsed;
						psStream->ds_nMaxIndirectRange += nUsed;
						nBlocksAdded += nUsed;
						bSomeBlocksAllocated = true;
					}
				}
			}
			else
			{
				psBlock[j] = sNewRun;
				nSize -= sNewRun.len;
				psStream->ds_nMaxIndirectRange += sNewRun.len;
				nBlocksAdded += sNewRun.len;
				sNewRun.len = 0;
				bSomeBlocksAllocated = true;
			}
			sPrevRun = psBlock[j];
		}
		if( nError >= 0 && bSomeBlocksAllocated )
		{
			nError = afs_logged_write( psVolume, psBlock, nBlkNum + i );
			if( nError < 0 )
			{
				printk( "Error: afs_expand_indirect_range:2() failed to write back indirect block %d\n", j );
				goto error;
			}
		}
	}
	kassertw( nError <= 0 );

	if( sNewRun.len > 0 )
	{
		afs_free_blocks( psVolume, &sNewRun );
	}

	afs_free_block_buffer( psVolume, psBlock );
	return( nBlocksAdded );
      error:
	afs_free_block_buffer( psVolume, psBlock );
	return( nError );
}

/** Expand an inode's double-indirect range
 * \par Description:
 * Expand the double-indirect range of the given inode by the given amount.  If the double-indirect
 * blockrun doesn't exist, create it.  Loop through it.  For each pointer, if it is empty, create a
 * new indirect blockrun for it.  Regardless, call afs_expand_indirect_blockrun() to expand it.
 * Continue until the expansion is complete, the filesystem is full, or the double-indirect range is
 * full.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode owning double-indirect range
 * \param nDeltaSize	Amount by which to increase double-indirect range
 * \param pbDiskFull	(out) set to true if disk becomes full
 * \return number of blocks added on success, negative error code on failure
 * \sa
 ****************************************************************************/
static off_t afs_expand_doubleindirect_range( AfsVolume_s * psVolume, AfsInode_s * psInode, off_t nDeltaSize )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	DataStream_s *psStream = &psInode->ai_sData;
	const int nBlockSize = psSuperBlock->as_nBlockSize;

	const int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );
	int nDataGroup = psInode->ai_sInodeNum.group;

	// ([nIDBlk][nIDPtr])([nDBlk][nDPtr])[nBlk]
	//    0-3     0-127     0-3    0-127   0-3

	const off_t nCurPos =( psStream->ds_nMaxDoubleIndirectRange > 0 ) ? psStream->ds_nMaxDoubleIndirectRange - psStream->ds_nMaxIndirectRange : 0;

	const int nDPtrSize = BLOCKS_PER_DI_RUN;
	const int nDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk;
	const int nIDPtrSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN;
	const int nIDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN * nPtrsPerBlk;

	int nDPtr =( nCurPos / nDPtrSize ) % nPtrsPerBlk;
	int nDBlk =( nCurPos / nDBlkSize ) % BLOCKS_PER_DI_RUN;
	int nIDPtr =( nCurPos / nIDPtrSize ) % nPtrsPerBlk;
	int nIDBlk =( nCurPos / nIDBlkSize );
	int nIndirectBlk;
	BlockRun_s sDataRun;
	BlockRun_s *psIndirectBlock;
	BlockRun_s *psDirectBlock;
	off_t nSize = nDeltaSize;
	off_t nBlocksAdded = 0;
	int nError = 0;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		nDataGroup++;
	}

	psDirectBlock = kmalloc( nBlockSize * 2, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psDirectBlock == NULL )
	{
		printk( "Error: afs_expand_doubleindirect_range() no memory for pointer buffers\n" );
		return( -ENOMEM );
	}
	psIndirectBlock =( BlockRun_s * ) ( ( ( uint8 * )psDirectBlock ) + nBlockSize );

	sDataRun.len = 0;

	if( psStream->ds_nMaxDoubleIndirectRange == 0 )
	{
		psStream->ds_nMaxDoubleIndirectRange = psStream->ds_nMaxIndirectRange;
	}

	if( psStream->ds_sDoubleIndirect.len == 0 )
	{
		nError = afs_alloc_meta_blocks( psVolume, &psStream->ds_sDoubleIndirect, NULL, psInode->ai_sInodeNum.group, BLOCKS_PER_DI_RUN );

		if( nError < 0 )
		{
			psStream->ds_nMaxDoubleIndirectRange = 0;
		}
//    printk( "Alloc double indirect block %d\n", afs_run_to_num( psSuperBlock, &psStream->ds_sDoubleIndirect ) );
	}

	nIndirectBlk = afs_run_to_num( psSuperBlock, &psStream->ds_sDoubleIndirect );

	// Iterate through all indirect blocks
	for( ; nIDBlk < BLOCKS_PER_DI_RUN && nSize > 0 && nError >= 0; ++nIDBlk )
	{
		bool bIndirectBlockDirty = false;

		nError = afs_logged_read( psVolume, psIndirectBlock, nIndirectBlk + nIDBlk );
		if( nError < 0 )
		{
			printk( "PANIC : afs_expand_doubleindirect_range() failed to load indirect block\n" );
			break;
		}
		// Iterate through all direct blocks, alloc new blocks as needed.
		for( ; nIDPtr < nPtrsPerBlk && nSize > 0 && nError >= 0; ++nIDPtr )
		{
			off_t nDirectBlkNum;

			// Alloc new direct block if needed
			if( psIndirectBlock[nIDPtr].len == 0 )
			{
				nError = afs_alloc_meta_blocks( psVolume, &psIndirectBlock[nIDPtr],( ( nIDPtr > 0 ) ? &psIndirectBlock[nIDPtr - 1] : NULL ), psInode->ai_sInodeNum.group, BLOCKS_PER_DI_RUN );
				if( nError < 0 )
				{
					break;
				}
				bIndirectBlockDirty = true;
//      printk( "Alloc direct block at pos %d\n",(int)nIDPtr );
			}

			nDirectBlkNum = afs_run_to_num( psSuperBlock, &psIndirectBlock[nIDPtr] );

			// Iterate through all fs-blocks building the direct block
			for( ; nDBlk < BLOCKS_PER_DI_RUN && nSize > 0 && nError >= 0; ++nDBlk )
			{
				status_t nTmpErr;

				nError = afs_logged_read( psVolume, psDirectBlock, nDirectBlkNum + nDBlk );
				if( nError < 0 )
				{
					printk( "Error: afs_expand_doubleindirect_range() failed to load direct block from disk\n" );
					break;
				}
				// Allocate data blocks and insert them into the direct block.
				for( ; nDPtr < nPtrsPerBlk && nSize > 0 && nError >= 0; ++nDPtr )
				{
					kassertw( psDirectBlock[nDPtr].len == 0 );

					if( sDataRun.len < BLOCKS_PER_DI_RUN )
					{
						BlockRun_s *psPrevRun;
						int i;

						if( sDataRun.len > 0 )
						{
							nError = afs_free_blocks( psVolume, &sDataRun );
							sDataRun.len = 0;
							if( nError < 0 )
							{
								printk( "Error: afs_expand_doubleindirect_range() failed to free unused data blocks: %d\n", nError );
								break;
							}
						}
						psPrevRun =( nDPtr > 0 ) ? &psDirectBlock[nDPtr - 1] : NULL;
						nError = afs_alloc_blocks( psVolume, &sDataRun, psPrevRun, nDataGroup, nSize, BLOCKS_PER_DI_RUN );

						if( nError < 0 )
						{
							if( nError == -ENOSPC && nBlocksAdded > 0 )
							{
								nError = 0;
								nSize = 0;
							}
							printk( "Error : afs_expand_doubleindirect_range() failed to alloc any data blocks\n" );
							break;
						}
						if( sDataRun.len < BLOCKS_PER_DI_RUN )
						{
							printk( "Error : afs_expand_doubleindirect_range() Largest span = %d\n", sDataRun.len );
							if( nBlocksAdded > 0 )
							{
								nError = 0;
								nSize = 0;
							}
							else
							{
								nError = -ENOSPC;
							}
							break;
						}
						if( S_ISDIR( psInode->ai_nMode ) == false )
						{
							off_t nRunStart = afs_run_to_num( psSuperBlock, &sDataRun );

							for( i = 0; i < sDataRun.len; ++i )
							{
								afs_wait_for_block_to_leave_log( psVolume, nRunStart + i );
							}
						}
					}

					psDirectBlock[nDPtr].group = sDataRun.group;
					psDirectBlock[nDPtr].start = sDataRun.start;
					psDirectBlock[nDPtr].len = BLOCKS_PER_DI_RUN;

					sDataRun.start += BLOCKS_PER_DI_RUN;
					sDataRun.len -= BLOCKS_PER_DI_RUN;

					nSize -= BLOCKS_PER_DI_RUN;
					nBlocksAdded += BLOCKS_PER_DI_RUN;
					psStream->ds_nMaxDoubleIndirectRange += BLOCKS_PER_DI_RUN;
				}
				nDPtr = 0;
				nTmpErr = afs_logged_write( psVolume, psDirectBlock, nDirectBlkNum + nDBlk );
				if( nTmpErr < 0 )
				{
					nError = nTmpErr;
					printk( "Error: afs_expand_doubleindirect_range() failed to write direct block: %d\n", nError );
					break;
				}
			}
			nDBlk = 0;
		}
		nIDPtr = 0;
		if( bIndirectBlockDirty )
		{
			status_t nTmpErr = afs_logged_write( psVolume, psIndirectBlock, nIndirectBlk + nIDBlk );

			if( nTmpErr < 0 )
			{
				printk( "Panic: afs_expand_doubleindirect_range() failed to write indirect block\n" );
				nError = nTmpErr;
				break;
			}
		}
	}
	// Release unused blocks.
	if( nError >= 0 && sDataRun.len > 0 )
	{
		afs_free_blocks( psVolume, &sDataRun );
	}
	kfree( psDirectBlock );

	kassertw( nError <= 0 );
	if( nError >= 0 )
	{
		if( nSize == nDeltaSize )
		{
			printk( "afs_expand_doubleindirect_range() max file size reached missed %d blocks\n",( int )nSize );
		}
//    printk( "Alloc %d blocks in double indirect range\n",(int)(nDeltaSize - nSize) );
		return( nBlocksAdded );
	}
	else
	{
		return( nError );
	}
}

/** Expand the data stream of the given inode
 * \par Description:
 * Expand the data stream of the given inode by the given amount.  First, try and expand the direct
 * range.  If that isn't enough, expand the indirect range.  If that isn't enough, expand the
 * double-indirect range.  If that isn't enough, the file is too big.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode to expand
 * \param nDeltaSize	Amount by which to increase expand file
 * \return 0 on success, negative error code on failure
 * \sa
  ****************************************************************************/
int afs_expand_stream( AfsVolume_s * psVolume, AfsInode_s * psInode, off_t nDeltaSize )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	DataStream_s *const psStream = &psInode->ai_sData;
	int nDataGroup = psInode->ai_sInodeNum.group;
	int nSize = nDeltaSize;
	off_t nBlocksAdded = 0;
	int nError = 0;
	bool bDiskFull = false;

	if( S_ISDIR( psInode->ai_nMode ) == false )
	{
		nDataGroup++;
	}
//  printk( "Expand stream with %d blocks\n",(int) nDeltaSize );

	if( psStream->ds_nMaxIndirectRange == 0 && psStream->ds_nMaxDoubleIndirectRange == 0 )
	{
		BlockRun_s sNewRun;
		off_t nOldSize = psStream->ds_nMaxDirectRange;
		int i;

		sNewRun.len = 0;

		for( i = 0; i < DIRECT_BLOCK_COUNT && nSize > 0; ++i )
		{
			if( nOldSize > 0 )
			{
				nOldSize -= psStream->ds_asDirect[i].len;
				if( nOldSize > 0 )
				{
					if( psStream->ds_asDirect[i].len == 0 )
					{
						panic( "afs_expand_stream:1() inconsistency between ds_nMaxDirectRange and block runs\n" );
					}
					continue;
				}
				if( nOldSize < 0 )
				{
					panic( "afs_expand_stream:2() inconsistency between ds_nMaxDirectRange and block runs.\n" );
				}
			}

			if( sNewRun.len == 0 )
			{
				int j;

				nError = afs_alloc_blocks( psVolume, &sNewRun,( ( psStream->ds_asDirect[i].len > 0 ) ? &psStream->ds_asDirect[i] : NULL ), nDataGroup, nSize, 1 );
				if( nError < 0 )
				{
					printk( "afs_expand_stream() failed to alloc datablocks for direct range %d\n", nError );
					if( nError == -ENOSPC && nBlocksAdded > 0 )
					{
						nError = 0;
						nSize = 0;
						bDiskFull = true;
						break;
					}
					goto error;
				}
				if( S_ISDIR( psInode->ai_nMode ) == false )
				{
					off_t nRunStart = afs_run_to_num( psVolume->av_psSuperBlock, &sNewRun );

					for( j = 0; j < sNewRun.len; ++j )
					{
						afs_wait_for_block_to_leave_log( psVolume, nRunStart + j );
					}
				}
			}
			if( psStream->ds_asDirect[i].len > 0 )
			{
				if( psStream->ds_asDirect[i].group == sNewRun.group && ( psStream->ds_asDirect[i].start + psStream->ds_asDirect[i].len ) == sNewRun.start )
				{
					uint nUsed;

					if( psStream->ds_asDirect[i].start + psStream->ds_asDirect[i].len + sNewRun.len > psSuperBlock->as_nBlockPerGroup )
					{
						nUsed = psSuperBlock->as_nBlockPerGroup -( psStream->ds_asDirect[i].start + psStream->ds_asDirect[i].len );
						if( nUsed > AFS_MAX_RUN_LENGTH )
							nUsed = AFS_MAX_RUN_LENGTH;
					}
					else
					{
						nUsed = sNewRun.len;
					}
					if( nUsed > sNewRun.len )
					{
						panic( "afs_expand_stream() to many blocks taken from run(%d of %d)\n", nUsed, sNewRun.len );
					}
					if( nUsed > 0 )
					{
						psStream->ds_asDirect[i].len += nUsed;
						sNewRun.start += nUsed;
						sNewRun.len -= nUsed;
						nSize -= nUsed;
						nBlocksAdded += nUsed;
						psStream->ds_nMaxDirectRange += nUsed;
					}
				}
			}
			else
			{
				psStream->ds_asDirect[i] = sNewRun;
				nSize -= sNewRun.len;
				nBlocksAdded += sNewRun.len;
				psStream->ds_nMaxDirectRange += sNewRun.len;
				sNewRun.len = 0;
			}
		}
		if( sNewRun.len > 0 )
		{
			afs_free_blocks( psVolume, &sNewRun );
		}
	}
	if( bDiskFull == false )
	{
		if( nSize > 0 && nError >= 0 && psStream->ds_nMaxDoubleIndirectRange == 0 )
		{
			nError = afs_expand_indirect_range( psVolume, psInode, nSize, &bDiskFull );
			if( nError < 0 )
			{
				printk( "afs_expand_stream() failed to alloc blocks for indirect range %d\n", nError );
				goto error;
			}
			nSize -= nError;
		}
	}
	if( bDiskFull == false && nSize > 0 )
	{
		nSize =( nSize + BLOCKS_PER_DI_RUN - 1 ) & ~( BLOCKS_PER_DI_RUN - 1 );
		nError = afs_expand_doubleindirect_range( psVolume, psInode, nSize );

		if( nError > 0 )
		{
			nSize -= nError;
			nError = 0;
		}
		else
		{
			if( nError == 0 )
			{
				printk( "afs_expand_stream() max file size reached\n" );
				nError = -EFBIG;
			}
			else
			{
				printk( "afs_expand_stream() failed to alloc blocks for double indirect range %d\n", nError );
			}
		}
	}
      error:
	afs_do_write_inode( psVolume, psInode );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_trunc_indirect( AfsVolume_s * psVolume, DataStream_s * psStream, off_t nDeltaSize )
{
	off_t nOrigDelta = nDeltaSize;
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );

	off_t nIndirectBlock = afs_run_to_num( psSuperBlock, &psStream->ds_sIndirect );
	BlockRun_s *pasDirectBlock;
	int nError;
	int i;

	if( nDeltaSize == 0 )
	{			// Seems like something went wrong during an allocation.
		if( psStream->ds_sIndirect.len == 0 )
		{
			afs_free_blocks( psVolume, &psStream->ds_sIndirect );
			psStream->ds_sIndirect.len = 0;
			psStream->ds_nMaxIndirectRange = 0;
		}
		else
		{
			printk( "Error: Request for removing 0 bytes from indirect range but there is no indirect block!\n" );
		}
		return( 0 );
	}
	pasDirectBlock = afs_alloc_block_buffer( psVolume );
	if( pasDirectBlock == NULL )
	{
		printk( "Error: afs_trunc_indirect() no memory for pointer block\n" );
		return( -ENOMEM );
	}
	for( i = psStream->ds_sIndirect.len - 1; i >= 0 && nDeltaSize > 0; --i )
	{
		bool bDirty = false;
		int j;

		nError = afs_logged_read( psVolume, pasDirectBlock, nIndirectBlock + i );

		if( nError < 0 )
		{
			printk( "Error: afs_trunc_indirect() failed to load indirect block\n" );
			goto error;
		}
		for( j = nPtrsPerBlk - 1; j >= 0 && nDeltaSize > 0; --j )
		{
			if( pasDirectBlock[j].len == 0 )
			{
				continue;
			}
			bDirty = true;
			if( pasDirectBlock[j].len <= nDeltaSize )
			{
				nError = afs_free_blocks( psVolume, &pasDirectBlock[j] );
				if( nError < 0 )
				{
					printk( "Error: afs_trunc_indirect:1() failed to free data-run: %d\n", nError );
					goto error;
				}
				nDeltaSize -= pasDirectBlock[j].len;
				psStream->ds_nMaxIndirectRange -= pasDirectBlock[j].len;
				pasDirectBlock[j].len = 0;
			}
			else
			{
				BlockRun_s sRun = pasDirectBlock[j];

				sRun.group = pasDirectBlock[j].group;
				sRun.start = pasDirectBlock[j].start + pasDirectBlock[j].len - nDeltaSize;
				sRun.len = nDeltaSize;

				pasDirectBlock[j].len -= nDeltaSize;
				psStream->ds_nMaxIndirectRange -= nDeltaSize;
				nError = afs_free_blocks( psVolume, &sRun );
				if( nError < 0 )
				{
					printk( "Error: afs_trunc_indirect:2() failed to free data-run: %d\n", nError );
					goto error;
				}
				nDeltaSize = 0;
			}
		}
		if( 0 == i && 0 == pasDirectBlock[0].len )
		{
			nError = afs_free_blocks( psVolume, &psStream->ds_sIndirect );
			if( nError < 0 )
			{
				printk( "Error: afs_trunc_indirect() failed to free indirect block: %d\n", nError );
				goto error;
			}
			psStream->ds_sIndirect.len = 0;
			psStream->ds_nMaxIndirectRange = 0;
		}
		if( bDirty )
		{
			nError = afs_logged_write( psVolume, pasDirectBlock, nIndirectBlock + i );
			if( nError < 0 )
			{
				printk( "Error: afs_trunc_indirect() failed to write pointer block\n" );
				goto error;
			}
		}
	}
	if( nDeltaSize != 0 )
	{
		printk( "%Ld, %Ld, %Ld, %Ld(%Ld)\n", psStream->ds_nMaxDirectRange, psStream->ds_nMaxIndirectRange, psStream->ds_nMaxDoubleIndirectRange, psStream->ds_nSize, nOrigDelta );

		printk( "OrigDelga : %Ld, CurDelta : %Ld\n", nOrigDelta, nDeltaSize );
		printk( "psStream = %p\n", psStream );
		printk( "ds_nMaxDirectRange         : %Ld\n", psStream->ds_nMaxDirectRange );
		printk( "ds_nMaxIndirectRange       : %Ld\n", psStream->ds_nMaxIndirectRange );
		printk( "ds_nMaxDoubleIndirectRange : %Ld\n", psStream->ds_nMaxDoubleIndirectRange );
		printk( "ds_nSize                   : %Ld\n", psStream->ds_nSize );
		panic( "afs_trunc_indirect() still %Ld bytes left after truncating!(%Ld:%d)\n", nDeltaSize, nIndirectBlock, psStream->ds_sIndirect.len );
	}
	kassertw( psStream->ds_nMaxIndirectRange >= 0 );

	if( psStream->ds_nMaxIndirectRange != 0 && psStream->ds_nMaxIndirectRange <= psStream->ds_nMaxDirectRange )
	{
		printk( "Panic: afs_trunc_indirect() direct range(%d) larger than indirect range (%d)!\n", ( int )psStream->ds_nMaxDirectRange, ( int )psStream->ds_nMaxIndirectRange );
	}
	afs_free_block_buffer( psVolume, pasDirectBlock );
	return( 0 );
      error:
	afs_free_block_buffer( psVolume, pasDirectBlock );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static off_t afs_trunc_double_indirect( AfsVolume_s * psVolume, DataStream_s * psStream, off_t nDeltaSize )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;

	const int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );

	// ([nIDBlk][nIDPtr])([nDBlk][nDPtr])[nBlk]
	//    0-3     0-127     0-3    0-127   0-3

	const off_t nCurPos = psStream->ds_nMaxDoubleIndirectRange - psStream->ds_nMaxIndirectRange - 1;

	const int nDPtrSize = BLOCKS_PER_DI_RUN;
	const int nDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk;
	const int nIDPtrSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN;
	const int nIDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN * nPtrsPerBlk;

	int nDPtr =( nCurPos / nDPtrSize ) % nPtrsPerBlk;
	int nDBlk =( nCurPos / nDBlkSize ) % BLOCKS_PER_DI_RUN;
	int nIDPtr =( nCurPos / nIDPtrSize ) % nPtrsPerBlk;
	int nIDBlk =( nCurPos / nIDBlkSize );
	int nIndirectBlk;

	BlockRun_s *psIndirectBlock;
	BlockRun_s *psDirectBlock;
	int nError = 0;

	psDirectBlock = kmalloc( nBlockSize * 2, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psDirectBlock == NULL )
	{
		printk( "Error: afs_trunc_double_indirect() no memory for pointer buffers\n" );
		return( -ENOMEM );
	}
	psIndirectBlock =( BlockRun_s * ) ( ( ( uint8 * )psDirectBlock ) + nBlockSize );

	kassertw( 0 != psStream->ds_sDoubleIndirect.len );

	if( psStream->ds_sDoubleIndirect.len == 0 )
	{
		printk( "afs_trunc_double_indirect() : DIB size: 0, range: %08x%08x\n",( int )( psStream->ds_nMaxDoubleIndirectRange >> 32 ), ( int )( psStream->ds_nMaxDoubleIndirectRange & 0xffffffff ) );
		printk( "Size:        %d\n",( int )psStream->ds_nSize );
		printk( "Direct:      %d\n",( int )psStream->ds_nMaxDirectRange );
		printk( "Indirect:    %d\n",( int )psStream->ds_nMaxIndirectRange );
		printk( "DblIndirect: %d\n",( int )psStream->ds_nMaxDoubleIndirectRange );
	}

	nIndirectBlk = afs_run_to_num( psSuperBlock, &psStream->ds_sDoubleIndirect );

	// Iterate through all indirect blocks
	for( ; nIDBlk >= 0 && nDeltaSize >= BLOCKS_PER_DI_RUN && nError >= 0; --nIDBlk )
	{
		bool bIndirectBlockDirty = false;

		nError = afs_logged_read( psVolume, psIndirectBlock, nIndirectBlk + nIDBlk );
		if( nError < 0 )
		{
			printk( "Error: afs_trunc_double_indirect() failed to load indirect block: %d\n", nError );
			break;
		}
		// Iterate through all direct blocks
		for( ; nIDPtr >= 0 && nDeltaSize >= BLOCKS_PER_DI_RUN && nError >= 0; --nIDPtr )
		{
			off_t nDirectBlkNum;

			kassertw( 0 != psIndirectBlock[nIDPtr].len );
			nDirectBlkNum = afs_run_to_num( psSuperBlock, &psIndirectBlock[nIDPtr] );

			// Iterate through all fs-blocks building the direct block
			for( ; nDBlk >= 0 && nDeltaSize >= BLOCKS_PER_DI_RUN && nError >= 0; --nDBlk )
			{
				nError = afs_logged_read( psVolume, psDirectBlock, nDirectBlkNum + nDBlk );
				if( nError < 0 )
				{
					printk( "Error: afs_trunc_double_indirect() failed to load direct block from disk: %d\n", nError );
					break;
				}
				// Free data blocks from the direct block
				for( ; nDPtr >= 0 && nDeltaSize >= BLOCKS_PER_DI_RUN && nError >= 0; --nDPtr )
				{
					kassertw( BLOCKS_PER_DI_RUN == psDirectBlock[nDPtr].len );
					nError = afs_free_blocks( psVolume, &psDirectBlock[nDPtr] );
					if( nError < 0 )
					{
						printk( "Error: afs_trunc_double_indirect() failed to free data-run: %d\n", nError );
						break;
					}
					nDeltaSize -= psDirectBlock[nDPtr].len;
					psStream->ds_nMaxDoubleIndirectRange -= psDirectBlock[nDPtr].len;
					psDirectBlock[nDPtr].len = 0;
				}
				// Free the direct block if it gets empty
				if( nDBlk == 0 && psDirectBlock[0].len == 0 )
				{
					nError = afs_free_blocks( psVolume, &psIndirectBlock[nIDPtr] );
					if( nError < 0 )
					{
						printk( "Error: afs_trunc_double_indirect() failed to free indirect block: %d\n", nError );
						break;
					}
					psIndirectBlock[nIDPtr].len = 0;
					bIndirectBlockDirty = true;
				}
				else
				{
					// FIXME : No need to logg it if we free it later.
					nError = afs_logged_write( psVolume, psDirectBlock, nDirectBlkNum + nDBlk );
					if( nError < 0 )
					{
						printk( "Panic: afs_trunc_double_indirect() failed to write direct block\n" );
						break;
					}
				}
				nDPtr = nPtrsPerBlk - 1;
			}
			nDBlk = BLOCKS_PER_DI_RUN - 1;
		}

		nIDPtr = nPtrsPerBlk - 1;

		if( 0 == nIDBlk && 0 == psIndirectBlock[0].len )
		{
//      printk( "Delete double indirect block\n" );
			nError = afs_free_blocks( psVolume, &psStream->ds_sDoubleIndirect );
			if( nError < 0 )
			{
				printk( "Error: afs_trunc_double_indirect() failed to free double indirect block: %d\n", nError );
				break;
			}
			psStream->ds_sDoubleIndirect.len = 0;
			psStream->ds_nMaxDoubleIndirectRange = 0;
		}
		else
		{
			if( bIndirectBlockDirty )
			{
				nError = afs_logged_write( psVolume, psIndirectBlock, nIndirectBlk + nIDBlk );
				if( nError < 0 )
				{
					printk( "Panic: afs_trunc_double_indirect() failed to write indirect block\n" );
					break;
				}
			}
		}
	}
	kfree( psDirectBlock );
	kassertw( nError <= 0 );
	kassertw( nDeltaSize >= 0 && nDeltaSize < BLOCKS_PER_DI_RUN );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_truncate_stream( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	DataStream_s *psStream = &psInode->ai_sData;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	off_t nOldSize = afs_get_inode_block_count( psInode );
	off_t nNewSize =( psStream->ds_nSize + nBlockSize - 1 ) / nBlockSize;
	status_t nError = 0;

	if( nNewSize >= nOldSize )
	{
		return( 0 );
	}
//    printk( "afs_truncate_file() truncate file from %d to %d\n",(int)nOldSize, (int)nNewSize );
	if( nNewSize < psStream->ds_nMaxDoubleIndirectRange )
	{
		off_t nDeltaSize =( nNewSize > psStream->ds_nMaxIndirectRange ) ? ( psStream->ds_nMaxDoubleIndirectRange - nNewSize ) : ( psStream->ds_nMaxDoubleIndirectRange - psStream->ds_nMaxIndirectRange );

		if( nDeltaSize > 256 * 1024 && S_ISREG( psInode->ai_nMode ) && ( psInode->ai_nFlags & INF_LOGGED ) == 0 )
		{
			while( nDeltaSize > 0 )
			{
				off_t nCurDelta = min( nDeltaSize, 256 * 1024 );

				nError = afs_trunc_double_indirect( psVolume, psStream, nCurDelta );
				if( nError < 0 )
				{
					goto error;
				}
				nDeltaSize -= nCurDelta;
				printk( "afs_truncate_stream() file very large, split transaction\n" );
				if( afs_validate_inode( psVolume, psInode ) != 0 )
				{
					panic( "afs_truncate_stream() invalidated inode!\n" );
				}
				nError = afs_do_write_inode( psVolume, psInode );
				if( nError < 0 )
				{
					goto error;
				}
				nError = afs_checkpoint_journal( psVolume );
				if( nError < 0 )
				{
					goto error;
				}
			}
		}
		else
		{
			nError = afs_trunc_double_indirect( psVolume, psStream, nDeltaSize );
			if( nError < 0 )
			{
				goto error;
			}
		}
	}
	if( nNewSize < psStream->ds_nMaxIndirectRange )
	{
		off_t nDeltaSize =( nNewSize > psStream->ds_nMaxDirectRange ) ? ( psStream->ds_nMaxIndirectRange - nNewSize ) : ( psStream->ds_nMaxIndirectRange - psStream->ds_nMaxDirectRange );

//      printk( "Remove %d blocks from indirect range %d\n",
//           (int) nDeltaSize, (int)psStream->ds_nMaxIndirectRange );
		afs_trunc_indirect( psVolume, psStream, nDeltaSize );
	}
	if( nNewSize < psStream->ds_nMaxDirectRange )
	{
		off_t nDeltaSize = psStream->ds_nMaxDirectRange - nNewSize;
		int i;

//      printk( "Remove %d blocks from direct range(%d)\n", (int) nDeltaSize, (int)psStream->ds_nMaxDirectRange );

		for( i = DIRECT_BLOCK_COUNT - 1; i >= 0 && nDeltaSize > 0; --i )
		{
			if( psStream->ds_asDirect[i].len > 0 )
			{
				if( psStream->ds_asDirect[i].len <= nDeltaSize )
				{
					nError = afs_free_blocks( psVolume, &psStream->ds_asDirect[i] );
					if( nError < 0 )
					{
						goto error;
					}
					nDeltaSize -= psStream->ds_asDirect[i].len;
					psStream->ds_nMaxDirectRange -= psStream->ds_asDirect[i].len;
					psStream->ds_asDirect[i].len = 0;
				}
				else
				{
					BlockRun_s sRun;

					sRun.group = psStream->ds_asDirect[i].group;
					sRun.start = psStream->ds_asDirect[i].start + psStream->ds_asDirect[i].len - nDeltaSize;
					sRun.len = nDeltaSize;

					psStream->ds_asDirect[i].len -= nDeltaSize;
					psStream->ds_nMaxDirectRange -= nDeltaSize;

					nError = afs_free_blocks( psVolume, &sRun );
					if( nError < 0 )
					{
						goto error;
					}
					nDeltaSize = 0;
				}
			}
		}
		kassertw( 0 == nDeltaSize );
	}
	kassertw( afs_get_inode_block_count( psInode ) * nBlockSize >= psStream->ds_nSize );
	afs_validate_inode( psVolume, psInode );
	nError = afs_do_write_inode( psVolume, psInode );
      error:
	return( nError );
}

/** Get a set of blocks from a file stream
 * \par Description:
 * Get a set of blocks of nRequestLen length starting at nPos from the given stream.  The
 * resulting blocks will start at pnStart and be of length pnActualLength.  First, see if
 * the request position is in the direct block range of the given stream. (XXX DFG Should
 * this check for total length as well?).  If so, walk the direct block list, looking for
 * the direct block containing the requested offset.  Then, starting from that point,
 * walk the direct block list looking for the block containing the end of the requested
 * range.  Return the block number and actual length.
 * Otherwise, if the offset is in the indirect range, walk the indirect block list, on
 * blockrun at a time, looking for the range containing the requested offset.  Then, walk
 * from that point, looking for the range containing the end of the requested range.
 * Return the block number and actual length.
 * Otherwise, it must be in the double-indirect range, error out if it's not.  Walk the
 * double-indirect block run, the indirect block runs within it, and the direct block
 * lists within those, looking for the block list that contains the offset.  Then, walk
 * from that point looking for the end.  Return the block number and actual length.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume containing stream
 * \param psStream		Data stream to search
 * \param nPos			Start offset into stream (in octets)
 * \param nRequestLen	Requested length of blocks (in octets)
 * \param pnStart		(out) Block number of found range
 * \param pnActualLength	(out) Actual found length (in octets)
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int afs_get_stream_blocks( AfsVolume_s * psVolume, DataStream_s * psStream, off_t nPos, int nRequestLen, off_t *pnStart, int *pnActualLength )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	int nBlockSize = psSuperBlock->as_nBlockSize;
	off_t nStart = -1;
	int nLen = 0;
	int nError = 0;

	if( nPos < psStream->ds_nMaxDirectRange )
	{
		off_t nCurPos = 0;
		int i;

		for( i = 0; i < DIRECT_BLOCK_COUNT; ++i )
		{
			if( nPos >= nCurPos && nPos < nCurPos + psStream->ds_asDirect[i].len )
			{
				off_t nOff = nPos - nCurPos;
				int j;

				nStart = afs_run_to_num( psSuperBlock, &psStream->ds_asDirect[i] ) + nOff;
				nLen = psStream->ds_asDirect[i].len - nOff;

				kassertw( nLen > 0 );

				for( j = i + 1; j < DIRECT_BLOCK_COUNT && nLen < nRequestLen; ++j )
				{
					int nBlkNum = afs_run_to_num( psSuperBlock, &psStream->ds_asDirect[j] );

					// XXX DFG should this be <=?
					if( nStart + nLen == nBlkNum )
					{
						nLen += psStream->ds_asDirect[j].len;
					}
					else
					{
						break;
					}
				}
				break;
			}
			nCurPos += psStream->ds_asDirect[i].len;
		}
	}
	else
	{
		if( nPos < psStream->ds_nMaxIndirectRange )
		{
			off_t nBlk = afs_run_to_num( psVolume->av_psSuperBlock, &psStream->ds_sIndirect );
			off_t nCurPos = psStream->ds_nMaxDirectRange;
			int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );
			BlockRun_s *psBlock;
			int i;

			psBlock = afs_alloc_block_buffer( psVolume );

			if( psBlock == NULL )
			{
				printk( "Error: afs_get_stream_blocks() no memory for indirect pointer block\n" );
				nError = -ENOMEM;
				goto error;
			}


			for( i = 0; i < psStream->ds_sIndirect.len && -1 == nStart; ++i, ++nBlk )
			{
				int j;

				nError = afs_logged_read( psVolume, psBlock, nBlk );
				if( nError < 0 )
				{
					printk( "Error: afs_get_stream_blocks() failed to load indirect pointer block\n" );
					break;
				}
				for( j = 0; j < nPtrsPerBlk; ++j )
				{
					if( psBlock[j].len <= 0 )
					{
						panic( "afs_get_stream_blocks() indirect pointer block %d:%d has invalid length %d\n", i, j, psBlock[j].len );
						nError = -EINVAL;
						break;
					}
//        kassertw( psBlock[ j ].len > 0 );
					if( nPos >= nCurPos && nPos < nCurPos + psBlock[j].len )
					{
						int nOff = nPos - nCurPos;
						int k;

						nStart = afs_run_to_num( psSuperBlock, &psBlock[j] ) + nOff;
						nLen = psBlock[j].len - nOff;

						kassertw( nLen > 0 );

						for( k = j + 1; k < nPtrsPerBlk && nLen < nRequestLen; ++k )
						{
							int nBlkNum = afs_run_to_num( psSuperBlock, &psBlock[k] );

							if( nStart + nLen == nBlkNum )
							{
								nLen += psBlock[k].len;
							}
							else
							{
								break;
							}
						}
						break;
					}
					nCurPos += psBlock[j].len;
				}
			}
			afs_free_block_buffer( psVolume, psBlock );
		}
		else
		{
			int nPtrsPerBlk = nBlockSize / sizeof( BlockRun_s );

			// ([nIDBlk][nIDPtr])([nDBlk][nDPtr])[nBlk]

			const off_t nCurPos = nPos - psStream->ds_nMaxIndirectRange;
			const int nDPtrSize = BLOCKS_PER_DI_RUN;
			const int nDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk;
			const int nIDPtrSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN;
			const int nIDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk * BLOCKS_PER_DI_RUN * nPtrsPerBlk;

			const int nOff = nCurPos % BLOCKS_PER_DI_RUN;
			const int nDPtr =( nCurPos / nDPtrSize ) % nPtrsPerBlk;
			const int nDBlk =( nCurPos / nDBlkSize ) % BLOCKS_PER_DI_RUN;
			const int nIDPtr =( nCurPos / nIDPtrSize ) % nPtrsPerBlk;
			const int nIDBlk =( nCurPos / nIDBlkSize );
			const int nIndirectBlk = afs_run_to_num( psSuperBlock, &psStream->ds_sDoubleIndirect ) + nIDBlk;
			int nDirectBlk;
			BlockRun_s *psBlock;
			int i;

			if( nPos >= psStream->ds_nMaxDoubleIndirectRange )
			{
				printk( "Error : afs_get_stream_blocks() request for blocks %d outside double indirect range %d\n",( int )nPos, ( int )psStream->ds_nMaxDoubleIndirectRange );
				nError = -EINVAL;
				goto error;
			}

			kassertw( 0 != psStream->ds_nMaxDoubleIndirectRange );
			kassertw( nPos < psStream->ds_nMaxDoubleIndirectRange );

			psBlock = afs_alloc_block_buffer( psVolume );

			if( psBlock == NULL )
			{
				printk( "afs_get_stream_blocks() no memory for double indirect pointer block\n" );
				nError = -ENOMEM;
				goto error;
			}
			// Load the indirect block
			nError = afs_logged_read( psVolume, psBlock, nIndirectBlk );
			if( nError < 0 )
			{
				afs_free_block_buffer( psVolume, psBlock );
				goto error;
			}
			// Fetch the direct block pointer and read it into the buffer
			nDirectBlk = afs_run_to_num( psSuperBlock, &psBlock[nIDPtr] ) + nDBlk;
			nError = afs_logged_read( psVolume, psBlock, nDirectBlk );
			if( nError < 0 )
			{
				afs_free_block_buffer( psVolume, psBlock );
				goto error;
			}

			nStart = afs_run_to_num( psSuperBlock, &psBlock[nDPtr] ) + nOff;
			nLen = psBlock[nDPtr].len - nOff;

			kassertw( nLen > 0 );

			for( i = nDPtr + 1; i < nPtrsPerBlk && nLen < nRequestLen; ++i )
			{
				off_t nBlkNum = afs_run_to_num( psSuperBlock, &psBlock[i] );

				if( nStart + nLen == nBlkNum )
				{
					nLen += psBlock[i].len;
				}
				else
				{
					break;
				}
			}
			afs_free_block_buffer( psVolume, psBlock );
		}
	}

	if( nError == 0 )
	{
		*pnStart = nStart;
		*pnActualLength = nLen;
	}
	else
	{
		printk( "afs_get_stream_blocks() Error = %d\n", nError );
	}
      error:
	return( nError );
}
