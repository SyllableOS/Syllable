
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

/** Initialize a new hash table
 * \par Description:
 * Initialize the given hash table to the default size, and mark it empty.
 * \par Note:
 * \par Warning:
 * \param psTable	The hash table to initialize
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Delete a hash table
 * \par Description:
 * Free the table in the given hash table.
 * \par Note:
 * XXX Should set the size to zero
 * \par Warning:
 * \param psTable	The hash table to delete
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
void afs_delete_hash_table( AfsHashTable_s * psTable )
{
	kfree( psTable->ht_apsTable );
}

/** Resize a hash table
 * \par Description:
 * Resize the given hash table to twice it's previous size.  Allocate a new table twice
 * teh size of the old table.  Then, for each entry in the old table, rehash it into the
 * new table.  Finally, free the old hash table and store the new hashtable information.
 * \par Note:
 * \par Warning:
 * \param psTable	The hash table to resize
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Insert a new entry into a hash
 * \par Description:
 * Insert the given block number into the given hash using the given hash entry.   First,
 * check to see if the block number is alreay in the hash.  If so, return error.
 * Otherwise, store the block number in the given entry, and hash it into the table.  If
 * the hash table is now greater than 3/4 full, resize the hash table.
 * \par Note:
 * \par Warning:
 * Hash tables do not shrink
 * \param psTable	Hash table to insert into
 * \param nBlockNum	Block number to insert
 * \param psNew		New entry to insert in hash table
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Look up an entry in a hash
 * \par Description:
 * Lookup the given block number in the given hash table.  Hash the block number, and
 * walk the chain at that bucket looking for the give block number.
 * \par Note:
 * \par Warning:
 * \param psTable	The hash table to search
 * \param nBlockNum	The block number to look up
 * \return The hash entry if it's found, NULL otherwise
 * \sa
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

/** Delete an entry from a hash
 * \par Description:
 * Delete and return the hash entry for the given block number from the given hash.
 * First, look up the given block number in the hash.  If it's not found, return NULL.
 * Then, if the entry is the first in it's bucket, set the second in it's bucket to be
 * first.  Otherwise, set the next pointer in the previous entry to point to the next
 * entry.  Reduce the table count by one, and return the found entry.
 * \par Note:
 * \par Warning:
 * \param psTable	The hash table
 * \param nBlockNum	The block number to delete
 * \return The deleted hash entry if found, NULL otherwise
 * \sa
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

/** Remove a hash entry from a transaction
 * \par Description:
 * Remove the given entry from the given transaction.  This is a bog-standard
 * remove-from-doubly-linked-list function.
 * \par Note:
 * XXX The entry points to it's transaction. This should at least be checked, and
 * probably asserted.
 * \par Warning:
 * \param
 * \return
 * \sa
 ****************************************************************************/
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


/** Write out an AFS superblock
 * \par Description:
 * Write out the superblock of the given AFS volume.  If the write fails, or if the
 * amount written is not the same as the superblock size, return an error.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS volume containing superblock to write
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
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


/** Callback for logged write of cached blocks
 * \par Description:
 * When a transaction is written out, it uses a cached logged write, that is the write is
 * not immediate (cached), and journaled (logged).  This callback is passed into
 * write_logged_blocks, and is called each time a block in a transaction is written out.
 * All this does is decrement the number of blocks yet to be written, and panic if that
 * number goes negative.  It's more of a sanity check than anything else.
 * \par Note:
 * \par Warning:
 * XXX This assumes that one block is being written, and ignores nNumBlocks.  This is
 * currently true, but not necessarily true.  This will fail if the VFS ever writes out
 * multiple blocks at once.  This should be fixed.
 * \param nBlockNum		The block being written
 * \param nNumBlocks	The number of blocks being written
 * \param pArg			The passed in cookie, in this case the transaction being written
 * \return void
 * \sa
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

/** Allocate a block buffer for a transaction
 * \par Description:
 * A transaction contains a singly linked list of block buffers.  Each buffer can hold
 * 128 blocks.  First, determine the size of new new buffer, leaving space for the
 * memory-manager overhead, and extend that to the nearest page boundary.  Next allocate
 * an area to contain the buffer.  Note, this allocates the buffer itself, as well.
 * Advance the returned pointer over the memory manager data structures.  Zero out the
 * first block (the index block) of the buffer.  Store the area containing the buffer,
 * the number of free blocks, and the number of used blocks.  Insert the buffer at the
 * head of the buffer list in the transaction.
 * \par Note:
 * XXX The 128 should be changed to a #define and/or a runtime tunable, for tuning
 * \par Warning:
 * \param psVolume	AFS volume containing the data to be put in the transaction
 * \param psTrans	Transaction to which buffer will be added
 * \return The new buffer on success, NULL on failure
 * \sa afs_free_transaction_buffer
 ****************************************************************************/
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

