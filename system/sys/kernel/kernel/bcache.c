
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

#include <atheos/types.h>
#include <posix/unistd.h>
#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>
#include <atheos/pci.h>
#include <atheos/kdebug.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/bcache.h"

BlockCache_s g_sBlockCache;

static const size_t READAHEAD_SIZE	= (32 * 1024);
static const size_t BC_FLUSH_SIZE	= 64;

static volatile bigtime_t g_nLastAccess = 0;

#define MAX_DEVICES	1024

typedef struct
{
	fs_id de_nFilesystem;
	off_t de_nBlockCount;
	kdev_t de_nDeviceID;
	ino_t de_nDeviceInode;
} DeviceEntry_s;


static DeviceEntry_s g_asDevices[MAX_DEVICES];

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static const size_t HT_DEFAULT_SIZE	= 1024;

#define HASH(d, b)   ((((off_t)d) << (sizeof(off_t)*8 - 6)) | (b))

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t init_hash_table( HashTable_s * psTable )
{
	psTable->ht_nSize = HT_DEFAULT_SIZE;
	psTable->ht_nMask = psTable->ht_nSize - 1;
	psTable->ht_nCount = 0;
	psTable->ht_apsTable = NULL;

	psTable->ht_hArea = create_area( "bcache_hash_table", ( void ** )&psTable->ht_apsTable, psTable->ht_nSize * sizeof( CacheBlock_s * ), psTable->ht_nSize * sizeof( CacheBlock_s * ), AREA_ANY_ADDRESS | AREA_FULL_ACCESS | AREA_KERNEL, AREA_FULL_LOCK );

	if ( psTable->ht_hArea < 0 )
	{
		return ( -ENOMEM );
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

static status_t resize_hash_table( HashTable_s * psTable )
{
	if ( psTable->ht_nSize & psTable->ht_nMask )
	{
		panic( "bcache:resize_hash_table() Inconsistency between hashtable size %d and mask %d in bcache!\n", psTable->ht_nSize, psTable->ht_nMask );
	}

	int nPrevSize = psTable->ht_nSize;
	int nNewSize = nPrevSize * 2;
	int nNewMask = nNewSize - 1;
	CacheBlock_s **pasNewTable = NULL;

	area_id hNewArea = create_area( "bcache_hash_table", ( void ** )&pasNewTable, nNewSize * sizeof( CacheBlock_s * ), nNewSize * sizeof( CacheBlock_s * ), AREA_ANY_ADDRESS | AREA_FULL_ACCESS | AREA_KERNEL, AREA_FULL_LOCK );

	if ( hNewArea < 0 )
	{
		return ( -ENOMEM );
	}

	for ( count_t i = 0; i < nPrevSize; ++i )
	{
		CacheBlock_s *psEntry;
		CacheBlock_s *psNext;

		for ( psEntry = psTable->ht_apsTable[i]; NULL != psEntry; psEntry = psNext )
		{
			int nHash = HASH( psEntry->cb_nDevice, psEntry->cb_nBlockNum ) & nNewMask;
			psNext = psEntry->cb_psNextHash;

			psEntry->cb_psNextHash = pasNewTable[nHash];
			pasNewTable[nHash] = psEntry;
		}
	}
	delete_area( psTable->ht_hArea );
	psTable->ht_apsTable = pasNewTable;
	psTable->ht_hArea = hNewArea;
	psTable->ht_nSize = nNewSize;
	psTable->ht_nMask = nNewMask;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bc_hash_insert( HashTable_s * psTable, dev_t nDev, off_t nBlockNum, CacheBlock_s *psBuffer )
{
	CacheBlock_s *psTmp;

	int nHash = HASH( nDev, nBlockNum ) & psTable->ht_nMask;

	for ( psTmp = psTable->ht_apsTable[nHash]; psTmp != NULL; psTmp = psTmp->cb_psNextHash )
	{
		if ( psTmp->cb_nDevice == nDev && psTmp->cb_nBlockNum == nBlockNum )
		{
			break;
		}
	}

	if ( ( NULL != psTmp && psTmp->cb_nDevice == nDev && psTmp->cb_nBlockNum == nBlockNum ) || psTmp == psBuffer )
	{
		panic( "bc_hash_insert() Entry already in the hash table! Dev=%d , Block=%d\n", nDev, ( int )nBlockNum );
		return ( -EEXIST );
	}
//    psBuffer->cb_nHashVal = HASH( nDev, nBlockNum );
	psBuffer->cb_psNextHash = psTable->ht_apsTable[nHash];
	psTable->ht_apsTable[nHash] = psBuffer;
	psTable->ht_nCount++;

	if ( psTable->ht_nCount < 65536 && psTable->ht_nCount >= ( ( psTable->ht_nSize * 3 ) / 4 ) )
	{
		resize_hash_table( psTable );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void *hash_lookup( HashTable_s * psTable, dev_t nDev, off_t nBlockNum )
{
	int nHash = HASH( nDev, nBlockNum ) & psTable->ht_nMask;
	CacheBlock_s *psEnt;

	for ( psEnt = psTable->ht_apsTable[nHash]; psEnt != NULL; psEnt = psEnt->cb_psNextHash )
	{
		if ( psEnt->cb_nDevice == nDev && psEnt->cb_nBlockNum == nBlockNum )
		{
			return ( psEnt );
		}
	}
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void *bc_hash_delete( HashTable_s * psTable, dev_t nDev, off_t nBlockNum )
{
	int nHash = HASH( nDev, nBlockNum ) & psTable->ht_nMask;
	CacheBlock_s *psTmp;
	CacheBlock_s *psPrev = NULL;

	for ( psTmp = psTable->ht_apsTable[nHash]; psTmp != NULL; psPrev = psTmp, psTmp = psTmp->cb_psNextHash )
	{
		if ( psTmp->cb_nDevice == nDev && psTmp->cb_nBlockNum == nBlockNum )
		{
			break;
		}
	}

	if ( psTmp == NULL )
	{
		panic( "bc_hash_delete() Entry %d , %Ld does not exist!\n", nDev, nBlockNum );
		return ( NULL );
	}

	if ( psTable->ht_apsTable[nHash] == psTmp )
	{
		psTable->ht_apsTable[nHash] = psTmp->cb_psNextHash;
	}
	else
	{
		if ( NULL != psPrev )
		{
			psPrev->cb_psNextHash = psTmp->cb_psNextHash;
		}
		else
		{
			panic( "bcache:bc_hash_delete() invalid hash table!\n" );
		}
	}
	psTable->ht_nCount--;

	if ( psTable->ht_nCount < 0 )
	{
		panic( "bcache:bc_hash_delete() ht_nCount got negative value %d!!!!!\n", psTable->ht_nCount );
	}
	return ( psTmp );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void bc_add_to_head( CacheEntryList_s * psList, CacheBlock_s *psEntry )
{
	if ( NULL != psEntry->cb_psNext || NULL != psEntry->cb_psPrev )
	{
		panic( "bcache:bc_add_to_head() Attempt to add entry twice!\n" );
	}

	if ( NULL != psList->cl_psMRU )
	{
		psList->cl_psMRU->cb_psNext = psEntry;
	}
	if ( NULL == psList->cl_psLRU )
	{
		psList->cl_psLRU = psEntry;
	}

	psEntry->cb_psNext = NULL;
	psEntry->cb_psPrev = psList->cl_psMRU;
	psList->cl_psMRU = psEntry;
	psList->cl_nCount++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static void add_to_tail( CacheEntryList_s * psList, CacheBlock_s *psEntry )
{
	if ( NULL != psEntry->cb_psNext || NULL != psEntry->cb_psPrev )
	{
		panic( "bcache:add_to_tail() Attempt to add entry twice!\n" );
	}


	if ( NULL != psList->cl_psLRU )
	{
		psList->cl_psLRU->cb_psPrev = psEntry;
	}
	if ( NULL == psList->cl_psMRU )
	{
		psList->cl_psMRU = psEntry;
	}

	psEntry->cb_psNext = psList->cl_psLRU;
	psEntry->cb_psPrev = NULL;

	psList->cl_psLRU = psEntry;
	psList->cl_nCount++;
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void bc_remove_from_list( CacheEntryList_s * psList, CacheBlock_s *psEntry )
{
	if ( NULL != psEntry->cb_psNext )
	{
		psEntry->cb_psNext->cb_psPrev = psEntry->cb_psPrev;
	}
	if ( NULL != psEntry->cb_psPrev )
	{
		psEntry->cb_psPrev->cb_psNext = psEntry->cb_psNext;
	}

	if ( psList->cl_psLRU == psEntry )
	{
		psList->cl_psLRU = psEntry->cb_psNext;
	}
	if ( psList->cl_psMRU == psEntry )
	{
		psList->cl_psMRU = psEntry->cb_psPrev;
	}

	psEntry->cb_psNext = NULL;
	psEntry->cb_psPrev = NULL;

	psList->cl_nCount--;
	if ( psList->cl_nCount < 0 )
	{
		panic( "bc_remove_from_list() cache buffer list %p got count of %d\n", psList, psList->cl_nCount );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static CacheBlock_s *lookup_block( dev_t nDev, off_t nBlockNum )
{
	CacheBlock_s *psBlock;
	bool bWarned = false;

	for ( count_t i = 0;; ++i )
	{
		psBlock = hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum );
		if ( NULL == psBlock )
		{
			return ( NULL );
		}

		if ( ( psBlock->cb_nFlags & CBF_BUSY ) == 0 && ( psBlock->cb_nDevice != nDev || psBlock->cb_nBlockNum != nBlockNum ) )
		{
			panic( "lookup_block() requested %d::%Ld got %d::%Ld!!!\n", nDev, nBlockNum, psBlock->cb_nDevice, psBlock->cb_nBlockNum );
			if ( bWarned )
			{
				printk( "Phew! lookup_block() block %d::%Ld previously stuck in busy state got free!\n", nDev, nBlockNum );
			}
			return ( NULL );
		}

		if ( ( psBlock->cb_nFlags & CBF_BUSY ) == 0 )
		{
			break;
		}

		unlock_semaphore( g_sBlockCache.bc_hLock );
		snooze( 1000 );
		if ( i > 5000 )
		{
			if ( bWarned == false )
			{
				printk( "Panic : lookup_block() block %d::%Ld stuck in busy state!\n", nDev, nBlockNum );
				trace_stack( 0, NULL );
				bWarned = true;
			}
			snooze( 1000000 );
			i = 0;
		}
		lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );
	}
	if ( psBlock->cb_nFlags & CBF_BUSY )
	{
		panic( "lookup_block() returned busy block!\n" );
	}
	if ( bWarned )
	{
		printk( "Phew! lookup_block() block %d::%Ld previously stuck in busy state got free!\n", nDev, nBlockNum );
	}
	return ( psBlock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static int assert_valid_block( CacheBlock_s *psBuffer )
{
	return ( 0 );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int cmp_blocks( const void *pBlk1, const void *pBlk2 )
{
	const CacheBlock_s *const *psBlk1 = pBlk1;
	const CacheBlock_s *const *psBlk2 = pBlk2;

	if ( ( *psBlk1 )->cb_nDevice != ( *psBlk2 )->cb_nDevice )
	{
		return ( ( *psBlk1 )->cb_nDevice - ( *psBlk2 )->cb_nDevice );
	}
	return ( ( *psBlk1 )->cb_nBlockNum - ( *psBlk2 )->cb_nBlockNum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t flush_block_list( CacheBlock_s **pasBlocks, count_t nCount, bool bFlushLocked )
{
	struct iovec pasIoVecs[BC_FLUSH_SIZE];
	int nIoVecCnt = 0;
	CacheBlock_s *psFirstBuffer;
	CacheBlock_s *psPrevBuffer;
	status_t nError = 0;
	count_t i;

	psPrevBuffer = NULL;
	psFirstBuffer = pasBlocks[0];

	if ( nCount > BC_FLUSH_SIZE )
	{
		panic( "flush_block_list() to many blocks: %d\n", nCount );
	}

	for ( i = 0; i < nCount && nError >= 0; ++i )
	{
		CacheBlock_s *psBuffer = pasBlocks[i];

		if ( psBuffer->cb_nRefCount > 0 && !bFlushLocked )
		{
			printk( "flush_block_list() Got locked block, not flushing\n" );
			continue;
		}
		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			continue;
		}
		kassertw( psBuffer->cb_nDevice >= 0 );

		if ( i > 0 && pasBlocks[i - 1]->cb_nDevice == pasBlocks[i]->cb_nDevice && pasBlocks[i - 1]->cb_nBlockNum >= pasBlocks[i]->cb_nBlockNum )
		{
			panic( "flush_block_list() Block %d is greater than block %d (%d) (bad sorting)\n", ( int )pasBlocks[i - 1]->cb_nBlockNum, ( int )pasBlocks[i]->cb_nBlockNum, i );
		}
		kassertw( 0 == psBuffer->cb_nRefCount || bFlushLocked );

		if ( NULL != psPrevBuffer && ( psPrevBuffer->cb_nDevice != psBuffer->cb_nDevice || psPrevBuffer->cb_nBlockNum != ( psBuffer->cb_nBlockNum - 1 ) ) )
		{
			if ( psPrevBuffer->cb_nBlockNum - psFirstBuffer->cb_nBlockNum != nIoVecCnt - 1 )
			{
				for ( count_t j = 0; j < nCount; j++ )
				{
					printk( "%d -> FLG=%02d, RC=%02d, Num=%Ld\n", j, pasBlocks[j]->cb_nFlags, pasBlocks[j]->cb_nRefCount, pasBlocks[j]->cb_nBlockNum );
				}
				panic( "flush_block_list:1() : Range %d - %d does not match with the length %d for node %d!\n", ( int )psFirstBuffer->cb_nBlockNum, ( int )psPrevBuffer->cb_nBlockNum, nIoVecCnt, i );
			}
			nError = writev_pos( psFirstBuffer->cb_nDevice, psFirstBuffer->cb_nBlockNum * CB_SIZE( psFirstBuffer ), pasIoVecs, nIoVecCnt );

			psFirstBuffer = psBuffer;
			nIoVecCnt = 0;
		}
		pasIoVecs[nIoVecCnt].iov_base = CB_DATA( psBuffer );
		pasIoVecs[nIoVecCnt].iov_len = CB_SIZE( psBuffer );
		nIoVecCnt++;

		if ( NULL == psPrevBuffer )
		{
			psFirstBuffer = psBuffer;
		}
		psPrevBuffer = psBuffer;
	}

	if ( nIoVecCnt > 0 )
	{
		if ( psPrevBuffer->cb_nBlockNum - psFirstBuffer->cb_nBlockNum != nIoVecCnt - 1 )
		{
			for ( count_t j = 0; j < nCount; j++ )
			{
				printk( "%d -> FLG=%02d, RC=%02d, Num=%Ld\n", j, pasBlocks[j]->cb_nFlags, pasBlocks[j]->cb_nRefCount, pasBlocks[j]->cb_nBlockNum );
			}
			panic( "flush_block_list:2() : Range %d - %d does not match with the length %d for node%d!\n", ( int )psFirstBuffer->cb_nBlockNum, ( int )psPrevBuffer->cb_nBlockNum, nIoVecCnt, i );
		}

		nError = writev_pos( psFirstBuffer->cb_nDevice, psFirstBuffer->cb_nBlockNum * CB_SIZE( psFirstBuffer ), pasIoVecs, nIoVecCnt );
		nIoVecCnt = 0;
	}

	for ( i = 0; i < nCount; ++i )
	{
		CacheBlock_s *psBuffer = pasBlocks[i];
		cache_callback *pFunc = psBuffer->cb_pFunc;
		void *pArg = psBuffer->cb_pArg;
		off_t nBlock = psBuffer->cb_nBlockNum;

		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			continue;
		}
		if ( psBuffer->cb_nRefCount > 0 && !bFlushLocked )
		{
			continue;
		}
		psBuffer->cb_pFunc = NULL;
		psBuffer->cb_nFlags &= ~CBF_DIRTY;
		atomic_dec( &g_sBlockCache.bc_nDirtyCount );
		atomic_sub( &g_sSysBase.ex_nDirtyCacheSize, CB_SIZE( psBuffer ) );
		if ( NULL != pFunc )
		{
			pFunc( nBlock, 1, pArg );
		}
	}

	for ( count_t i = 0; i < nCount; ++i )
	{
		if ( ( pasBlocks[i]->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			continue;
		}
		printk( "Panic: flush_block_list() failed to write back block %d\n", ( int )pasBlocks[i]->cb_nBlockNum );
	}
	if ( nError < 0 )
	{
		printk( "Panic : flush_block_list() failed!!\n" );
	}
	return ( nError );
}


void release_cache_blocks( int nBlockSize )
{
	CacheBlock_s *apsBlockList[BC_FLUSH_SIZE];	// FIXME : Too much stack usage!!!!!!!!!!!
	CacheBlock_s *psBlock;
	count_t nCount = 0;
	count_t nNumDirty = 0;

	for ( psBlock = g_sBlockCache.bc_sNormal.cl_psLRU; NULL != psBlock; psBlock = psBlock->cb_psNext )
	{
		kassertw( 0 == psBlock->cb_nRefCount );

		if ( psBlock->cb_nFlags & CBF_BUSY )
		{
			continue;
		}
		if ( psBlock->cb_nDevice == -1 )
		{
			printk( "release_cache_blocks() Block with dev=-1!!\n" );
			continue;
		}
		if( nBlockSize != -1 && CB_SIZE( psBlock ) != nBlockSize )
		{
			continue;
		}
		if ( psBlock->cb_nFlags & CBF_DIRTY )
		{
			nNumDirty++;
		}

		psBlock->cb_nFlags |= CBF_BUSY;
		apsBlockList[nCount++] = psBlock;

		if ( nCount >= BC_FLUSH_SIZE )
		{
			break;
		}
	}
	if ( nNumDirty > 0 )
	{
		qsort( apsBlockList, nCount, sizeof( CacheBlock_s * ), cmp_blocks );
		UNLOCK( g_sBlockCache.bc_hLock );
		flush_block_list( apsBlockList, nCount, false );
		LOCK( g_sBlockCache.bc_hLock );
	}
	for ( count_t i = 0; i < nCount; ++i )
	{
		psBlock = apsBlockList[i];

		bc_remove_from_list( &g_sBlockCache.bc_sNormal, psBlock );
		if ( bc_hash_delete( &g_sBlockCache.bc_sHashTab, psBlock->cb_nDevice, psBlock->cb_nBlockNum ) != psBlock )
		{
			panic( "Failed to remove cache block from hash-table\n" );
		}
		free_cache_block( psBlock );
	}
}

size_t shrink_block_cache( size_t nBytesNeeded )
{
	size_t nTotalFreed = 0;
	count_t nRetries = 0;

	while ( nTotalFreed < nBytesNeeded && atomic_read( &g_sSysBase.ex_nBlockCacheSize ) > 0 )
	{
		int nFreed;

		LOCK( g_sBlockCache.bc_hLock );
		release_cache_blocks( -1 );
		
		if( ( nFreed = shrink_cache_heaps( -1 ) ) == -EAGAIN )
		{
			UNLOCK( g_sBlockCache.bc_hLock );
			snooze( 1000 );
			LOCK( g_sBlockCache.bc_hLock );
		}
		UNLOCK( g_sBlockCache.bc_hLock );
		if ( nFreed > 0 )
		{
			nTotalFreed += nFreed;
		}
		if ( nFreed <= 0 && nRetries++ > 10000 )
		{
			printk( "Warning: shrink_block_cache() failed to shrink the block-cache. Current size: %d\n", atomic_read( &g_sSysBase.ex_nBlockCacheSize ) );
			break;
		}
	}
	return ( nTotalFreed );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t read_block_list( dev_t nDev, off_t nBlockNum, CacheBlock_s **apsList, count_t nCount, size_t nBlockSize )
{
	struct iovec asIoVecs[BC_FLUSH_SIZE];	// FIXME : Too much stack usage!!!!!!!!!!!
	status_t nError;

	kassertw( nCount <= BC_FLUSH_SIZE );

	for ( count_t i = 0; i < nCount; ++i )
	{
		kassertw( apsList[i]->cb_nFlags & CBF_BUSY );
		kassertw( ( apsList[i]->cb_nFlags & CBF_DIRTY ) == 0 );

		asIoVecs[i].iov_base = CB_DATA( apsList[i] );
		asIoVecs[i].iov_len = nBlockSize;
	}

	nError = readv_pos( nDev, nBlockNum * nBlockSize, asIoVecs, nCount );
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static void dump_list( CacheEntryList_s * psList )
{
	CacheBlock_s *psEntry;
	int i = 0;

	for ( psEntry = g_sBlockCache.bc_sNormal.cl_psLRU; NULL != psEntry; psEntry = psEntry->cb_psNext )
	{
		printk( "%d -> FLG=%02ld, RC=%02d, Num=%Ld\n", i++, psEntry->cb_nFlags, psEntry->cb_nRefCount, psEntry->cb_nBlockNum );
	}
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static CacheBlock_s *do_get_empty_block( dev_t nDev, off_t nBlockNum, size_t nBlockSize )
{
	CacheBlock_s *psBuffer;

      again:
	psBuffer = lookup_block( nDev, nBlockNum );

	if ( NULL != psBuffer )
	{
		psBuffer->cb_nRefCount++;

		if ( 1 == psBuffer->cb_nRefCount )
		{
			bc_remove_from_list( &g_sBlockCache.bc_sNormal, psBuffer );
			bc_add_to_head( &g_sBlockCache.bc_sLocked, psBuffer );
		}
	}
	else
	{
		bool bBlocked;

		psBuffer = NULL;
		if ( alloc_cache_blocks( &psBuffer, 1, nBlockSize, &bBlocked ) < 1 )
		{
//          release_cache_blocks();
//          goto again;
			panic( "do_get_empty_block() failed to alloc cache block\n" );
		}
		if ( bBlocked )
		{
			free_cache_block( psBuffer );
			goto again;
		}

/*	
	psBuffer = alloc_cache_block( nBlockSize );

	if ( psBuffer == NULL ) {
	    release_cache_blocks();
	    goto again;
	}
	*/
		if ( psBuffer == NULL )
		{
			panic( "Failed to alloc cache block\n" );
		}
		psBuffer->cb_nDevice = nDev;
		psBuffer->cb_nBlockNum = nBlockNum;
		psBuffer->cb_nFlags &= ~CBF_BUSY;
		psBuffer->cb_nRefCount++;
		bc_hash_insert( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum, psBuffer );

		bc_add_to_head( &g_sBlockCache.bc_sLocked, psBuffer );
	}
	g_nLastAccess = get_system_time();
	return ( psBuffer );
}

void *get_empty_block( dev_t nDev, off_t nBlockNum, size_t nBlockSize )
{
	CacheBlock_s *psBuffer = NULL;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: get_empty_block() invalid device %d\n", nDev );
		goto error;
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: get_empty_block() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		goto error;
	}

	psBuffer = do_get_empty_block( nDev, nBlockNum, nBlockSize );
	if ( psBuffer != NULL )
	{
		CURRENT_THREAD->tr_nNumLockedCacheBlocks++;
	}
      error:
	unlock_semaphore( g_sBlockCache.bc_hLock );
	return ( ( NULL != psBuffer ) ? CB_DATA( psBuffer ) : NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void *get_cache_block( dev_t nDev, off_t nBlockNum, size_t nBlockSize )
{
	CacheBlock_s *apsBlockList[BC_FLUSH_SIZE];
	CacheBlock_s *psBuffer = NULL;
	size_t nMaxReadahead = READAHEAD_SIZE / nBlockSize;
	size_t nListSize = 0;

	if ( nMaxReadahead > BC_FLUSH_SIZE )
	{
		nMaxReadahead = BC_FLUSH_SIZE;
	}
	LOCK( g_sBlockCache.bc_hLock );

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: get_cache_block() invalid device %d\n", nDev );
		goto error;
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: get_cache_block() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		goto error;
	}

	psBuffer = lookup_block( nDev, nBlockNum );

	if ( NULL != psBuffer )
	{
		psBuffer->cb_nRefCount++;
		if ( 1 == psBuffer->cb_nRefCount )
		{
			bc_remove_from_list( &g_sBlockCache.bc_sNormal, psBuffer );
			bc_add_to_head( &g_sBlockCache.bc_sLocked, psBuffer );
		}
	}
	else
	{
		CacheBlock_s sGuardBlock;
		count_t nNumNeeded;
		off_t nReqBlock;
		bool bBlocked;

		nReqBlock = nBlockNum;

		// Scan forward to see how many blocks we can "readahed"
		for ( nNumNeeded = 1; nNumNeeded < nMaxReadahead && ( nBlockNum + nNumNeeded < g_asDevices[nDev].de_nBlockCount ); ++nNumNeeded )
		{
			if ( hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum + nNumNeeded ) != NULL )
			{
				break;
			}
		}

		// If we got less than nMaxReadahead blocks we check if we
		// can start reading before the requested block to increase
		// the readahed size

		for ( ; nNumNeeded < nMaxReadahead && nBlockNum > 0; ++nNumNeeded )
		{
			if ( hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum - 1 ) != NULL )
			{
				break;
			}
			nBlockNum--;
		}

		memset( &sGuardBlock, 0, sizeof( sGuardBlock ) );

		sGuardBlock.cb_nFlags |= CBF_BUSY;
		sGuardBlock.cb_nDevice = nDev;
		sGuardBlock.cb_nBlockNum = nReqBlock;
		bc_hash_insert( &g_sBlockCache.bc_sHashTab, nDev, nReqBlock, &sGuardBlock );

		nListSize = alloc_cache_blocks( apsBlockList, nNumNeeded, nBlockSize, &bBlocked );

		if ( bc_hash_delete( &g_sBlockCache.bc_sHashTab, nDev, nReqBlock ) != &sGuardBlock )
		{
			panic( "Failed to remove guard block from hash-table\n" );
		}
		
		kassertw( bBlocked || nListSize == nNumNeeded );

		if ( bBlocked )
		{
			nBlockNum = nReqBlock;
			// Scan forward to see how many blocks we can "readahed"
			for ( nNumNeeded = 1; nNumNeeded < nListSize && ( nBlockNum + nNumNeeded < g_asDevices[nDev].de_nBlockCount ); ++nNumNeeded )
			{
				if ( hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum + nNumNeeded ) != NULL )
				{
					break;
				}
			}

			// If we got less than nMaxReadahead blocks we check if we
			// can start reading before the requested block to increase
			// the readahed size

			for ( ; nNumNeeded < nListSize && nBlockNum > 0; ++nNumNeeded )
			{
				if ( hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum - 1 ) != NULL )
				{
					break;
				}
				nBlockNum--;
			}
		}
		if ( nListSize == 0 )
		{
			panic( "get_cache_block() failed to alloc buffer\n" );
		}
		if ( nListSize < nNumNeeded )
		{
			// Make sure we atleast loads the requested block.
			int nMaxBlocks = nBlockNum + nNumNeeded - nReqBlock;

			if ( nListSize <= nMaxBlocks )
			{
				nBlockNum = nReqBlock;
			}
			else
			{
				nBlockNum = nReqBlock + nMaxBlocks - nListSize;
			}
			nNumNeeded = nListSize;
		}
		else if ( nListSize > nNumNeeded )
		{
			// Free excessive blocks
			for ( count_t i = nNumNeeded; i < nListSize; ++i )
			{
				free_cache_block( apsBlockList[i] );
			}
			nListSize = nNumNeeded;
		}

		psBuffer = NULL;
		for ( count_t i = 0; i < nListSize; ++i )
		{
			CacheBlock_s *psBlock = apsBlockList[i];

			psBlock->cb_nFlags |= CBF_BUSY;
			psBlock->cb_nDevice = nDev;
			psBlock->cb_nBlockNum = nBlockNum + i;
			bc_hash_insert( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum + i, psBlock );

			if ( psBlock->cb_nBlockNum == nReqBlock )
			{
				psBlock->cb_nRefCount++;
				psBuffer = psBlock;
				bc_add_to_head( &g_sBlockCache.bc_sLocked, psBuffer );
			}
			else
			{
				bc_add_to_head( &g_sBlockCache.bc_sNormal, psBlock );
			}
		}

		UNLOCK( g_sBlockCache.bc_hLock );
		read_block_list( nDev, nBlockNum, apsBlockList, nNumNeeded, nBlockSize );
		LOCK( g_sBlockCache.bc_hLock );

		for ( count_t i = 0; i < nListSize; ++i )
		{
			CacheBlock_s *psBlock = apsBlockList[i];

			psBlock->cb_nFlags &= ~CBF_BUSY;
		}
		if ( psBuffer == NULL )
		{
			panic( "get_cache_block() requested block not loaded!! (S:%d %Ld - %Ld)\n", nListSize, nBlockNum, nReqBlock );
		}
		nListSize = 0;
	}

	// If someone else loaded the block while we flushed old blocks
	// we might end up not using the already allocated blocks.
	// We then must get rid of them here

	for ( count_t i = 0; i < nListSize; ++i )
	{
		free_cache_block( apsBlockList[i] );
	}

	g_nLastAccess = get_system_time();

      error:
	UNLOCK( g_sBlockCache.bc_hLock );

	if ( psBuffer != NULL )
	{
		CURRENT_THREAD->tr_nNumLockedCacheBlocks++;
	}
	return ( ( NULL != psBuffer ) ? CB_DATA( psBuffer ) : NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t mark_blocks_dirty( dev_t nDev, off_t nBlockNum, size_t nBlockCount )
{
	CacheBlock_s *psBuffer = NULL;
	status_t nError = 0;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: mark_blocks_dirty() invalid device %d\n", nDev );
		nError = -EINVAL;
		goto error;
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: mark_blocks_dirty() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		nError = -EINVAL;
		goto error;
	}

	for ( count_t i = 0; i < nBlockCount; ++i )
	{
		psBuffer = lookup_block( nDev, nBlockNum + i );

		if ( NULL != psBuffer )
		{
			if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
			{
				atomic_inc( &g_sBlockCache.bc_nDirtyCount );
				atomic_add( &g_sSysBase.ex_nDirtyCacheSize, CB_SIZE( psBuffer ) );
			}
			psBuffer->cb_nFlags |= CBF_DIRTY;
		}
		else
		{
			printk( "Error: mark_blocks_dirty() failed to lookup block %Ld at device %d\n", nBlockNum, nDev );
			nError = -EINVAL;
		}
	}
	g_nLastAccess = get_system_time();
      error:
	unlock_semaphore( g_sBlockCache.bc_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Register a callback with a set of cache blocks.
 *	The blocks are marked dirty, and have their reference counts
 *	increased by one.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t set_blocks_info( dev_t nDev, off_t *panBlocks, size_t nCount,
			  bool bDoClone  __attribute__ ((unused)),
			  cache_callback *pFunc, void *pArg )
{
	CacheBlock_s *psBuffer;
	status_t nError = 0;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: set_blocks_info() invalid device %d\n", nDev );
		nError = -EINVAL;
		goto error;
	}

	for ( count_t i = 0; i < nCount; ++i )
	{
		cache_callback *pOldFunc;
		void *pOldArg;
		off_t nOldBlock;

		if ( panBlocks[i] >= g_asDevices[nDev].de_nBlockCount )
		{
			printk( "Error: set_blocks_info() request block %Ld outside disk (%Ld)\n", panBlocks[i], g_asDevices[nDev].de_nBlockCount );
			nError = -EINVAL;
			break;
		}

		psBuffer = hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, panBlocks[i] );

		if ( NULL == psBuffer )
		{
			printk( "Panic: set_blocks_info() failed to lookup cache block\n" );
			nError = -ENOENT;
			break;
		}
		if ( psBuffer->cb_nRefCount == 0 )
		{
			printk( "Error: set_blocks_info() block %d not locked!\n", ( int )panBlocks[i] );
		}

		if ( psBuffer->cb_nFlags & CBF_BUSY )
		{
			panic( "set_blocks_info() found a busy block with reference count of %d\n", psBuffer->cb_nRefCount );
		}

		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			atomic_inc( &g_sBlockCache.bc_nDirtyCount );
			atomic_add( &g_sSysBase.ex_nDirtyCacheSize, CB_SIZE( psBuffer ) );
		}

		psBuffer->cb_nFlags |= CBF_DIRTY;

		pOldFunc = psBuffer->cb_pFunc;
		pOldArg = psBuffer->cb_pArg;
		nOldBlock = psBuffer->cb_nBlockNum;

		psBuffer->cb_pFunc = pFunc;
		psBuffer->cb_pArg = pArg;

		if ( NULL != pOldFunc )
		{
			printk( "Error: set_blocks_info() block already has a callback\n" );
			pOldFunc( nOldBlock, 1, pOldArg );
		}
	}
	g_nLastAccess = get_system_time();
      error:
	unlock_semaphore( g_sBlockCache.bc_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t write_logged_blocks( dev_t nDev, off_t nBlockNum, const void *pBuffer, count_t nCount, size_t nBlockSize, cache_callback *pFunc, void *pArg )
{
	status_t nError = 0;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	for ( count_t i = 0; i < nCount; ++i )
	{
		CacheBlock_s *psBlock = do_get_empty_block( nDev, nBlockNum + i, nBlockSize );

		if ( NULL != psBlock )
		{
			memcpy( CB_DATA( psBlock ), ( ( char * )pBuffer ) + i * nBlockSize, nBlockSize );
			if ( ( psBlock->cb_nFlags & CBF_DIRTY ) == 0 )
			{
				atomic_inc( &g_sBlockCache.bc_nDirtyCount );
				atomic_add( &g_sSysBase.ex_nDirtyCacheSize, nBlockSize );
				psBlock->cb_nFlags |= CBF_DIRTY;
			}
			if ( psBlock->cb_pFunc != NULL )
			{
				panic( "write_logged_blocks() buffer already has a callback registered!\n" );
			}
			psBlock->cb_pFunc = pFunc;
			psBlock->cb_pArg = pArg;

			kassertw( psBlock->cb_nRefCount > 0 );
			psBlock->cb_nRefCount--;
			bc_remove_from_list( &g_sBlockCache.bc_sLocked, psBlock );
			if ( 0 == psBlock->cb_nRefCount )
			{
				bc_add_to_head( &g_sBlockCache.bc_sNormal, psBlock );
			}
			else
			{
				bc_add_to_head( &g_sBlockCache.bc_sLocked, psBlock );
			}
		}
		else
		{
			panic( "write_logged_blocks() failed to find block %d\n", ( int )( nBlockNum + i ) );
			nError = -EIO;
			break;
		}
	}
	unlock_semaphore( g_sBlockCache.bc_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void release_cache_block( dev_t nDev, off_t nBlockNum )
{
	CacheBlock_s *psBuffer;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: release_cache_block() invalid device %d\n", nDev );
		goto error;
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: release_cache_block() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		goto error;
	}

	psBuffer = lookup_block( nDev, nBlockNum );

	kassertw( NULL != psBuffer );

	if ( NULL != psBuffer )
	{
		kassertw( psBuffer->cb_nRefCount > 0 );
		psBuffer->cb_nRefCount--;
		CURRENT_THREAD->tr_nNumLockedCacheBlocks--;

		bc_remove_from_list( &g_sBlockCache.bc_sLocked, psBuffer );
		if ( 0 == psBuffer->cb_nRefCount )
		{
			bc_add_to_head( &g_sBlockCache.bc_sNormal, psBuffer );
		}
		else
		{
			bc_add_to_head( &g_sBlockCache.bc_sLocked, psBuffer );
		}
	}
	g_nLastAccess = get_system_time();
      error:
	unlock_semaphore( g_sBlockCache.bc_hLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t read_phys_blocks( dev_t nDev, off_t nBlockNum, void *pBuffer,
			  uint_fast32_t nBlockCount, size_t nBlockSize )
{
	ssize_t nError = 0;

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: read_phys_blocks() invalid device %d\n", nDev );
		return ( -EINVAL );
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: read_phys_blocks() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		return ( -EINVAL );
	}

	nError = read_pos( nDev, nBlockNum * nBlockSize, pBuffer, nBlockCount * nBlockSize );

	if ( nError >= 0 )
	{
		LOCK( g_sBlockCache.bc_hLock );
		for ( count_t i = 0; i < nBlockCount; ++i )
		{
			CacheBlock_s *psBlock = lookup_block( nDev, nBlockNum + i );

			if ( psBlock != NULL )
			{
				if ( psBlock->cb_pFunc != NULL )
				{
					printk( "Error: read_phys_blocks(%d,%d) block %d has a registered callback!\n", ( int )nBlockNum, nBlockCount, ( int )( nBlockNum + i ) );
				}
				memcpy( ( ( uint8_t * )pBuffer ) + i * nBlockSize, CB_DATA( psBlock ), nBlockSize );
			}
		}
		UNLOCK( g_sBlockCache.bc_hLock );
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t write_phys_blocks( dev_t nDev, off_t nBlockNum, const void *pBuffer,
			   count_t nBlockCount, size_t nBlockSize )
{
	ssize_t nError;

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: write_phys_blocks() invalid device %d\n", nDev );
		return ( -EINVAL );
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: write_phys_blocks() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		return ( -EINVAL );
	}

	nError = write_pos( nDev, nBlockNum * nBlockSize, pBuffer, nBlockCount * nBlockSize );

	if ( nError >= 0 )
	{
		LOCK( g_sBlockCache.bc_hLock );
		for ( count_t i = 0; i < nBlockCount; ++i )
		{
			CacheBlock_s *psBlock = lookup_block( nDev, nBlockNum + i );

			if ( psBlock != NULL )
			{
				if ( psBlock->cb_pFunc != NULL )
				{
					printk( "Error: write_phys_blocks(%d,%d) block %d has a registered callback!\n", ( int )nBlockNum, nBlockCount, ( int )( nBlockNum + i ) );
				}
				memcpy( CB_DATA( psBlock ), ( ( uint8_t * )pBuffer ) + i * nBlockSize, nBlockSize );

				if ( psBlock->cb_nFlags & CBF_DIRTY )
				{
					atomic_dec( &g_sBlockCache.bc_nDirtyCount );
					atomic_add( &g_sSysBase.ex_nDirtyCacheSize, nBlockSize );
					psBlock->cb_nFlags &= ~CBF_DIRTY;
				}
			}
		}
		UNLOCK( g_sBlockCache.bc_hLock );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t cached_read( dev_t nDev, off_t nBlockNum, void *pBuffer,
		      count_t nBlockCount, size_t nBlockSize )
{
	status_t nError = 0;

	for ( count_t i = 0; i < nBlockCount; ++i )
	{
		void *pBlock = get_cache_block( nDev, nBlockNum + i, nBlockSize );

		if ( NULL != pBlock )
		{
			memcpy( ( ( char * )pBuffer ) + i * nBlockSize, pBlock, nBlockSize );
			release_cache_block( nDev, nBlockNum + i );
		}
		else
		{
			printk( "cached_read() failed to find block %d\n", ( int )( nBlockNum + i ) );
			nError = -EIO;
			break;
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

status_t cached_write( dev_t nDev, off_t nBlockNum, const void *pBuffer,
		       count_t nBlockCount, size_t nBlockSize )
{
	status_t nError = 0;

	LOCK( g_sBlockCache.bc_hLock );

	for ( count_t i = 0; i < nBlockCount; ++i )
	{
		CacheBlock_s *psBlock = do_get_empty_block( nDev, nBlockNum + i, nBlockSize );

		if ( NULL != psBlock )
		{
			// The memcopy might cause a pagefault that require
			// memory to be allocated and then might cause the cache
			// to be flushed. Therefor we need to unlock before performing
			// the copy.
			UNLOCK( g_sBlockCache.bc_hLock );
			memcpy( CB_DATA( psBlock ), ( ( char * )pBuffer ) + i * nBlockSize, nBlockSize );
			LOCK( g_sBlockCache.bc_hLock );
			if ( ( psBlock->cb_nFlags & CBF_DIRTY ) == 0 )
			{
				atomic_inc( &g_sBlockCache.bc_nDirtyCount );
				atomic_add( &g_sSysBase.ex_nDirtyCacheSize, nBlockSize );
				psBlock->cb_nFlags |= CBF_DIRTY;
			}
			kassertw( psBlock->cb_nRefCount > 0 );
			psBlock->cb_nRefCount--;
			bc_remove_from_list( &g_sBlockCache.bc_sLocked, psBlock );
			if ( 0 == psBlock->cb_nRefCount )
			{
				bc_add_to_head( &g_sBlockCache.bc_sNormal, psBlock );
			}
			else
			{
				bc_add_to_head( &g_sBlockCache.bc_sLocked, psBlock );
			}
		}
		else
		{
			printk( "cached_write() failed to find block %d\n", ( int )( nBlockNum + i ) );
			nError = -EIO;
			break;
		}
	}
	UNLOCK( g_sBlockCache.bc_hLock );
	return ( nError );
}

static status_t do_flush_device_cache( dev_t nDevice, bool bOnlyLoggedBlocks, bool bFlushLocked )
{
	CacheBlock_s *pasIoList[BC_FLUSH_SIZE];
	CacheBlock_s *psBuffer;
	count_t i = 0;

	if ( nDevice < 0 || nDevice >= MAX_DEVICES || g_asDevices[nDevice].de_nBlockCount == 0 )
	{
		printk( "Error: flush_device_cache() invalid device %d\n", nDevice );
		return ( -EINVAL );
	}

	LOCK( g_sBlockCache.bc_hLock );

	for ( psBuffer = ( bFlushLocked ? g_sBlockCache.bc_sLocked .cl_psLRU : g_sBlockCache.bc_sNormal.cl_psLRU ); NULL != psBuffer; psBuffer = psBuffer->cb_psNext )
	{
		if( !bFlushLocked )
			kassertw( 0 == psBuffer->cb_nRefCount );

		if ( psBuffer->cb_nDevice != nDevice )
		{
			continue;
		}
		if ( psBuffer->cb_nFlags & CBF_BUSY )
		{
			continue;
		}
		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			continue;
		}
		if ( bOnlyLoggedBlocks && psBuffer->cb_pFunc == NULL )
		{
			continue;
		}

		psBuffer->cb_nFlags |= CBF_BUSY;

		if ( i >= BC_FLUSH_SIZE )
		{
			qsort( pasIoList, BC_FLUSH_SIZE, sizeof( CacheBlock_s * ), cmp_blocks );
			UNLOCK( g_sBlockCache.bc_hLock );
			flush_block_list( pasIoList, BC_FLUSH_SIZE, bFlushLocked );
			LOCK( g_sBlockCache.bc_hLock );	// No need to restart the iteration. The BUSY flag ensure the current node is not deleted.
			for ( count_t j = 0; j < BC_FLUSH_SIZE; ++j )
			{
				pasIoList[j]->cb_nFlags &= ~CBF_BUSY;
			}
			i = 0;
		}
		pasIoList[i++] = psBuffer;
	}
	if ( i > 0 )
	{
		qsort( pasIoList, i, sizeof( CacheBlock_s * ), cmp_blocks );
		UNLOCK( g_sBlockCache.bc_hLock );
		flush_block_list( pasIoList, i, bFlushLocked );
		LOCK( g_sBlockCache.bc_hLock );
		for ( count_t j = 0; j < i; ++j )
		{
			pasIoList[j]->cb_nFlags &= ~CBF_BUSY;
		}

	}
	UNLOCK( g_sBlockCache.bc_hLock );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t flush_device_cache( dev_t nDevice, bool bOnlyLoggedBlocks )
{
	return( do_flush_device_cache( nDevice, bOnlyLoggedBlocks, false ) );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t flush_locked_device_cache( dev_t nDevice, bool bOnlyLoggedBlocks )
{
	return( do_flush_device_cache( nDevice, bOnlyLoggedBlocks, true ) );
}

status_t flush_cache_block( dev_t nDev, off_t nBlockNum )
{
	CacheBlock_s *pasIoList[BC_FLUSH_SIZE];
	CacheBlock_s *psBuffer;
	count_t nCount = 1;

	if ( nDev < 0 || nDev >= MAX_DEVICES || g_asDevices[nDev].de_nBlockCount == 0 )
	{
		printk( "Error: flush_cache_block() invalid device %d\n", nDev );
		return ( -EINVAL );
	}
	if ( nBlockNum >= g_asDevices[nDev].de_nBlockCount )
	{
		printk( "Error: flush_cache_block() request block %Ld outside disk (%Ld)\n", nBlockNum, g_asDevices[nDev].de_nBlockCount );
		return ( -EINVAL );
	}

	LOCK( g_sBlockCache.bc_hLock );

	psBuffer = lookup_block( nDev, nBlockNum );

	if ( psBuffer == NULL )
	{
		goto done;
	}
	if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
	{
		goto done;
	}
	psBuffer->cb_nFlags |= CBF_BUSY;

	for ( count_t i = 1; nCount < BC_FLUSH_SIZE; ++i )
	{
		psBuffer = lookup_block( nDev, nBlockNum + i );
		if ( psBuffer == NULL || ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			break;
		}
		psBuffer->cb_nFlags |= CBF_BUSY;
		nCount++;
	}
	while ( nCount < BC_FLUSH_SIZE )
	{
		psBuffer = lookup_block( nDev, nBlockNum - 1 );
		if ( psBuffer == NULL || ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			break;
		}
		psBuffer->cb_nFlags |= CBF_BUSY;
		nCount++;
		nBlockNum--;
	}
	for ( count_t i = 0; i < nCount; ++i )
	{
		pasIoList[i] = hash_lookup( &g_sBlockCache.bc_sHashTab, nDev, nBlockNum + i );
		if ( pasIoList[i] == NULL )
		{
			panic( "flush_cache_block() failed to lookup block %d:%Ld\n", nDev, nBlockNum + i );
			goto done;
		}
	}
	UNLOCK( g_sBlockCache.bc_hLock );
	flush_block_list( pasIoList, nCount, false );
	LOCK( g_sBlockCache.bc_hLock );
	for ( count_t i = 0; i < nCount; ++i )
	{
		pasIoList[i]->cb_nFlags &= ~CBF_BUSY;
	}
      done:
	UNLOCK( g_sBlockCache.bc_hLock );
	return ( 0 );
}

status_t setup_device_cache( dev_t nDevice, fs_id nFS, off_t nBlockCount )
{
	struct stat sStat;

	if ( nDevice < 0 || nDevice >= MAX_DEVICES )
	{
		printk( "Error: setup_device_cache() invalid device %d\n", nDevice );
		return ( -EINVAL );
	}
	if ( g_asDevices[nDevice].de_nBlockCount != 0 )
	{
		return ( -EBUSY );
	}
	status_t nError = fstat( nDevice, &sStat );
	if ( nError < 0 )
	{
		return ( nError );
	}

	LOCK( g_sBlockCache.bc_hLock );
	for ( count_t i = 0; i < MAX_DEVICES; ++i )
	{
		if ( g_asDevices[i].de_nBlockCount == 0 )
		{
			continue;
		}
		if ( g_asDevices[i].de_nDeviceID == sStat.st_dev &&
		     g_asDevices[i].de_nDeviceInode == sStat.st_ino )
		{
			nError = -EBUSY;
			goto error;
		}
	}
	g_asDevices[nDevice].de_nBlockCount = nBlockCount;
	g_asDevices[nDevice].de_nFilesystem = nFS;
	g_asDevices[nDevice].de_nDeviceID = sStat.st_dev;
	g_asDevices[nDevice].de_nDeviceInode = sStat.st_ino;
	nError = 0;
      error:
	UNLOCK( g_sBlockCache.bc_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t shutdown_device_cache( dev_t nDevice )
{
	CacheBlock_s *psBuffer;
	bool bDoRetry = false;

	if ( nDevice < 0 || nDevice >= MAX_DEVICES || g_asDevices[nDevice].de_nBlockCount == 0 )
	{
		printk( "Error: shutdown_device_cache() invalid device %d\n", nDevice );
		return ( -EINVAL );
	}

      retry:
	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	for ( psBuffer = g_sBlockCache.bc_sNormal.cl_psLRU; NULL != psBuffer; )
	{
		CacheBlock_s *psTmp;

		kassertw( 0 == psBuffer->cb_nRefCount );

		if ( psBuffer->cb_nDevice != nDevice )
		{
			psBuffer = psBuffer->cb_psNext;
			continue;
		}
		if ( psBuffer->cb_nFlags & CBF_BUSY )
		{
			printk( "PANIC: shutdown_device_cache() found busy cache block\n" );
			bDoRetry = true;
			psBuffer = psBuffer->cb_psNext;
			continue;
		}

		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) != 0 )
		{
			printk( "PANIC: shutdown_device_cache() found dirty cache block\n" );
			bDoRetry = true;
			psBuffer = psBuffer->cb_psNext;
			continue;
		}
		psTmp = psBuffer->cb_psNext;
		bc_remove_from_list( &g_sBlockCache.bc_sNormal, psBuffer );
		kassertw( bc_hash_delete( &g_sBlockCache.bc_sHashTab, nDevice, psBuffer->cb_nBlockNum ) == psBuffer );
		free_cache_block( psBuffer );
		psBuffer = psTmp;
	}

	for ( psBuffer = g_sBlockCache.bc_sLocked.cl_psLRU; NULL != psBuffer; psBuffer = psBuffer->cb_psNext )
	{
		if ( psBuffer->cb_nDevice == nDevice )
		{
			printk( "PANIC: shutdown_device_cache() block %d on device %d are locked!!\n", ( int )psBuffer->cb_nBlockNum, nDevice );
			bDoRetry = true;
			continue;
		}
	}
	g_asDevices[nDevice].de_nBlockCount = 0;
	unlock_semaphore( g_sBlockCache.bc_hLock );

	if ( bDoRetry )
	{
		bDoRetry = false;
		snooze( 10000 );
		flush_device_cache( nDevice, false );
		goto retry;
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void flush_block_cache( void )
{
	CacheBlock_s *pasIoList[BC_FLUSH_SIZE];
	CacheBlock_s *psBuffer;
	count_t i = 0;

	lock_semaphore( g_sBlockCache.bc_hLock, SEM_NOSIG, INFINITE_TIMEOUT );

	for ( psBuffer = g_sBlockCache.bc_sNormal.cl_psLRU; NULL != psBuffer; psBuffer = psBuffer->cb_psNext )
	{
		kassertw( 0 == psBuffer->cb_nRefCount );

		if ( psBuffer->cb_nFlags & CBF_BUSY )
		{
			continue;
		}

		if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
		{
			continue;
		}
		psBuffer->cb_nFlags |= CBF_BUSY;

		if ( i >= BC_FLUSH_SIZE )
		{
			qsort( pasIoList, BC_FLUSH_SIZE, sizeof( CacheBlock_s * ), cmp_blocks );
			UNLOCK( g_sBlockCache.bc_hLock );
			flush_block_list( pasIoList, BC_FLUSH_SIZE, false );
			LOCK( g_sBlockCache.bc_hLock );	// No need to restart the iteration. The BUSY flag ensure the current node is not deleted.
			for ( count_t j = 0; j < BC_FLUSH_SIZE; ++j )
			{
				pasIoList[j]->cb_nFlags &= ~CBF_BUSY;
			}

			i = 0;
		}
		pasIoList[i++] = psBuffer;
	}
	if ( i > 0 )
	{
		qsort( pasIoList, i, sizeof( CacheBlock_s * ), cmp_blocks );
		UNLOCK( g_sBlockCache.bc_hLock );
		flush_block_list( pasIoList, i, false );
		LOCK( g_sBlockCache.bc_hLock );

		for ( count_t j = 0; j < i; ++j )
		{
			pasIoList[j]->cb_nFlags &= ~CBF_BUSY;
		}
	}
	UNLOCK( g_sBlockCache.bc_hLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t cache_flusher( void *pData )  __attribute__ ((noreturn)) ;
static status_t cache_flusher( void *pData  __attribute__ ((unused)) )
{
	CacheBlock_s *psBuffer;
	bigtime_t nLastFlushTime = get_system_time();

	for ( ;; )
	{
		CacheBlock_s *pasIoList[BC_FLUSH_SIZE];
		count_t i = 0;
		bigtime_t nCurTime;
		dev_t nDev = -1;

		if ( atomic_read( &g_sSysBase.ex_nDirtyCacheSize ) > 0 || atomic_read( &g_sSysBase.ex_nFreePageCount ) < 4096 )
		{
			snooze( 500000 );
		}
		else
		{
			snooze( 2000000 );
		}
		if ( atomic_read( &g_sSysBase.ex_nDirtyCacheSize ) == 0 && atomic_read( &g_sSysBase.ex_nFreePageCount ) >= 4096 )
		{
			continue;
		}
		nCurTime = get_system_time();

//      if ( (nCurTime < nLastFlushTime + 5000000) && (g_nLastAccess > nCurTime - 1000000) ) {
//          continue;
//      }
		nLastFlushTime = nCurTime;

		LOCK( g_sBlockCache.bc_hLock );
		for ( psBuffer = g_sBlockCache.bc_sNormal.cl_psLRU; NULL != psBuffer; psBuffer = psBuffer->cb_psNext )
		{
			kassertw( 0 == psBuffer->cb_nRefCount );

			if ( psBuffer->cb_nFlags & CBF_BUSY )
			{
				continue;
			}
			if ( ( psBuffer->cb_nFlags & CBF_DIRTY ) == 0 )
			{
				continue;
			}
			// Make sure we don't lock buffers from two different devices at the same time
			// as that will cause deadlock if one of the devices is actualy a file inside
			// the other.
			if ( nDev != -1 && psBuffer->cb_nDevice != nDev )
			{
				continue;
			}
			nDev = psBuffer->cb_nDevice;

			psBuffer->cb_nFlags |= CBF_BUSY;
			pasIoList[i++] = psBuffer;
			if ( i >= BC_FLUSH_SIZE )
			{
				break;
			}
		}
		if ( i > 0 )
		{
			qsort( pasIoList, i, sizeof( CacheBlock_s * ), cmp_blocks );

			UNLOCK( g_sBlockCache.bc_hLock );
			flush_block_list( pasIoList, i, false );
			LOCK( g_sBlockCache.bc_hLock );
			for ( count_t j = 0; j < i; ++j )
			{
				pasIoList[j]->cb_nFlags &= ~CBF_BUSY;
			}
		}
		if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) < 4096 )
		{
			release_cache_blocks( -1 );
			shrink_cache_heaps( -1 );
		}
		UNLOCK( g_sBlockCache.bc_hLock );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t shutdown_block_cache( void )
{
	CacheBlock_s *psBuffer;

	flush_block_cache();

	for ( psBuffer = g_sBlockCache.bc_sLocked.cl_psLRU; NULL != psBuffer; psBuffer = psBuffer->cb_psNext )
	{
		printk( "PANIC : shutdown_block_cache() called while block %d is still locked!!!\n", ( int )psBuffer->cb_nBlockNum );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_block_cache( void )
{
	thread_id hFlusherThread;

	memset( &g_sBlockCache, 0, sizeof( BlockCache_s ) );

	init_hash_table( &g_sBlockCache.bc_sHashTab );

	g_sBlockCache.bc_hLock = create_semaphore( "bcache_lock", 1, SEM_WARN_DBL_LOCK );

	hFlusherThread = spawn_kernel_thread( "bcache_flusher", cache_flusher, 0, DEFAULT_KERNEL_STACK, NULL );
	if ( hFlusherThread >= 0 )
	{
		wakeup_thread( hFlusherThread, false );
	}
}
