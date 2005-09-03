
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
#include <macros.h>

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>

#include "afs.h"
#include "btree.h"
static int g_nReads;

/** Count the number of used blocks in a volume
 * \par Description:
 * Count the number of used blocks in the given volume.  Get the bitmap block from
 * the volume, and run through the bitmap block, counting all the set bits.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return number of used blocks on success, negative error code on failure
 * \sa
 *****************************************************************************/
off_t afs_count_used_blocks( AfsVolume_s * psVolume )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBlocksPerGroup = psSuperBlock->as_nBlockPerGroup /( nBlockSize * 8 );
	const int nWordsPerBlock = nBlockSize / 4;
	uint32 *panBlock;
	off_t nCount = 0;
	int i;

	panBlock = afs_alloc_block_buffer( psVolume );

	if( panBlock == NULL )
	{
		printk( "Error: afs_count_used_blocks() no memory for bitmap buffer\n" );
		return( -ENOMEM );
	}

	for( i = 0; i < psSuperBlock->as_nAllocGroupCount * nBlocksPerGroup; ++i )
	{
		int j;
		int nError;

		nError = afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + i );

		if( nError < 0 )
		{
			printk( "PANIC : afs_count_used_blocks() failed to load bitmap block %d\n", i );
			afs_free_block_buffer( psVolume, panBlock );
			return( nError );
		}
		for( j = 0; j < nWordsPerBlock; ++j )
		{
			int k;

			if( panBlock[j] == 0 )
			{
				continue;
			}
			if( panBlock[j] == ~0 && i * nWordsPerBlock * 32 + j * 32 + 32 < psSuperBlock->as_nNumBlocks )
			{
				nCount += 32;
				continue;
			}
			for( k = 0; k < 32; ++k )
			{
				if( i * nWordsPerBlock * 32 + j * 32 + k >= psSuperBlock->as_nNumBlocks )
				{
					if( ( panBlock[j] & ( 1L << k ) ) == 0 )
					{
						printk( "Panic: afs_count_used_blocks() block past the partition-end are marked as free\n" );
					}
//                  break;
				}
				else
				{
					if( ( panBlock[j] & ( 1L << k ) ) != 0 )
					{
						nCount++;
					}
				}
			}
		}
	}
	afs_free_block_buffer( psVolume, panBlock );

	return( nCount );
}