/** Free a block buffer from a transaction
 * \par Description:
 * Merely delete the area containing the buffer.
 * \par Note:
 * XXX Alloc adds the buffer to the transaction, but free does not remove it.  This is
 * asymmetrical, and possibly a bug.
 * \par Warning:
 * \param psVolume	Volume containing the transaction containing the buffer
 * \param psBuffer	Buffer to free
 * \return void
 * \sa afs_alloc_transaction_buffer
 ****************************************************************************/
static void afs_free_transaction_buffer( AfsVolume_s * psVolume, AfsTransBuffer_s *psBuffer )
{
	delete_area( psBuffer->atb_hAreaID );
}

/** Replay the journal on an AFS volume
 * \par Description:
 * This will replay the journal on an AFS filesystem.  This is done prior t mounting.
 * First, if the filesystem is clean, set the current journal location to the start of
 * the journal and return.  Next, get an index block buffer and a block buffer.  While we
 * have outstanding journal blocks, loop.  First, read the index block. (An index block
 * indexes the destination disk address of the 128 consecutive journal blocks following
 * it.) Then, loop through the index block and the blocks it indexes in parallel, reading
 * each journal block into the block buffer and writing it to the destination address in
 * the corresponding index block entry.  If we reach the end of the journal, wrap around
 * to the beginning.  End loop.  Finally, set the current journal location to the start
 * of the journal, write out the superblock, and return.
 * \par Note:
 * \par Warning:
 * \param psVolume	The AFS volume containing the journal to replay
 * \return 0 on success, negative error code on failure.
 * \sa
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
			if( pIndex[i] == 0 || psSuperBlock->as_nValidLogBlocks == 0 )
  				break;

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

/** Allocate and initialize a new transaction.
 * \par Description:
 * Allocate a new transaction, determine it's start location in the journal, and link it
 * into the list of transactions for this volume.  A volume contains a doubly linked list
 * of transactions, in reverse-cronological order, so the most recent is first on the
 * list.  Therefore, the head of the list is the most recently committed transaction.
 * Set the begining of this transaction to one past the end of the previous transaction,
 * wrapping at the end of the journal.  If there are no previous transactions, set it
 * to the beginning of the journal.  The new transaction is linked into the head of the
 * list, and, if the tail pointer is NULL, into the tail of the list.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS Volume to add transaction to
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Begin an AFS transaction
 * \par Description:
 * Begin a new AFS transaction on the given volume.  Lock the journal.  If a transaction
 * is in progress, error.  If there are allocated transaction blocks, but no active
 * transaction, error.  Otherwise, mark this transaction started.  If a transaction
 * exists that has not been committed to the journal yet, use it.  Otherwise, allocate a
 * new one.  Note the transaction list is locked.
 * \par Note:
 * XXX The journal is locked, but not unlocked if there is an error.
 * \par Warning:
 * \param psVolume	The AFS volume to start a new transaction on
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Write out a transaction
 * \par Description:
 * Schedule the writeout of all the outstanding blocks in a transaction.  Since all I/O
 * is cached, the write will not necessarily happen now, but at some future time.  Loop
 * through all the hash entries in the transaction.  Then, for each block in the entry, if
 * it has not yet been written out, write the block to it's destination address, and mark
 * it written.  Note that all blocks in a transaction are written in sequential order, so
 * a count of the written blocks is all that is necessary for determining if a block is
 * written.  Increment the number of outstanding block writes.  A callback is registered
 * with each write, that decrements this counter.  When the counter is zero, the
 * transaction is actually written to disk. Next, walk all the hash entries in the
 * transaction.  If there is no buffer for this entry, free the entry.  Otherwise,
 * un-associate the entry with the transaction.  Finally, mark the transaction as
 * written.
 * \par Note:
 * This currently writes each block one at a time.  This is quite likely suboptimal, and
 * a trivial check could be added to coalesce multiple adjacent blocks into a single
 * write.  This would require the fix in the note for afs_block_flushed_callback.
 * \par Warning:
 * No checks are done to verify that the previous transaction is finished or that the
 * current really was written to disk.  If you don't check this before calling it you
 * will jepordize the poor users hard earned data.
 *
 * SEE ALSO:
 *	Also note that this function fetches a block with get_empty_block()
 *	and that if somebody reads that block before we write to it, they
 *	will get a cache hit on the empty block, and read invalid data.
 *	Therefore it is very important to avoid other threads accessing
 *	the cache block until it is gracefully updated. This is achived
 *	by locking the psVolume->av_hJournalListsLock mutex both while
 *	calling this function, and when reading meta data from the
 *	cache.
 * \param psVolume	AFS volume contiaining the transaction
 * \param psTrans	Transaction to write
 * \return 0 on success, negative error code on failure
 * \sa afs_block_flushed_callback
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

/** Flush a transaction from the block cache
 * \par Description:
 * Force the writeout of a transaction from the block cache.  Walk the list of buffers,
 * and, for each block in the buffer, flush it from the cache.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS volume containing the transaction
 * \param psTrans	Transaction to flush
 * \return
 * \sa
 ****************************************************************************/
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

