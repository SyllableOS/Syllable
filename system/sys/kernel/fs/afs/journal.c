
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

#include <posix/unistd.h>
#include <posix/errno.h>
#include <macros.h>

#include <atheos/string.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>
#include <atheos/areas.h>

#include "afs.h"


#define HT_DEFAULT_SIZE   128
#define HASH(b)  (b)

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_init_hash_table( AfsHashTable_s * psTable )
{
	psTable->ht_nSize = HT_DEFAULT_SIZE;
	psTable->ht_nMask = psTable->ht_nSize - 1;
	psTable->ht_nCount = 0;

	psTable->ht_apsTable = kmalloc( psTable->ht_nSize * sizeof( AfsHashEnt_s * ), MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );

	if( psTable->ht_apsTable == NULL )
	{
		return( -ENOMEM );
	}
	else
	{
		return( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void afs_delete_hash_table( AfsHashTable_s * psTable )
{
	kfree( psTable->ht_apsTable );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_resize_hash_table( AfsHashTable_s * psTable )
{
	int nPrevSize;
	int nNewSize;
	int nNewMask;
	int i;
	int nHash;
	AfsHashEnt_s **pasNewTable;

	if( psTable->ht_nSize & psTable->ht_nMask )
	{
		panic( "bcache:resize_hash_table() Inconsistency between hashtable size %d and mask %d in bcache!\n", psTable->ht_nSize, psTable->ht_nMask );
		return( -EINVAL );
	}

	nPrevSize = psTable->ht_nSize;
	nNewSize = nPrevSize * 2;
	nNewMask = nNewSize - 1;

//  printk( "Resize hash table from %d to %d\n", nPrevSize, nNewSize );

	pasNewTable = kmalloc( nNewSize * sizeof( AfsHashEnt_s * ), MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );

	if( pasNewTable == NULL )
	{
		return( -ENOMEM );
	}

	for( i = 0; i < nPrevSize; ++i )
	{
		AfsHashEnt_s *psEntry;
		AfsHashEnt_s *psNext;

		for( psEntry = psTable->ht_apsTable[i]; NULL != psEntry; psEntry = psNext )
		{
			nHash = psEntry->he_nHashVal & nNewMask;
			psNext = psEntry->he_psNext;

			psEntry->he_psNext = pasNewTable[nHash];
			pasNewTable[nHash] = psEntry;
		}
	}
	kfree( psTable->ht_apsTable );
	psTable->ht_apsTable = pasNewTable;
	psTable->ht_nSize = nNewSize;
	psTable->ht_nMask = nNewMask;

	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_hash_insert( AfsHashTable_s * psTable, off_t nBlockNum, AfsHashEnt_s *psNew )
{
	AfsHashEnt_s *psTmp;
	int nHash;

	nHash = HASH( nBlockNum ) & psTable->ht_nMask;

	for( psTmp = psTable->ht_apsTable[nHash]; psTmp != NULL; psTmp = psTmp->he_psNext )
	{
		if( psTmp->he_nBlockNum == nBlockNum )
		{
			break;
		}
	}

	if( psTmp != NULL && psTmp->he_nBlockNum == nBlockNum )
	{
		panic( "afs_hash_insert() Entry already in the hash table! Block=%Ld\n", nBlockNum );
		return( -EEXIST );
	}

	psNew->he_nBlockNum = nBlockNum;
	psNew->he_nHashVal = HASH( nBlockNum );

	psNew->he_psNext = psTable->ht_apsTable[nHash];
	psTable->ht_apsTable[nHash] = psNew;

	psTable->ht_nCount++;

	if( psTable->ht_nCount >= ( ( psTable->ht_nSize * 3 ) / 4 ) )
	{
		afs_resize_hash_table( psTable );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static AfsHashEnt_s *afs_hash_lookup( AfsHashTable_s * psTable, off_t nBlockNum )
{
	int nHash = HASH( nBlockNum ) & psTable->ht_nMask;
	AfsHashEnt_s *psEnt;

	for( psEnt = psTable->ht_apsTable[nHash]; psEnt != NULL; psEnt = psEnt->he_psNext )
	{
		if( psEnt->he_nBlockNum == nBlockNum )
		{
			break;
		}
	}
	return( psEnt );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static AfsHashEnt_s *afs_hash_delete( AfsHashTable_s * psTable, off_t nBlockNum )
{
	int nHash = HASH( nBlockNum ) & psTable->ht_nMask;
	AfsHashEnt_s *psTmp;
	AfsHashEnt_s *psPrev = NULL;

	for( psTmp = psTable->ht_apsTable[nHash]; NULL != psTmp; psPrev = psTmp, psTmp = psTmp->he_psNext )
	{
		if( psTmp->he_nBlockNum == nBlockNum )
		{
			break;
		}
	}

	if( psTmp == NULL )
	{
		panic( "afs_hash_delete() Entry %d does not exist!\n",( int )nBlockNum );
		return( NULL );
	}

	if( psTable->ht_apsTable[nHash] == psTmp )
	{
		psTable->ht_apsTable[nHash] = psTmp->he_psNext;
	}
	else
	{
		if( NULL != psPrev )
		{
			psPrev->he_psNext = psTmp->he_psNext;
		}
		else
		{
			panic( "afs_hash_delete() invalid hash table!\n" );
		}
	}
	psTable->ht_nCount--;

	if( psTable->ht_nCount < 0 )
	{
		panic( "afs_hash_delete() ht_nCount got negative value %d!!!!!\n", psTable->ht_nCount );
	}
	return( psTmp );
}

static void afs_add_jb_entry( AfsTransaction_s *psTrans, AfsHashEnt_s *psEntry )
{
	if( psEntry->he_psTransaction != NULL )
	{
		panic( "afs_add_jb_entry() entry already belongs to a transaction\n" );
		return;
	}

	if( psTrans->at_psFirstEntry != NULL )
	{
		psTrans->at_psFirstEntry->he_psPrevInTrans = psEntry;
	}
	if( psTrans->at_psLastEntry == NULL )
	{
		psTrans->at_psLastEntry = psEntry;
	}
	psEntry->he_psNextInTrans = psTrans->at_psFirstEntry;
	psEntry->he_psPrevInTrans = NULL;
	psTrans->at_psFirstEntry = psEntry;
	psEntry->he_psTransaction = psTrans;
}

static void afs_remove_jb_entry( AfsTransaction_s *psTrans, AfsHashEnt_s *psEntry )
{
	if( psEntry->he_psNextInTrans != NULL )
	{
		psEntry->he_psNextInTrans->he_psPrevInTrans = psEntry->he_psPrevInTrans;
	}
	if( psEntry->he_psPrevInTrans != NULL )
	{
		psEntry->he_psPrevInTrans->he_psNextInTrans = psEntry->he_psNextInTrans;
	}
	if( psTrans->at_psFirstEntry == psEntry )
	{
		psTrans->at_psFirstEntry = psEntry->he_psNextInTrans;
	}
	if( psTrans->at_psLastEntry == psEntry )
	{
		psTrans->at_psLastEntry = psEntry->he_psPrevInTrans;
	}
	psEntry->he_psNextInTrans = NULL;
	psEntry->he_psPrevInTrans = NULL;
	psEntry->he_psTransaction = NULL;
}



static int afs_write_superblock( AfsVolume_s * psVolume )
{
	int nError = write_pos( psVolume->av_nDevice, psVolume->av_nSuperBlockPos, psVolume->av_psSuperBlock, AFS_SUPERBLOCK_SIZE );

	if( nError < 0 )
	{
		printk( "Error: afs_write_superblock() failed to write superblock : %d\n", nError );
		return( nError );
	}
	else if( nError != AFS_SUPERBLOCK_SIZE )
	{
		printk( "Error: afs_write_superblock() failed to write a full superblock : %d\n", nError );
		return( -EIO );
	}
	else
	{
		return( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Called by the cache system for each logged block that is flushed to
 *	disk.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void afs_block_flushed_callback( off_t nBlockNum, int nNumBlocks, void *pArg )
{
	AfsTransaction_s *psTrans = pArg;

	atomic_dec( &psTrans->at_nPendingLogBlocks );
	if( atomic_read( &psTrans->at_nPendingLogBlocks ) < 0 )
	{
		panic( "afs_block_flushed_callback() at_nPendingLogBlocks == %d\n", atomic_read( &psTrans->at_nPendingLogBlocks ) );
	}
}

static AfsTransBuffer_s *afs_alloc_transaction_buffer( AfsVolume_s * psVolume, AfsTransaction_s *psTrans )
{
	AfsTransBuffer_s *psBuffer;
	uint32 nSize = sizeof( AfsTransBuffer_s ) + 129 * psVolume->av_psSuperBlock->as_nBlockSize;
	area_id hArea;

	nSize =( nSize + PAGE_SIZE - 1 ) & PAGE_MASK;

	psBuffer = NULL;
	hArea = create_area( "afs_trans_buf",( void ** )&psBuffer, nSize, nSize, AREA_ANY_ADDRESS | AREA_FULL_ACCESS | AREA_KERNEL, AREA_FULL_LOCK );

	if( hArea < 0 )
	{
		printk( "Error: afs_alloc_transaction_buffer() failed to create trasaction buffer\n" );
		return( NULL );
	}

	psBuffer->atb_pBuffer =( off_t * )( psBuffer + 1 );
	memset( psBuffer->atb_pBuffer, 0, psVolume->av_psSuperBlock->as_nBlockSize );	// Clear the index block
	psBuffer->atb_hAreaID = hArea;
	psBuffer->atb_nMaxBlockCount = 128;
	psBuffer->atb_nBlockCount = 0;
	psBuffer->atb_psNext = psTrans->at_psFirstBuffer;
	psTrans->at_psFirstBuffer = psBuffer;
	return( psBuffer );
}

static void afs_free_transaction_buffer( AfsVolume_s * psVolume, AfsTransBuffer_s *psBuffer )
{
	delete_area( psBuffer->atb_hAreaID );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_replay_log( AfsVolume_s * psVolume )
{
	AfsSuperBlock_s *psSuperBlock = psVolume->av_psSuperBlock;
	const int nBlockSize = psSuperBlock->as_nBlockSize;
	off_t nLogBlock;
	int nError;
	off_t *pIndex;
	void *pBlockBuffer;

	if( psSuperBlock->as_nValidLogBlocks == 0 )
	{
		psSuperBlock->as_nLogStart = psVolume->av_nLoggStart;
		return( 0 );
	}

	if( psVolume->av_nFlags & FS_IS_READONLY )
	{
		printk( "afs_replay_log() Trying to replay log on read-only FS\n" );
		return( -EROFS );
	}

	printk( "afs_replay_log() %d log blocks from %d\n", psSuperBlock->as_nValidLogBlocks,( int )psSuperBlock->as_nLogStart );

	pIndex = kmalloc( nBlockSize, MEMF_KERNEL );
	if( pIndex == NULL )
	{
		printk( "Error: afs_replay_log() no memory for index block\n" );
		return( -ENOMEM );
	}
	pBlockBuffer = kmalloc( nBlockSize, MEMF_KERNEL );
	if( pBlockBuffer == NULL )
	{
		printk( "Error: afs_replay_log() no memory for block buffer\n" );
		kfree( pIndex );
		return( -ENOMEM );
	}

	nLogBlock = psSuperBlock->as_nLogStart;

	while( psSuperBlock->as_nValidLogBlocks )
	{
		int i;

		nError = read_phys_blocks( psVolume->av_nDevice, nLogBlock, pIndex, 1, nBlockSize );
		if( nError < 0 )
		{
			printk( "Error: afs_replay_log() failed to read index block\n" );
			goto error;
		}
		nLogBlock++;
		psSuperBlock->as_nValidLogBlocks--;
		if( nLogBlock > psVolume->av_nLoggEnd )
		{
			nLogBlock = psVolume->av_nLoggStart;
		}
		for( i = 0; i < 128; i++ )
		{
			if( pIndex[i] == 0 )
			{
				break;
			}
			printk( "afs_replay_log() write back block %d to %d\n",( int )nLogBlock, ( int )pIndex[i] );

			if( pIndex[i] >= psSuperBlock->as_nNumBlocks )
			{
				printk( "Panic: attempt to write back block belonging outside the disk!!\n" );
				goto error;
			}
			nError = read_phys_blocks( psVolume->av_nDevice, nLogBlock++, pBlockBuffer, 1, nBlockSize );
			psSuperBlock->as_nValidLogBlocks--;
			if( nError < 0 )
			{
				printk( "Panic: afs_replay_log() failed to read journal block\n" );
				goto error;
			}
			nError = write_phys_blocks( psVolume->av_nDevice, pIndex[i], pBlockBuffer, 1, nBlockSize );
			if( nError < 0 )
			{
				printk( "Panic: afs_replay_log() failed to write back journal block\n" );
				goto error;
			}
			if( nLogBlock > psVolume->av_nLoggEnd )
			{
				nLogBlock = psVolume->av_nLoggStart;
			}
		}
	}
	psSuperBlock->as_nLogStart = psVolume->av_nLoggStart;
	psSuperBlock->as_nValidLogBlocks = 0;

	afs_write_superblock( psVolume );
//  write_phys_blocks( psVolume->av_nDevice, 0, psVolume->av_psSuperBlock, 1, nBlockSize );

	kfree( pBlockBuffer );
	kfree( pIndex );
	return( 0 );
      error:
	kfree( pBlockBuffer );
	kfree( pIndex );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Allocate and initialize a new transaction header.
 *	The at_nLogStart is set to the first block after the previous
 *	transaction, or the first block in the log if the transaction
 *	list are empty.
 *	The new header is linked into the doubly linked list of transactions
 *	in the volume structure.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_create_transaction( AfsVolume_s * psVolume )
{
	AfsTransaction_s *psPrevTrans = psVolume->av_psFirstTrans;
	AfsTransaction_s *psTrans;

	kassertw( psPrevTrans == NULL || psPrevTrans->at_bWrittenToLog );

	psTrans = kmalloc( sizeof( AfsTransaction_s ), MEMF_KERNEL | MEMF_CLEAR );
	if( psTrans == NULL )
	{
		printk( "Error: afs_create_transaction() no memory for transaction header\n" );
		return( -ENOMEM );
	}
	// Calculate the starting point in the journal of the new transaction
	if( psPrevTrans != NULL )
	{
		off_t nPrevEnd = psPrevTrans->at_nLogStart + psPrevTrans->at_nBlockCount;

		if( nPrevEnd <= psVolume->av_nLoggEnd )
		{
			psTrans->at_nLogStart = nPrevEnd;
		}
		else
		{
			psTrans->at_nLogStart = psVolume->av_nLoggStart +( nPrevEnd - psVolume->av_nLoggEnd ) - 1;
		}
	}
	else
	{
		psTrans->at_nLogStart = psVolume->av_nLoggStart;
	}
	psTrans->at_nBlockCount = 0;

//  printk( "afs_create_transaction() new transaction start at %d\n",(int) psTrans->at_nLogStart );

	// Link the new transaction into the volumes list.
	if( psVolume->av_psLastTrans == NULL )
	{
		psVolume->av_psLastTrans = psTrans;
	}
	if( psVolume->av_psFirstTrans != NULL )
	{
		psVolume->av_psFirstTrans->at_psPrev = psTrans;
	}
	psTrans->at_psNext = psVolume->av_psFirstTrans;
	psTrans->at_psPrev = NULL;
	psVolume->av_psFirstTrans = psTrans;
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_begin_transaction( AfsVolume_s * psVolume )
{
	int nError = 0;

	LOCK( psVolume->av_hJournalLock );

//    printk( "Begin transaction\n" );
	if( psVolume->av_nTransactionStarted != 0 )
	{
		panic( "attempt to nest calls to afs_begin_transaction()\n" );
		return( -EINVAL );
	}
	if( psVolume->as_nTransAllocatedBlocks != 0 )
	{
		panic( "afs_begin_transaction() %Ld blocks allocated without a transaction\n", psVolume->as_nTransAllocatedBlocks );
		return( -EINVAL );
	}
	psVolume->av_nTransactionStarted++;

	LOCK( psVolume->av_hJournalListsLock );
	if( psVolume->av_psFirstTrans == NULL || psVolume->av_psFirstTrans->at_bWrittenToLog )
	{
		nError = afs_create_transaction( psVolume );
		if( nError < 0 )
		{
			psVolume->av_nTransactionStarted--;
		}
	}
	UNLOCK( psVolume->av_hJournalListsLock );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Write all blocks to their final destination on the disk.
 *	For each block we writes we increase the at_nPendingLogBlocks
 *	counter, and register a callback with the cache block, that
 *	will be called when the buffer is safely written to disk.
 *	The callback will decrease the counter each time a block
 *	is flushed, and when it gets to zero we knows that the entire
 *	transaction is really written to disk, and the journal space
 *	occupied by this transaction can be reclaimed.
 *
 * NOTE:
 *	No checks are done to verify that the previous transaction is
 *	finished or that the current really was written to disk.
 *	If you don't check this before calling it you will jepordize 
 *	the poor users hard earned data.
 *
 *	Also note that this function fetch a block with get_empty_block()
 *	and that if somebody read that block before we write to it, they
 *	will get a cache hit on the empty block, and read invalid data.
 *	Therefore it is very important to avoid other threads accessing
 *	the cache block until it is gracefully updated. This is achived
 *	by locking the psVolume->av_hJournalListsLock mutex both while
 *	calling this function, and when reading meta data from the
 *	cache.
 * SEE ALSO:
 ****************************************************************************/

static int afs_write_transaction_to_disk( AfsVolume_s * psVolume, AfsTransaction_s *psTrans )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsTransBuffer_s *psBuf;
	AfsHashEnt_s *psEntry;
	int nIndex = 0;

	for( psBuf = psTrans->at_psFirstBuffer; psBuf != NULL; psBuf = psBuf->atb_psNext )
	{
		int i;

		for( i = 0; i < psBuf->atb_nBlockCount; ++i )
		{
			off_t nBlock = psBuf->atb_pBuffer[i];
			void *pLogBuffer =( ( uint8 * )psBuf->atb_pBuffer ) + ( i + 1 ) * nBlockSize;
			int nError;

			if( nIndex++ < atomic_read( &psTrans->at_nWrittenBlocks ) )
			{
				printk( "afs_write_transaction_to_disk() block %d already written. Skipping\n", nIndex );
				continue;
			}
			nError = write_logged_blocks( psVolume->av_nDevice, nBlock, pLogBuffer, 1, nBlockSize, afs_block_flushed_callback, psTrans );
			if( nError < 0 )
			{
				// FIXME: We should try a few times and then mark the FS read-only and give up.
				printk( "Error: afs_write_transaction_to_disk() failed to write block(err=%d) will try again later\n", nError );
				return( nError );
			}

/*	    psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, nBlock );
	    if( psEntry != NULL && psEntry->he_pBuffer == pLogBuffer ) {
		if( afs_hash_delete( &psVolume->av_sLogHashTable, nBlock ) != psEntry ) {
		    panic( "afs_write_transaction() failed to delete entry from hash-table\n" );
		}
		kfree( psEntry );
	    }*/
			atomic_inc( &psTrans->at_nPendingLogBlocks );
			atomic_inc( &psTrans->at_nWrittenBlocks );
		}
	}

	nIndex = 0;
	while( psTrans->at_psFirstEntry != NULL )
	{
		psEntry = psTrans->at_psFirstEntry;
		psTrans->at_psFirstEntry = psEntry->he_psNextInTrans;

		if( psEntry->he_psTransBuf == NULL )
		{
//          printk( "afs_write_transaction_to_disk() remove block %Ld from hash table(%p)\n", psEntry->he_nBlockNum, psTrans );
			if( afs_hash_delete( &psVolume->av_sLogHashTable, psEntry->he_nBlockNum ) != psEntry )
			{
				panic( "afs_write_transaction_to_disk() failed to delete entry from hash-table\n" );
			}
			kfree( psEntry );
		}
		else
		{
//          printk( "afs_write_transaction_to_disk() block %Ld to current. Not from hash table(%p)\n", psEntry->he_nBlockNum, psTrans );
			psEntry->he_psTransaction = NULL;
		}
		nIndex++;
	}
//    if( nIndex > 0 ) {
//      printk( "afs_write_transaction_to_disk() removed %d hash-table entries\n", nIndex );
//    }
	psTrans->at_psLastEntry = NULL;
	psTrans->at_bWrittenToDisk = true;
	return( 0 );
}

static void afs_force_flush_transaction( AfsVolume_s * psVolume, AfsTransaction_s *psTrans )
{
	AfsTransBuffer_s *psBuf;

	for( psBuf = psTrans->at_psFirstBuffer; psBuf != NULL; psBuf = psBuf->atb_psNext )
	{
		int i;

		for( i = 0; i < psBuf->atb_nBlockCount; ++i )
		{
			flush_cache_block( psVolume->av_nDevice, psBuf->atb_pBuffer[i] );
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Checks if the oldest transaction has been safely flushed to disk, and
 *	if so deletes it. The av_nPendingLogBlocks in the volume structure
 *	and the super block is updated to reflect the new range of valid
 *	journal blocks.
 *	If there is more transaction written to the log, but not yet to their
 *	final destination, the oldest one are written to the cahche now.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_discard_flushed_journal_entries( AfsVolume_s * psVolume )
{
	AfsTransaction_s *psTrans;

	psTrans = psVolume->av_psLastTrans;
	if( psTrans == NULL )
	{
		return( 0 );
	}
	if( psTrans->at_bWrittenToLog == false || psTrans->at_bWrittenToDisk == false )
	{
		return( 0 );
	}
	if( atomic_read( &psTrans->at_nPendingLogBlocks ) > 0 )
	{
		return( 0 );
	}
	// Unlink the finished transaction.
	psVolume->av_psLastTrans = psTrans->at_psPrev;
	if( psTrans->at_psPrev != NULL )
	{
		psTrans->at_psPrev->at_psNext = NULL;
	}
	else
	{
		psVolume->av_psFirstTrans = NULL;
	}
	psVolume->av_nPendingLogBlocks -= psTrans->at_nBlockCount;
	kassertw( psVolume->av_nPendingLogBlocks >= 0 );

	// Update the super-block.

	psVolume->av_psSuperBlock->as_nValidLogBlocks -= psTrans->at_nBlockCount;
	kassertw( psVolume->av_psSuperBlock->as_nValidLogBlocks >= 0 );

	if( psVolume->av_psLastTrans == NULL )
	{
		if( psVolume->av_psSuperBlock->as_nValidLogBlocks != 0 )
		{
			panic( "afs_discard_flushed_journal_entries(): No more transactions but %d blocks are pending\n", psVolume->av_psSuperBlock->as_nValidLogBlocks );
			return( -EINVAL );
		}
		psVolume->av_psSuperBlock->as_nLogStart = psVolume->av_nLoggStart;
	}
	else
	{
		kassertw( psVolume->av_psLastTrans->at_bWrittenToDisk == false );
		psVolume->av_psSuperBlock->as_nLogStart = psVolume->av_psLastTrans->at_nLogStart;
	}
//  printk( "afs_discard_flushed_journal_entries() New journal range = %d(%d)\n",
//       (int)psVolume->av_psSuperBlock->as_nLogStart, (int)psVolume->av_psSuperBlock->as_nValidLogBlocks );

	while( psTrans->at_psFirstBuffer != NULL )
	{
		AfsTransBuffer_s *psBuf = psTrans->at_psFirstBuffer;

		psTrans->at_psFirstBuffer = psBuf->atb_psNext;
		afs_free_transaction_buffer( psVolume, psBuf );
	}
	kfree( psTrans );

	afs_write_superblock( psVolume );

	// If the now last transaction is written to log we write it to the disk now.
	psTrans = psVolume->av_psLastTrans;
	if( psTrans != NULL && psTrans->at_bWrittenToLog )
	{
		afs_write_transaction_to_disk( psVolume, psTrans );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Makes room for nBlockCount number of blocks in the log by forcing
 *	cache flushes, and terminate the transactions that are finished
 *	because they are written to the disk.
 * NOTE:
 *	If nBlockCount are larger than the entire log this function will
 *	never finish.
 * SEE ALSO:
 ****************************************************************************/

static int afs_wait_for_log_space( AfsVolume_s * psVolume, int nBlockCount )
{
	int nLoops = 0;

	while( psVolume->av_nPendingLogBlocks + nBlockCount > psVolume->av_nJournalSize )
	{
		if( nLoops++ > 500 )
		{
			printk( "Wait for %d log blocks to be free(%d)\n", nBlockCount, psVolume->av_nPendingLogBlocks );
		}
		if( psVolume->av_psLastTrans != NULL )
		{
			afs_force_flush_transaction( psVolume, psVolume->av_psLastTrans );
		}
//          flush_device_cache( psVolume->av_nDevice, true );
		if( psVolume->av_psLastTrans != NULL && psVolume->av_psLastTrans->at_bWrittenToLog && psVolume->av_psLastTrans->at_bWrittenToDisk == false )
		{
			afs_write_transaction_to_disk( psVolume, psVolume->av_psLastTrans );
		}
		afs_discard_flushed_journal_entries( psVolume );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	First the transaction is written to the journal. Then the super-block
 *	is updated to contain the new valid journal area. 
 * NOTE:
 *	The logg are written directly to the disk, not through the cache.
 * SEE ALSO:
 ****************************************************************************/

static int afs_write_transaction_to_log( AfsVolume_s * psVolume )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsTransaction_s *psTrans = psVolume->av_psFirstTrans;
	AfsTransBuffer_s *psBuf;
	off_t nBlockAddr;
	int nError = 0;
	int nBlocksToWrite = psVolume->av_nOldBlockCount;
	int nTotalBlockCount = 0;

	if( psTrans == NULL || psTrans->at_bWrittenToLog == true )
	{
		return( 0 );
	}
	if( psTrans->at_nLogStart < psVolume->av_nLoggStart )
	{
		panic( "afs_write_transaction_to_log() First block start before log-area. Log: %Ld->%Ld, Log-start: %Ld\n", psVolume->av_nLoggStart, psVolume->av_nLoggEnd, psTrans->at_nLogStart );
	}
	if( psTrans->at_nLogStart > psVolume->av_nLoggEnd )
	{
		panic( "afs_write_transaction_to_log() First block start after log-area. Log: %Ld->%Ld, Log-start: %Ld\n", psVolume->av_nLoggStart, psVolume->av_nLoggEnd, psTrans->at_nLogStart );
	}

	nBlockAddr = psTrans->at_nLogStart;
//  printk( "Write transaction from %d(%d blocks)\n", (int)psTrans->at_nLogStart, psTrans->at_nBlockCount );
	for( psBuf = psTrans->at_psFirstBuffer; psBuf != NULL; psBuf = psBuf->atb_psNext )
	{
		int nBlockCount = psBuf->atb_nBlockCount + 1;	// Data blocks + index block;

		if( nBlockAddr + nBlockCount <= psVolume->av_nLoggEnd + 1 )
		{
			nError = write_phys_blocks( psVolume->av_nDevice, nBlockAddr, psBuf->atb_pBuffer, nBlockCount, nBlockSize );
//      printk( "Write one run %d-%d\n",(int)nBlockAddr, (int)(nBlockAddr + nBlockCount - 1) );
			if( nError < 0 )
			{
				printk( "Error: afs_write_transaction_to_log() failed to write unsplitted run\n" );
				printk( "Error: %Ld -> %Ld(%d) Error = %d\n", nBlockAddr, nBlockAddr + nBlockCount * nBlockSize - 1, nBlockSize, nError );
				goto error;
			}
			nBlockAddr += nBlockCount;
		}
		else
		{
			int nFirstSize = psVolume->av_nLoggEnd - nBlockAddr + 1;
			int nSecondSize = nBlockCount - nFirstSize;

			if( nFirstSize <= 0 || nSecondSize <= 0 )
			{
				panic( "afs_write_transaction_to_log() got negative block-run. Log: %Ld->%Ld, Trans: %Ld,%d, Sizes: %d,%d\n", psVolume->av_nLoggStart, psVolume->av_nLoggEnd, psTrans->at_nLogStart, psTrans->at_nBlockCount, nFirstSize, nSecondSize );
			}

			nError = write_phys_blocks( psVolume->av_nDevice, nBlockAddr, psBuf->atb_pBuffer, nFirstSize, nBlockSize );
			if( nError < 0 )
			{
				printk( "Error: afs_write_transaction_to_log() failed to write first run of splitted write\n" );
				printk( "Error: %Ld -> %Ld(%d) Error = %d\n", nBlockAddr, nBlockAddr + nFirstSize * nBlockSize - 1, nBlockSize, nError );
				goto error;
			}
			nBlockAddr = psVolume->av_nLoggStart;
			nError = write_phys_blocks( psVolume->av_nDevice, nBlockAddr,( ( uint8 * )psBuf->atb_pBuffer ) + ( nFirstSize * nBlockSize ), nSecondSize, nBlockSize );
			if( nError < 0 )
			{
				printk( "Error: afs_write_transaction_to_log() failed to write second run of splitted write\n" );
				printk( "Error: %Ld -> %Ld(%d) Error = %d\n", psVolume->av_nLoggStart, psVolume->av_nLoggStart + ( nBlockCount - nFirstSize ) * nBlockSize - 1, nBlockSize, nError );
				goto error;
			}
			nBlockAddr += nSecondSize;
		}
		nTotalBlockCount += nBlockCount;
	}
	psVolume->av_nOldBlockCount -= nTotalBlockCount;
	psTrans->at_bWrittenToLog = true;

	if( psVolume->av_nOldBlockCount != 0 )
	{
		panic( "afs_write_transaction_to_log() mismatching block count! av_nOldBlockCount = %d\n", psVolume->av_nOldBlockCount );
	}

	// Update the range in super-block
	psVolume->av_psSuperBlock->as_nValidLogBlocks += nBlocksToWrite;
//  printk( "afs_write_transaction_to_log() New journal range = %d(%d)\n",
//       (int)psVolume->av_psSuperBlock->as_nLogStart, psVolume->av_psSuperBlock->as_nValidLogBlocks );

	afs_write_superblock( psVolume );

	if( psVolume->av_psLastTrans != NULL && psVolume->av_psLastTrans->at_bWrittenToLog && psVolume->av_psLastTrans->at_bWrittenToDisk == false )
	{
		afs_write_transaction_to_disk( psVolume, psVolume->av_psLastTrans );
	}
	return( 0 );
//    return( afs_create_transaction( psVolume ) );
      error:
	return( nError );
}

static void afs_validate_transaction( AfsVolume_s * psVolume, AfsTransaction_s *psTrans )
{
	AfsTransBuffer_s *psBuf;
	int nBlockCount = 0;

	for( psBuf = psTrans->at_psFirstBuffer; psBuf != NULL; psBuf = psBuf->atb_psNext )
	{
		int i;

		nBlockCount += psBuf->atb_nBlockCount + 1;
		for( i = 0; i < psBuf->atb_nBlockCount; ++i )
		{
			if( psBuf->atb_pBuffer[i] == 0 )
			{
				panic( "afs_validate_transaction() transaction buffer have 0 reference at index %d\n", i );
			}
		}
		for( ; i < 128; ++i )
		{
			if( psBuf->atb_pBuffer[i] != 0 )
			{
				panic( "afs_validate_transaction() transaction buffer have non-0 reference at index %d\n", i );
			}
		}
	}
	if( nBlockCount != psTrans->at_nBlockCount )
	{
		panic( "afs_validate_transaction() mismatch between at_nBlockCount(%d) and actual block count (%d)\n", psTrans->at_nBlockCount, nBlockCount );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Merge the blocks touched during the previous transaction
 *	(recorded in the volume struct) with the current "super" transaction.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_merge_transactions( AfsVolume_s * psVolume )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsTransaction_s *psTrans = psVolume->av_psFirstTrans;
	AfsJournalBlock_s *psBlock;
	int nNewBlocks = 0;
	int nError = 0;
	int nNew = 0;
	int nMerged = 0;

	if( psTrans == NULL )
	{
		panic( "afs_merge_transactions() something went terribly wrong! psTrans ended up as a NULL pointer\n" );
		return( -EINVAL );
	}
	if( psTrans->at_bWrittenToLog )
	{
		printk( "afs_merge_transactions() the transaction is already written to the log\n" );
		return( -EINVAL );
	}
	afs_wait_for_log_space( psVolume, psVolume->av_nNewBlockCount + psVolume->av_nJournalSize / 128 );

	// Here we merge the blocks touched during the current transaction with blocks
	// from previous transactions.
	for( psBlock = psVolume->av_psFirstLogBlock; psBlock != NULL; psBlock = psBlock->jb_psNext )
	{
		AfsTransBuffer_s *psBuf;
		AfsHashEnt_s *psEntry;

		psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, psBlock->jb_nBlock );
		if( psEntry == NULL )
		{
			panic( "afs_merge_transactions:1() can't find block %Ld in hash-table\n", psBlock->jb_nBlock );
			return( -EINVAL );
		}
		if( psEntry->he_psTransBuf != psBlock )
		{
			panic( "afs_merge_transactions() hash-table entry does not point to most resent block!\n" );
			return( -EINVAL );
		}
		if( psEntry->he_psTransaction != psTrans || psEntry->he_pBuffer == NULL )
		{
			// If new or not our's we must alloc a new block
			if( psTrans->at_psFirstBuffer == NULL || psTrans->at_psFirstBuffer->atb_nBlockCount == psTrans->at_psFirstBuffer->atb_nMaxBlockCount )
			{
				psBuf = afs_alloc_transaction_buffer( psVolume, psTrans );
				if( psBuf == NULL )
				{
					printk( "Error: afs_merge_transactions() no memory for transaction buffer\n" );
					nError = -ENOMEM;
					goto error;
				}
				psTrans->at_nBlockCount++;	// Need space for the index block
				psVolume->av_nOldBlockCount++;
				psVolume->av_nPendingLogBlocks++;
			}
			else
			{
				psBuf = psTrans->at_psFirstBuffer;
			}
			// Think twice before re-ordering the next few lines(then leave it as it is).
			if( psBlock->jb_nBlock == 0 )
			{
				panic( "afs_merge_transactions() block refere to disk-address 0!\n" );
			}
			psBuf->atb_pBuffer[psBuf->atb_nBlockCount] = psBlock->jb_nBlock;
			psBuf->atb_nBlockCount++;
			psBlock->jb_pNewBuffer =( ( uint8 * )psBuf->atb_pBuffer ) + ( psBuf->atb_nBlockCount ) * nBlockSize;
			nNewBlocks++;
		}
	}

	while( psVolume->av_psFirstLogBlock != NULL )
	{
		AfsHashEnt_s *psEntry;

		psBlock = psVolume->av_psFirstLogBlock;
		psVolume->av_psFirstLogBlock = psBlock->jb_psNext;

		psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, psBlock->jb_nBlock );
		if( psEntry == NULL )
		{
			panic( "afs_merge_transactions:2() can't find block in hash-table\n" );
			return( -EINVAL );
		}
		if( psEntry->he_psTransBuf != psBlock )
		{
			panic( "afs_merge_transactions() hash-table entry does not point to most resent block!\n" );
			return( -EINVAL );
		}
		if( psEntry->he_psTransaction != psTrans || psEntry->he_pBuffer == NULL )
		{
			nNew++;
			memcpy( psBlock->jb_pNewBuffer, psBlock->jb_pData, nBlockSize );
			psEntry->he_pBuffer = psBlock->jb_pNewBuffer;
			if( psEntry->he_psTransaction != psTrans )
			{
				if( psEntry->he_psTransaction != NULL )
				{
					afs_remove_jb_entry( psEntry->he_psTransaction, psEntry );
				}
				afs_add_jb_entry( psTrans, psEntry );
			}
		}
		else
		{
			nMerged++;
			memcpy( psEntry->he_pBuffer, psBlock->jb_pData, nBlockSize );
			psVolume->av_nNewBlockCount--;
		}
		psEntry->he_psTransBuf = NULL;
		kfree( psBlock );
	}
//    printk( "Merge transaction %d new and %d merged\n", nNew, nMerged );

	psTrans->at_nBlockCount += nNewBlocks;
	psVolume->av_nOldBlockCount += nNewBlocks;
	psVolume->av_nPendingLogBlocks += nNewBlocks;
	psVolume->av_nNewBlockCount -= nNewBlocks;
	if( psVolume->av_nNewBlockCount != 0 )
	{
		panic( "afs_merge_transactions() mismatching block count! av_nNewBlockCount = %d\n", psVolume->av_nNewBlockCount );
	}
	afs_validate_transaction( psVolume, psTrans );
	return( 0 );
      error:
	while( nNewBlocks > 0 )
	{
		if( psTrans->at_psFirstBuffer == NULL )
		{
			panic( "afs_merge_transactions() to few transaction buffers. %d blocks left\n", nNewBlocks );
		}
		if( psTrans->at_psFirstBuffer->atb_nBlockCount <= nNewBlocks )
		{
			AfsTransBuffer_s *psNext = psTrans->at_psFirstBuffer->atb_psNext;

			nNewBlocks -= psTrans->at_psFirstBuffer->atb_nBlockCount;

			printk( "afs_merge_transactions() delete buffer with %d blocks\n", psTrans->at_psFirstBuffer->atb_nBlockCount );
			afs_free_transaction_buffer( psVolume, psTrans->at_psFirstBuffer );
			psTrans->at_psFirstBuffer = psNext;

			psTrans->at_nBlockCount--;
			psVolume->av_nOldBlockCount--;
			psVolume->av_nPendingLogBlocks--;
		}
		else
		{
			printk( "afs_merge_transactions() strip %d blocks from buffer\n", nNewBlocks );
			psTrans->at_psFirstBuffer->atb_nBlockCount -= nNewBlocks;
			memset( psTrans->at_psFirstBuffer->atb_pBuffer + psTrans->at_psFirstBuffer->atb_nBlockCount, 0, nNewBlocks * sizeof( off_t ) );
			nNewBlocks = 0;
		}
	}
	afs_validate_transaction( psVolume, psTrans );
	return( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	Discard the blocks touched during the previous transaction
 *	(recorded in the volume struct). This is used to "baild out"
 *	when something fail half-way through a transaction.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_discard_transactions( AfsVolume_s * psVolume )
{
	int nNumReverted = 0;
	int nNumDiscarded = 0;
	int i;

	while( psVolume->av_psFirstLogBlock != NULL )
	{
		AfsJournalBlock_s *psBlock = psVolume->av_psFirstLogBlock;
		AfsHashEnt_s *psEntry;

		psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, psBlock->jb_nBlock );
		if( psEntry == NULL )
		{
			panic( "afs_discard_transactions() can't find block in hash-table\n" );
			return( -EINVAL );
		}
		if( psEntry->he_psTransBuf != psBlock )
		{
			panic( "afs_discard_transactions() hash-table entry does not point to most resent block!\n" );
			return( -EINVAL );
		}
		if( psEntry->he_pBuffer == NULL )
		{
			// It don't belong to any older transactions either so we nukes it.
//          printk( "afs_discard_transactions() remove block %Ld from hash table(%p)\n", psBlock->jb_nBlock, psEntry->he_psTransaction );
			if( afs_hash_delete( &psVolume->av_sLogHashTable, psBlock->jb_nBlock ) != psEntry )
			{
				panic( "afs_discard_transactions() failed to delete entry from hash-table\n" );
			}
			if( psEntry->he_psTransaction != NULL )
			{
				afs_remove_jb_entry( psEntry->he_psTransaction, psEntry );
			}
			kfree( psEntry );
			nNumDiscarded++;
		}
		else
		{
			nNumReverted++;
		}
		psEntry->he_psTransBuf = NULL;

		psVolume->av_psFirstLogBlock = psBlock->jb_psNext;
		kfree( psBlock );
		psVolume->av_nNewBlockCount--;
	}
	if( psVolume->av_nNewBlockCount != 0 )
	{
		panic( "afs_discard_transactions() mismatching block count! av_nNewBlockCount = %d\n", psVolume->av_nNewBlockCount );
	}
//    printk( "afs_discard_transactions() reverted %d blocks and discarded %d blocks. Reverted %Ld allocated blocks\n",
//          nNumReverted, nNumDiscarded, psVolume->as_nTransAllocatedBlocks );
	psVolume->av_psSuperBlock->as_nUsedBlocks -= psVolume->as_nTransAllocatedBlocks;
	psVolume->as_nTransAllocatedBlocks = 0;

	for( i = 0; i < psVolume->av_sLogHashTable.ht_nCount; ++i )
	{
		AfsHashEnt_s *psEntry;

		for( psEntry = psVolume->av_sLogHashTable.ht_apsTable[i]; psEntry != NULL; psEntry = psEntry->he_psNext )
		{
			if( psEntry->he_psTransBuf != NULL )
			{
				panic( "afs_discard_transactions() hashtable entry still points to block header\n" );
			}
			if( psEntry->he_pBuffer == NULL )
			{
				panic( "afs_discard_transactions() hashtable entry have no buffer\n" );
			}
		}
	}

	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Called when new blocks are allocated for files. This function will
 *	flush out compleate journal entries until the given block is no
 *	longer in the journal.
 *
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_wait_for_block_to_leave_log( AfsVolume_s * psVolume, off_t nBlock )
{
	int nRun = 0;
	int nError = 0;

	while( afs_hash_lookup( &psVolume->av_sLogHashTable, nBlock ) != NULL )
	{
		printk( "Wait for block %Ld to leave the log\n", nBlock );
		if( nRun == 2 )
		{
			nError = afs_write_transaction_to_log( psVolume );
			if( nError >= 0 )
			{
				nError = afs_create_transaction( psVolume );
			}
		}
		if( psVolume->av_psLastTrans != NULL )
		{
			afs_force_flush_transaction( psVolume, psVolume->av_psLastTrans );
		}
//    flush_device_cache( psVolume->av_nDevice, true );
		if( psVolume->av_psLastTrans != NULL && psVolume->av_psLastTrans->at_bWrittenToLog && psVolume->av_psLastTrans->at_bWrittenToDisk == false )
		{
			afs_write_transaction_to_disk( psVolume, psVolume->av_psLastTrans );
		}
		afs_discard_flushed_journal_entries( psVolume );
		nRun++;
//    if( nRun > 500 ) {
//      printk( "Panic: Block %d won't leave the log\n",(int)nBlock );
//      break;
//    }
	}
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	This function is called by the file-system to tell that it is now safe
 *	to split the transaction to avoid journal overflow.
 *	This can be necessary during operations that might touch more blocks
 *	than can possibly fit in the journal. It breaks the atomicity of the
 *	operation, but is only performed at times where a half finished
 *	operation won't threathen the integrity of the file-system.
 *	One example where this can happen is when deleting a file with lot's
 *	of attributes. If the entire deletion would be performed in one
 *	transaction, the blocks touched by both the file, and all its
 *	attributes might exceed the size of the log. By allowing the journaling
 *	system to start a new transaction between every attribute, we avoid
 *	overflowing the log, but risk that a crash will leave the file with
 *	half it's attributes deleted. This is not good, but far better than
 *	jepordizing the entire file-system by overflowing the log.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_checkpoint_journal( AfsVolume_s * psVolume )
{
	status_t nError = 0;

//    printk( "Checkpoint journal\n" );

	LOCK( psVolume->av_hJournalListsLock );
	if( psVolume->av_nOldBlockCount + psVolume->av_nNewBlockCount + psVolume->av_nJournalSize / 128 > psVolume->av_nJournalSize )
	{
		nError = afs_write_transaction_to_log( psVolume );
		if( nError >= 0 )
		{
			nError = afs_create_transaction( psVolume );
		}
	}
	if( nError >= 0 )
	{
//      printk( "afs_create_transaction() merge transaction\n" );
		nError = afs_merge_transactions( psVolume );
		if( nError >= 0 )
		{
			afs_discard_flushed_journal_entries( psVolume );
		}
	}
	if( nError >= 0 )
	{
		psVolume->as_nTransAllocatedBlocks = 0;
	}
	UNLOCK( psVolume->av_hJournalListsLock );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_end_transaction( AfsVolume_s * psVolume, bool bSubmitChanges )
{
	status_t nError = 0;

//    off_t nUsedBlocks;

//    printk( "End transaction\n" );
	LOCK( psVolume->av_hJournalListsLock );
	if( psVolume->av_nOldBlockCount + psVolume->av_nNewBlockCount > 128 )
	{
		nError = afs_write_transaction_to_log( psVolume );
		if( nError >= 0 )
		{
			nError = afs_create_transaction( psVolume );
		}
	}
	if( nError >= 0 && bSubmitChanges )
	{
		nError = afs_merge_transactions( psVolume );
		if( nError < 0 )
		{
			printk( "Error: afs_end_transaction() failed to merge transaction: %d\n", nError );
			afs_discard_transactions( psVolume );
		}
		else
		{
			psVolume->as_nTransAllocatedBlocks = 0;
		}
	}
	else
	{
		afs_discard_transactions( psVolume );
	}
	afs_discard_flushed_journal_entries( psVolume );
	UNLOCK( psVolume->av_hJournalListsLock );

	psVolume->av_nTransactionStarted--;
	if( psVolume->av_nTransactionStarted < 0 )
	{
		panic( "unbalanced call to afs_end_transaction()\n" );
	}
	psVolume->av_nLastJournalAccesTime = get_system_time();

/*
    nUsedBlocks = afs_count_used_blocks( psVolume );

    if( psVolume->av_psSuperBlock->as_nUsedBlocks != nUsedBlocks ) {
	printk( "Error: afs_end_transaction() Inconsistency between super-block and allocation bitmap!\n" );
	printk( "Superblock: %Ld used blocks\n", psVolume->av_psSuperBlock->as_nUsedBlocks );
	printk( "Bitmap:     %Ld used blocks\n", nUsedBlocks );
//	psVolume->av_psSuperBlock->as_nUsedBlocks = nUsedBlocks;
    }
    */
	UNLOCK( psVolume->av_hJournalLock );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Periodically finish off half filled journal entries so they won't
 *	stay unwritten forever if all other disk access stops.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static uint32 afs_journal_flusher( void *pData )
{
	AfsVolume_s *volatile psVolume = pData;

	while( psVolume->av_bRunJournalFlusher )
	{
		snooze( 1000000 );

		LOCK( psVolume->av_hJournalLock );
		LOCK( psVolume->av_hJournalListsLock );

//      if( psVolume->av_nOldBlockCount + psVolume->av_nNewBlockCount > 128 ) {
//          afs_write_transaction_to_log( psVolume );
//      }
		if( psVolume->av_nNewBlockCount != 0 )
		{
			panic( "afs_journal_flusher() found a half-finished transaction in the journal!\n" );
			continue;
//          afs_merge_transactions( psVolume );
		}
//      if( psVolume->av_nOldBlockCount != 0 ) {
		afs_write_transaction_to_log( psVolume );
//      }
		afs_discard_flushed_journal_entries( psVolume );
		UNLOCK( psVolume->av_hJournalListsLock );
		UNLOCK( psVolume->av_hJournalLock );
	}
	psVolume->av_bRunJournalFlusher = true;	// Handshake with unmount() to tell that we are about to leave.
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_start_journal_flusher( AfsVolume_s * psVolume )
{
	thread_id hThread;

	hThread = spawn_kernel_thread( "afs_log_flusher", afs_journal_flusher, 0, 0, psVolume );
	if( hThread >= 0 )
	{
		wakeup_thread( hThread, false );
	}
	return( hThread );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void afs_flush_journal( AfsVolume_s * psVolume )
{
	printk( "Flushing journal:\n" );

	LOCK( psVolume->av_hJournalLock );

	flush_device_cache( psVolume->av_nDevice, false );

	if( psVolume->av_nOldBlockCount + psVolume->av_nNewBlockCount + psVolume->av_nJournalSize / 128 > psVolume->av_nJournalSize )
	{
		printk( "flush old journal entry\n" );
		afs_write_transaction_to_log( psVolume );
	}
	printk( "Merge last transaction\n" );
	if( psVolume->av_nNewBlockCount != 0 )
	{
		panic( "afs_flush_journal() found a half-finished transaction in the journal!\n" );
//      afs_merge_transactions( psVolume );
	}
	printk( "Write last transaction to the journal\n" );
	afs_write_transaction_to_log( psVolume );
	printk( "Wait for the transactions to finish\n" );
	afs_wait_for_log_space( psVolume, psVolume->av_nJournalSize );
	printk( "All journal entries flushed\n" );
	UNLOCK( psVolume->av_hJournalLock );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_logged_read( AfsVolume_s * psVolume, void *pBuffer, off_t nBlock )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsHashEnt_s *psEntry;
	int nError;

	psVolume->av_nLastJournalAccesTime = get_system_time();

	LOCK( psVolume->av_hJournalListsLock );

	psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, nBlock );

	if( psEntry != NULL )
	{
//      printk( "afs_logged_read() found block %Ld in log\n", nBlock );
		if( psEntry->he_psTransBuf != NULL )
		{
			memcpy( pBuffer, psEntry->he_psTransBuf->jb_pData, nBlockSize );
			nError = 0;
		}
		else if( psEntry->he_pBuffer != NULL )
		{
			memcpy( pBuffer, psEntry->he_pBuffer, nBlockSize );
			nError = 0;
		}
		else
		{
			panic( "afs_logged_read() both buffer pointers of hash entry is NULL!!\n" );
			nError = -EINVAL;
		}
	}
	else
	{
//      printk( "afs_logged_read() load block %Ld\n", nBlock );
		nError = cached_read( psVolume->av_nDevice, nBlock, pBuffer, 1, nBlockSize );
	}
	UNLOCK( psVolume->av_hJournalListsLock );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_logged_write( AfsVolume_s * psVolume, const void *pBuffer, off_t nBlock )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsJournalBlock_s *psBlock;
	AfsHashEnt_s *psEntry;
	int nError;

	LOCK( psVolume->av_hJournalListsLock );

	if( psVolume->av_nTransactionStarted != 1 )
	{
		panic( "afs_logged_write() called while av_nTransactionStarted = %d\n", psVolume->av_nTransactionStarted );
		nError = -EINVAL;
		goto error;
	}

	psEntry = afs_hash_lookup( &psVolume->av_sLogHashTable, nBlock );

	if( psEntry != NULL && psEntry->he_psTransBuf != NULL )
	{
		memcpy( psEntry->he_psTransBuf->jb_pData, pBuffer, nBlockSize );
		goto done;
	}

	psBlock = kmalloc( sizeof( AfsJournalBlock_s ) + nBlockSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if( psBlock == NULL )
	{
		printk( "Error: afs_logged_write() no memory for logg block\n" );
		nError = -ENOMEM;
		goto error;
	}

	if( psEntry == NULL )
	{
		psEntry = kmalloc( sizeof( AfsHashEnt_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
		if( psEntry == NULL )
		{
			printk( "Error: afs_logged_write() no memory for hash-table entry\n" );
			nError = -ENOMEM;
			kfree( psBlock );
			goto error;
		}
//      printk( "afs_logged_write() add block %Ld to hash table(%p)\n", nBlock, psVolume->av_psFirstTrans );
		afs_hash_insert( &psVolume->av_sLogHashTable, nBlock, psEntry );
		afs_add_jb_entry( psVolume->av_psFirstTrans, psEntry );
	}			/* else {
				   if( psEntry->he_psTransaction == NULL ) {
				   panic( "afs_logged_write() found entry in hash-table not belonging to any transaction\n" );
				   }
				   } */

	psVolume->av_nNewBlockCount++;

	psBlock->jb_pData =( uint8 * )( psBlock + 1 );
	psBlock->jb_nBlock = nBlock;

	psBlock->jb_psNext = psVolume->av_psFirstLogBlock;
	psVolume->av_psFirstLogBlock = psBlock;
	memcpy( psBlock->jb_pData, pBuffer, nBlockSize );

	psEntry->he_psTransBuf = psBlock;

      done:
	psVolume->av_nLastJournalAccesTime = get_system_time();

	UNLOCK( psVolume->av_hJournalListsLock );
	return( 0 );
      error:
	psVolume->av_nLastJournalAccesTime = get_system_time();
	UNLOCK( psVolume->av_hJournalListsLock );
	return( nError );
}