/** Mark blocks used in a volume
 * \par Description:
 * Mark the blocks in the given run used in the given volume.  First, get the block
 * bitmap from the volume.  Then, for all the bits representing blocks in the run,
 * mark set them.  Write the block bitmap back out.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psRun		Block run to mark as used
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_mark_blocks_used( AfsVolume_s * psVolume, const BlockRun_s * psRun )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBlocksPerGroup = psSuperBlock->as_nBlockPerGroup /( nBlockSize * 8 );
	int nBlock = psRun->start /( nBlockSize * 8 );
	int nWord =( psRun->start / 32 ) % ( nBlockSize / 4 );
	int nBit = psRun->start % 32;
	int nSize = psRun->len;
	uint32 *panBlock;
	int nError;

	panBlock = afs_alloc_block_buffer( psVolume );

	if( panBlock == NULL )
	{
		printk( "Error: afs_mark_blocks_used() no memory for bitmap buffer\n" );
		return( -ENOMEM );
	}
//  printk( "afs_mark_blocks_used( %d %d %d ) nBlock = %d\n",
//        psRun->group, psRun->start, psRun->len, nBlock );

	for( ; nBlock < nBlocksPerGroup && nSize > 0; ++nBlock )
	{
		nError = afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + psRun->group * nBlocksPerGroup + nBlock );
		g_nReads++;

		if( nError < 0 )
		{
			printk( "Error: afs_mark_blocks_used() failed to load bitmap block %d\n", nBlock );
			afs_free_block_buffer( psVolume, panBlock );
			return( nError );
		}
		for( ; nWord < ( nBlockSize / 4 ) && nSize > 0; ++nWord, nBit = 0 )
		{
			uint32 nMask;

			if( nSize >= 32 && 0 == nBit )
			{
				if( panBlock[nWord] != 0 )
				{
					panic( "Attempt to alloc used block! %d:%d:%d", psRun->group, nBlock, nWord );
				}
				panBlock[nWord] = ~0;
				nSize -= 32;
				continue;
			}
			for( nMask = 1L << nBit; nBit < 32 && nSize > 0; ++nBit, nMask <<= 1 )
			{
				if( ( nMask & panBlock[nWord] ) != 0 )
				{
					panic( "Attempt to alloc used block! %d:%d:%d:%d Mask = %08x", psRun->group, nBlock, nWord, nBit, nMask );
				}
				panBlock[nWord] |= nMask;
				nSize--;
			}
			nBit = 0;
		}
		nWord = 0;
		nError = afs_logged_write( psVolume, panBlock, psVolume->av_nBitmapStart + psRun->group * nBlocksPerGroup + nBlock );
		if( nError < 0 )
		{
			printk( "Error: afs_mark_blocks_used() failed to write bitmap block: %d\n", nError );
			afs_free_block_buffer( psVolume, panBlock );
			return( nError );
		}
	}
	afs_free_block_buffer( psVolume, panBlock );
	kassertw( nSize == 0 );
	psSuperBlock->as_nUsedBlocks += psRun->len;
	psVolume->as_nTransAllocatedBlocks += psRun->len;

	if( psSuperBlock->as_nUsedBlocks > psSuperBlock->as_nNumBlocks )
	{
		panic( "afs_mark_blocks_used() FS got %d free blocks!\n",( int )( psSuperBlock->as_nNumBlocks - psSuperBlock->as_nUsedBlocks ) );
	}

	return( 0 );
}

/** Mark blocks free in a volume
 * \par Description:
 * Mark the blocks in the given block run as free in the given volume. First, get
 * the block bitmap from the volume.  Then, for each block in the run, mark it's bit
 * in the bitmap free.  Finally, write back the bitmap.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psRun		Block run containing block to mark free
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_free_blocks( AfsVolume_s * psVolume, const BlockRun_s * psRun )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBlocksPerGroup = psSuperBlock->as_nBlockPerGroup /( nBlockSize * 8 );
	const int nWordsPerBlock = nBlockSize / 4;
	int nBlock = psRun->start /( nBlockSize * 8 );
	int nWord =( psRun->start / 32 ) % nWordsPerBlock;
	int nBit = psRun->start % 32;
	int nSize = psRun->len;
	uint32 *panBlock;
	int nError;

	if( psRun->start + psRun->len > psSuperBlock->as_nBlockPerGroup )
	{
		panic( "afs_free_blocks() Attempt to free invalid run %d, %d, %d\n", psRun->group, psRun->start, psRun->len );
		return( -EINVAL );
	}

	panBlock = afs_alloc_block_buffer( psVolume );

	if( panBlock == NULL )
	{
		printk( "Error: afs_free_blocks() no memory for bitmap buffer\n" );
		return( -ENOMEM );
	}

	for( ; nBlock < nBlocksPerGroup && nSize > 0; ++nBlock )
	{
		nError = afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + psRun->group * nBlocksPerGroup + nBlock );
		g_nReads++;
		if( nError < 0 )
		{
			printk( "PANIC : afs_free_blocks() failed to load bitmap block %d\n", nBlock );
			afs_free_block_buffer( psVolume, panBlock );
			return( nError );
		}
		for( ; nWord < nWordsPerBlock && nSize > 0; ++nWord, nBit = 0 )
		{
			uint32 nMask;

			if( nSize >= 32 && 0 == nBit )
			{
				if( panBlock[nWord] != ~0 )
				{
					panic( "Attempt to free unused block! %d:%d:%d", psRun->group, nBlock, nWord );
				}
				panBlock[nWord] = 0;
				nSize -= 32;
				continue;
			}
			for( nMask = 1L << nBit; nBit < 32 && nSize > 0; ++nBit, nMask <<= 1 )
			{
				if( ( nMask & panBlock[nWord] ) != nMask )
				{
					panic( "Attempt to free unused block! %d:%d:%d:%d Mask = %08x", psRun->group, nBlock, nWord, nBit, nMask );
				}
				panBlock[nWord] &= ~nMask;
				nSize--;
			}
			nBit = 0;
		}
		nWord = 0;
		nError = afs_logged_write( psVolume, panBlock, psVolume->av_nBitmapStart + psRun->group * nBlocksPerGroup + nBlock );
		if( nError < 0 )
		{
			printk( "Error: afs_free_blocks() failed to write bitmap block to log\n" );
			afs_free_block_buffer( psVolume, panBlock );
			return( nError );
		}
	}
	afs_free_block_buffer( psVolume, panBlock );
	kassertw( nSize == 0 );
	psSuperBlock->as_nUsedBlocks -= psRun->len;
	psVolume->as_nTransAllocatedBlocks -= psRun->len;
	return( 0 );
}

/** Scan a block group for free blocks
 * \par Description:
 * Scan the given group of the given volume for the given number of free blocks,
 * storing the bitmap blocks into the given buffer, starting at the given location.
 * For each block in the bitmap starting at the given start location, get the bitmap
 * block.  Next, loop through all the bits.  If we haven't found a hole yet, and
 * there is a free block, start the hole.  If we have found a hole, and there is a
 * used block, stop the hole.  Return the hole.
 * \par Note:
 * This will find the first hole after pnStart.  If the hole is not big enough, the
 * caller will need to call again with pnStart set to the returned pnEnd + 1.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param apBlocks	Buffer to store block bitmap
 * \param nGroup	Number of group to scan
 * \param pnStart	Return argument for the start of the free blocks
 * \param pnEnd		Return argument for the end of the free blocks
 * \param nCount	Requested number of blocks
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
static int afs_scan_group( AfsVolume_s * psVolume, uint32 *apBlocks[], int nGroup, off_t *pnStart, off_t *pnEnd, int nCount )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBmBlocksPerGrp = psSuperBlock->as_nBlockPerGroup /( nBlockSize * 8 );
	const int nWordsPerBlock = nBlockSize / 4;
	int nBlock = *pnStart /( nBlockSize * 8 );
	int nWord =( *pnStart / 32 ) % nWordsPerBlock;
	int nBit = *pnStart % 32;
	off_t nStart = -1;
	off_t nEnd = -2;	// Make the size calculation equal 0.
	int32 *panBlock;
	bool bDone = false;
	int nError;
	int i;

//  printk( "Scan for %d blocks at %d in grp %d(%d , %d , %d)\n", nCount, (int)*pnStart, nGroup, nBlock, nWord, nBit );
	// Iterate through bitmap-block in this group
	for( i = 0; nBlock < nBmBlocksPerGrp && false == bDone; ++i, ++nBlock, nWord = 0 )
	{
		if( apBlocks[i] == NULL )
		{
			apBlocks[i] = afs_alloc_block_buffer( psVolume );
			if( apBlocks[i] == NULL )
			{
				printk( "Error: afs_free_blocks() no memory for bitmap buffer\n" );
				return( -ENOMEM );
			}
			nError = afs_logged_read( psVolume, apBlocks[i], psVolume->av_nBitmapStart + nGroup * nBmBlocksPerGrp + nBlock );
			g_nReads++;
			if( nError < 0 )
			{
				printk( "PANIC : afs_scan_group() failed to load bitmap block %d\n", nBlock );
				return( nError );
			}
		}
		panBlock = apBlocks[i];
		// Iterate through each 32 bit words in the current bitmap-block
		for( ; nWord < nWordsPerBlock && false == bDone; ++nWord, nBit = 0 )
		{
			uint32 nMask;

			if( nStart == -1 && 0 == nBit && panBlock[nWord] == ~0 )
			{
				continue;
			}
			if( nStart != -1 && 0 == nBit && panBlock[nWord] == 0 )
			{
				nEnd += 32;
				if( ( ( nEnd - nStart ) + 1 ) >= nCount )
				{
					bDone = true;
				}
				continue;
			}
			// Iterate through each bit in the current 32 bit word
			for( nMask = 1L << nBit; nBit < 32 && false == bDone; ++nBit, nMask <<= 1 )
			{
				if( ( nMask & panBlock[nWord] ) == 0 )
				{
					if( -1 == nStart )
					{
						nStart =( nBlock * nBlockSize * 8 ) + nWord * 32 + nBit;
						nEnd = nStart;
//          printk( "afs_scan_group() found start at %d\n", nStart );
					}
					else
					{
						nEnd++;
					}
				}
				else
				{
					if( -1 != nStart )
					{
						bDone = true;
					}
				}
			}
		}
	}

	*pnStart = nStart;

	if( ( nEnd - nStart ) + 1 > nCount )
	{
		*pnEnd = nStart + nCount - 1;
	}
	else
	{
		*pnEnd = nEnd;
	}
//  printk( "afs_scan_group() found end at %d\n", *pnEnd );
	return( ( *pnEnd - nStart ) + 1 );
}

/** Allocate a block run
 * \par Description:
 * Allocate a block run of the given size mod the given modulo.  If psPrevRun is
 * given, try to allocate the new run immediately after it.  Otherwise, try and
 * allocate in the given block group.  First, if psPrevRun is given, search
 * immediately after it for space of the correct size.  If we get anything back (?),
 * return it.  Otherwise, start at the given group, wrapping if necessary, and
 * search for a block run of at least the desired size.  When one is found, mark
 * it's blocks used, and return it.
 * \par Note:
 * Hoops are jumped through for apBlockBuffers.  Is this necessary?
 * \par Warning:
 * The modulo only works for powers of 2.  Since this is only called with 1 and 4,
 * this is okay. I'm not sure the modulo test is sufficiant.
 * \param psVolume	AFS filesystem pointer
 * \param psRun		Block run to fill in with allocated space
 * \param psPrevRun	If given, allocate new run immediately after this one.
 * \param nGroup	Block group to start searching in
 * \param nCount	Desired number of blocks
 * \param nModulo	Desired granularity modulo (must be power of 2)
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int afs_alloc_blocks( AfsVolume_s * psVolume, BlockRun_s * psRun, BlockRun_s * psPrevRun, int nGroup, int nCount, int nModulo )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nGroupCount = psSuperBlock->as_nAllocGroupCount;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	const int nBmBlocksPerGrp = psSuperBlock->as_nBlockPerGroup /( nBlockSize * 8 );
	const int nMask = ~( nModulo - 1 );
	uint32 *apBlockBuffers[nBmBlocksPerGrp];
	BlockRun_s sLargest;
	off_t nStart;
	off_t nEnd;
	int nError;
	int i;
	int j;

//  bigtime_t   nStartTime = read_pentium_clock();
//  bigtime_t   nEndTime;

	g_nReads = 0;

	if( ( nCount & ~nMask ) != 0 )
	{
		panic( "afs_alloc_blocks() count(%d) and modulo (%d) are inconsistent!\n", nCount, nModulo );
	}
	for( i = 0; i < nBmBlocksPerGrp; ++i )
	{
		apBlockBuffers[i] = NULL;
	}
	if( NULL != psPrevRun )
	{
		int nNewCount;

		nGroup = psPrevRun->group;
		nStart = psPrevRun->start + psPrevRun->len;
		nNewCount = afs_scan_group( psVolume, apBlockBuffers, psPrevRun->group, &nStart, &nEnd, nCount );

		if( nNewCount < 0 )
		{
			printk( "Error: afs_alloc_blocks:1() : afs_scan_group() failed with err = %d\n", nNewCount );
			return( nNewCount );
		}
		nNewCount &= nMask;
		if( nNewCount > 0 && nStart == psPrevRun->start + psPrevRun->len )
		{
			psRun->group = psPrevRun->group;
			psRun->start = nStart;
			psRun->len = nNewCount;
			goto found;
		}
	}
	if( nGroup >= nGroupCount )
	{
		nGroup =( nGroupCount > 8 ) ? 8 : 0;
	}
	sLargest.group = -1;
	sLargest.len = 0;

	for( i = nGroup, j = 0; j < nGroupCount; ++i, ++j )
	{
		int k;

		if( i >= nGroupCount )
		{
			i -= nGroupCount;
		}
		nStart = 0;
		nEnd = 0;
		for( k = 0; k < nBmBlocksPerGrp; ++k )
		{
			if( apBlockBuffers[k] != NULL )
			{
				afs_free_block_buffer( psVolume, apBlockBuffers[k] );
				apBlockBuffers[k] = NULL;
			}
		}
		for( ;; )
		{
			int nNewCount = afs_scan_group( psVolume, apBlockBuffers, i, &nStart, &nEnd, nCount );

			if( nNewCount < 0 )
			{
				printk( "Error: afs_alloc_blocks:2() : afs_scan_group() failed with err = %d\n", nNewCount );
				return( nNewCount );
			}
			if( ( nNewCount & nMask ) > sLargest.len )
			{
				sLargest.group = i;
				sLargest.start = nStart;
				sLargest.len = nNewCount & nMask;

				if( sLargest.len >= nCount )
				{
					*psRun = sLargest;
					goto found;
				}
			}
			if( nNewCount < 1 )
			{
				break;
			}
			nStart = nEnd + 1;
		}
	}
	if( sLargest.len != 0 )
	{
		*psRun = sLargest;
		goto found;
	}
	for( i = 0; i < nBmBlocksPerGrp; ++i )
	{
		if( apBlockBuffers[i] != NULL )
		{
			afs_free_block_buffer( psVolume, apBlockBuffers[i] );
			apBlockBuffers[i] = NULL;
		}
	}
	printk( "afs_alloc_blocks() failed to alloc %d blocks\n", nCount );
	return( -ENOSPC );
      found:
//    if( psRun->len != nCount ) {
//      printk( "afs_alloc_blocks() request truncated from %d to %d(%d)\n", nCount, psRun->len, nCount - psRun->len );
//    }
	for( i = 0; i < nBmBlocksPerGrp; ++i )
	{
		if( apBlockBuffers[i] != NULL )
		{
			afs_free_block_buffer( psVolume, apBlockBuffers[i] );
			apBlockBuffers[i] = NULL;
		}
	}
	nError = afs_mark_blocks_used( psVolume, psRun );

/*
  nEndTime = read_pentium_clock();

  printk( "Alloc of %d blocks took %dmS and %d reads\n",
  psRun->len,(int) (nEndTime - nStartTime) / 200000, g_nReads );
  */
	return( nError );
}