/** Clean up a finished and committed transaction
 * \par Description:
 * If the oldest transaction on the given volume is written to the log and disk,
 * completely, free up the journal space it consumes, note in the volume and superblock
 * the reduced number of pending blocks, and unlink and free up the transaction and it's
 * buffers.  Then, if the (new) last transaction is written to the journal, initiait the
 * write-to-disk for that transaction.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
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

/** Free up space in the journal
 * \par Description:
 * Free up at least the given number of blocks in the kernel by looping on transactions,
 * flushing them to the journal, writing them to disk, and discarding them.
 * \par Note:
 * \par Warning:
 * If nBlockCount is larger than the journal, this will never terminate.
 * XXX add check and panic
 * \param psVolume		AFS volume containing journal with needed space
 * \param nBlockCount	Number of journal blocks to free
 * \return 0 on success
 * \sa
 * afs_force_fluch_transaction, afs_write_transaction_to_disk,
 * afs_discard_flushed_journal_entries
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

/** Write a transaction to the journal
 * \par Description:
 * Write the most recent transaction to the journal.  Note that, since only one
 * transaction can be outstanding at a time, this is the only transaction that has not
 * yet been writen to the journal.  Loop through the buffers in the transaction.  If this
 * buffer does not cross the end-of-journal boundry, write it directly to the disk (not
 * thorugh the block cache).  Otherwise, split it into two writes, one upto the end of
 * the journal, and one starting at the beginning, and write each one directly to disk.
 * Mark the transaction as written to the journal, and subtract the number of blocks
 * written from the total outstanding for this volume.  If the result is not zero, panic.
 * Add the number of blocks just journaled to the number of outstanding journaled blocks
 * in the volume, and write the superblock to the disk.  If the last (oldest) transaction
 * on the list for this volume exists, is journaled, and is not written to disk, then
 * initiate a write-to-disk for that transaction.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS volume containing transaction to write
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Validate the internal consistancy of a transaction
 * \par Description:
 * Validate the given transaction.  Walk the list of buffers in the transaction.  For
 * each buffer, check that all the blocks less than the blockcount for the buffer refer
 * to a non-zero disk address, and that all other blocks refer to a zero disk address.
 * Finally, assure that the number of blocks actually referenced in the transaction is
 * the same as the blockcount in the transaction.  If any of these checks fails, panic.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS volume containing the transaction
 * \param psTrans	Transaction to validate
 * \return
 * \sa
 ****************************************************************************/
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