/** Determine if a block is free
 * \par Description:
 * Determine if the given block is free on the given volume.  First, read the
 * correct bitmap block. Then, check to see if the given block is marked free.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param nBlock	Block number to check
 * \return true if block is free, false if block is used or on error.
 * \sa
 *****************************************************************************/
bool afs_is_block_free( AfsVolume_s * psVolume, off_t nBlock )
{
	uint32 *panBlock;
	int nGroup = nBlock / psVolume->av_psSuperBlock->as_nBlockPerGroup;
	int nStart = nBlock % psVolume->av_psSuperBlock->as_nBlockPerGroup;
	bool bFree;

	panBlock = afs_alloc_block_buffer( psVolume );
	if( panBlock == NULL )
	{
		printk( "Error: afs_is_block_free() no memory for bitmap block\n" );
		return( false );
	}

	if( afs_logged_read( psVolume, panBlock, psVolume->av_nBitmapStart + nGroup ) < 0 )
	{
		printk( "Panic: afs_is_block_free() failed to load bitmap block\n" );
		return( false );
	}

	if( panBlock[nStart / 32] & ( 1 << ( nStart % 32 ) ) )
	{
		bFree = false;
	}
	else
	{
		bFree = true;
	}
	afs_free_block_buffer( psVolume, panBlock );

	return( bFree );
}