/** Merge all outstanding blocks into the current transaction
 * \par Description:
 * Walk all the journal blocks in the journal for the given volume, merging them all in
 * the currently open transaction.  First, ensure the journal has enough space to hold
 * all the outstanding blocks on the volume, plus 1/128th of the size of the journal. For
 * each journal block, get it's hash entry.  If that journal block is not in the current
 * transaction, and does not yet have a buffer, allocate a new buffer, and increase the
 * number of outstanding blocks, pending blocks, and merged blocks.  If the buffer
 * allocation failed, jump to the error handling code.  Otherwise, grab a buffer from the
 * current transaction.  Set the block pointer of the current position in the buffer to
 * the block, increase the block count of the buffer,  and point the block buffer pointer
 * to the correct slot in the buffer.  Next, walk the list of journal blocks.  For each
 * one, get it's hash entry.  If the entry is not part of this transaction, or it doesn't
 * have a buffer, copy the data from the block into new buffer allocated in the previous
 * loop, and, if it's not in this transaction, remove it from it's current transaction
 * and add it to this one.  Otherwise, just copy the data from the block into it's
 * buffer.  Finally, update the block counts in the transaction and volume, and validate
 * the transaction.  In the case of a buffer allocation failure, free buffers from the
 * transaction until there is enough to hold all the outstanding blocks.  Validate the
 * transaction and return the error.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume containing transactions to merge
 * \return 0 on success, negative error code on failure
 * \sa
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


/** Discard journal blocks not yet written
 * \par Description:
 * Discard any blocks belonging to the previous transaction, leaving them unwritten.
 * This will cancel a transaction as if it never happend, in case it fails part way
 * through.  Walk the list of open journal block in the volume.  For each block, get it's
 * hash entry.  If the has entry is not waiting for writing, free it's hash entry and
 * remove it from it's transaction (if any).  Advance the first block pointer in the
 * volume, and free the block.  Fix up the used blocks and allocated blocks numbers in
 * the volume.  Next, for each remaining hash entry, verify that it's being written back.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
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

/** Flush journal until block is free
 * \par Description:
 * Flush completed journal entries until the given block is free.  Loop until the given
 * block is no-longer in the journal hash.  In this loop, if this is the third time
 * through, write the current transaction to the journal and, if that succeeded, create a
 * new transaction. If the last transaction is not flushed to disk, flush it.  If the
 * last transaction has ben written to the journal but not to the disk, write it to the
 * disk.  Discard any flushed journal entries, increment the run number, and loop.
 * \par Note:
 * - The write-to-disk and the flush-transaction should probably be done in the opposite
 *   order, so that you write-then-flush, rather than flush-then-write (which doesn't do
 *   any good).  Kurt may have had a reason for this.
 * - I'm not sure about the magic third-time-through-creates-a-new-transaction thing.
 *   I'm not sure a new transaction should be created at all, let alone only sometimes.
 *   This is only called from the stream code, which doesn't seem to have a concept of a
 *   transaction, so maybe it's okay.
 * \par Warning:
 * No error handling for failure of called functions.  Should probably error out.
 * \param psVolume		AFS volume containing journal
 * \param nBlock		Block number of block to wait for
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Mark journal transaction split point
 * \par Description:
 * From Kurt:
 *	This function is called by the file-system to tell that it is now safe to split the
 *	transaction to avoid journal overflow.  This can be necessary during operations that
 *	might touch more blocks than can possibly fit in the journal. It breaks the atomicity
 *	of the operation, but is only performed at times where a half finnished operation
 *	won't threathen the integrity of the file-system.  One example where this can happen
 *	is when deleting a file with lot's of attributes. If the entire deletion whould be
 *	performed in one transaction, the blocks touched by both the file, and all it's
 *	attributes might exceed the size of the log. By allowing the journaling system to
 *	start a new transaction between every attribute, we avoid overflowing the log, but
 *	risk that a crash will leave the file with half it's attributes deleted. This is not
 *	good, but far better than jepordizing the entire file-system by overflowing the log.
 * From DFG:
 * Lock the transaction list, and check to see if we're about to overflow the journal.
 * If the outstanding journal entries + the current transaction + 1/128th of the journal
 * size will overflow, then write the current transaction and start a new one. Then, if
 * that succeded, merge the new transaction into the journal, and, if that succeeded,
 * flush the journal.  Finally, if the flush succeeded, mark the journal empty.  Unlock
 * the journal and return error code.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume containing journal
 * \return 0 on success, negative error code on failure.
 * \sa
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

/** End a journal transaction
 * \par Description:
 * End a journal transaction.  This is called when a transaction is complete, and it
 * writes the transaction to the disk journal.  First, lock the transaction list.  Next,
 * if the total outstanding journal size in blocks is greater than 128, write the
 * transaction and create a new one. (DFG What???) Next, if bSubmitChanges is TRUE, merge
 * in the outstanding transaction, and, if that fails, discard the outstanding
 * transaction.  If it succeeded, mark the journal empty.  Otherwise, if bSubmitChanges
 * is false, discard the outstanding transaction.  Either way, flush the journal, and
 * unlock the journal.  Reduce the number of started transactions, timestamp the journal,
 * and unlock the main journal lock taken in  afs_begin_transaction.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume containing journal
 * \param
 * \return
 * \sa
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

/** Periodically flush journal entries
 * \par Description:
 * It's highly undesirable that journal entries hang around forever.  They should be
 * flushed periodically, to avoid re-appearing deleted files, and other problems if there
 * is a crash.  This function does that flushing.  In an infinite loop, sleep for 1
 * second, then lock the journal and transaction list.  If there's an in-progress
 * transaction, panic and continue.  Otherwise, write the outstanding transactions to
 * log, clean up any written-to-disk journal entries.  Unlock the transaction list and
 * journal, and loop again.
 * \par Note:
 * DFG says "This function never exits, despite the comment at the bottom", but if that
 * was true, afs_unmount would never return either... and it does, doesn't it!?
 * \par Warning:
 * \param pData		Volume to flush, in the form of a void data pointer
 * \return 0
 * \sa
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
			//XXX DFG unlock!
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

/** Start the periodic journal flushing thread
 * \par Description:
 * Create a kernel thread to periodically flush the journal for a volume, and start it
 * running.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS volume to flush
 * \return thread ID of flushing thread
 * \sa
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

/** Flush the journal for an AFS volume
 * \par Description:
 * This is the external entry point to flush the journal for a volume.  Flush the journal
 * for the given volume to disk.  First, lock the journal.  Next, if the outstanding
 * journal entries + 1/128th of the journal size is greater than the journal size, write
 * the current transaction to the journal.  Then, if there is an outstanding transaction,
 * panic (should come first.  Should not be possible, because of the journal lock).
 * Next, unconditionally write the outstanding transactions to the journal.  Wait for the
 * writes to complete, then unlock the journal.
 * \par Note:
 * This should be cleaned up.  Just test for open transaction and panic, then write to
 * log, wait for space, and unlock.
 * \par Warning:
 * \param
 * \return
 * \sa
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

/** Read a block from the journal
 * \par Description:
 * This will read a block from the disk, getting it from the journal, if it's there.
 * This is atomic with respect to logged writes, transactions, and other logged reads,
 * and will pick up changes to the block that have been journalled but not yet written to
 * disk.  First, timestamp the journal.  Then, lock the transaction list.  This means
 * that no other journalled reads can occur, no logged_writes can occur, and no
 * transactions can begin or end while this read is occuring.  Look for the block in the
 * journal.  If it's there, copy from the transaction buffer (for unfinished
 * transactions) or the journal buffer (for completed but not committed transactions)
 * into the destination buffer.  Otherwise, read from disk.  Unlock the transaction list,
 * allowing other reads/writes to proceed.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume from which to read
 * \param pBuffer		Buffer to read into
 * \param nBlock		AFS disk block to read
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Write a block through the journal
 * \par Description:
 * This will write a block to the disk through the journal, using the currently open
 * transaction.  This is atomic with respect to logged reads, transaction begin/end, and
 * other logged writes.  First, lock the transaction list.  This ensures logged_read,
 * other logged_writes, and transaction begin/end cannot occur until this is done.  Next,
 * see if the block is already in a journal entry.  If it is, and it has a transaction
 * buffer (ie it's not yet flushed), copy the data into the buffer.  Otherwise, create a
 * new transaction buffer.  If the block did not have a journal entry, create one and add
 * it to the journal hash.  Add the block to the current transaction, and copy the data
 * into it.  Timestamp the journal, and unlock the transaction list.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS volume to write to
 * \param pBuffer		Data to write
 * \param nBlock		AFS block number to write to
 * \return 0 on success, negative error code on failure
 * \sa
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
