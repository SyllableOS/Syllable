
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

#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/semaphore.h>
#include <posix/errno.h>

#include <macros.h>

#include "inc/sysbase.h"
#include "inc/bcache.h"

extern BlockCache_s g_sBlockCache;


typedef struct _CacheHeap CacheHeap_s;
struct _CacheHeap
{
	area_id ch_hAreaID;
	vuint32 ch_nSize;
	void *ch_pAddress;
	CacheBlock_s *volatile ch_psFirstFreeBlock;
	uint32 ch_nBlockSize;
	vuint32 ch_nUsedBlocks;
	volatile bool ch_bBusy;
};

static CacheHeap_s g_asCacheHeaps[MAX_CACHE_BLK_SIZE_ORDERS];

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static int get_block_order( int nSize )
{
	int nOrder;

	for ( nOrder = 0; nOrder < MAX_CACHE_BLK_SIZE_ORDERS; nOrder++ )
	{
		if ( 1 << nOrder == nSize )
		{
			break;
		}
	}
	if ( nOrder == MAX_CACHE_BLK_SIZE_ORDERS )
	{
		panic( "get_block_order() Invalid block size: %d\n", nSize );
		return ( 0 );
	}
	return ( nOrder );
}

static int expand_heap( CacheHeap_s *psHeap )
{
	const uint32 nBlockSize = sizeof( CacheBlock_s ) + psHeap->ch_nBlockSize;
	uint32 nNewSize;
	uint32 nOldCount;
	uint32 nNewBlocks;
	CacheBlock_s *psBlock;
	int i;
	int nError;

	while ( psHeap->ch_bBusy )
	{
		UNLOCK( g_sBlockCache.bc_hLock );
		snooze( 10000 );
		LOCK( g_sBlockCache.bc_hLock );
	}

	nNewSize = psHeap->ch_nSize + PAGE_SIZE * 32;
	nOldCount = psHeap->ch_nSize / nBlockSize;
	nNewBlocks = nNewSize / nBlockSize - nOldCount;
	psBlock = ( CacheBlock_s * )( ( ( uint8 * )psHeap->ch_pAddress ) + nBlockSize * nOldCount );

	psHeap->ch_bBusy = true;

	UNLOCK( g_sBlockCache.bc_hLock );
	nError = resize_area( psHeap->ch_hAreaID, nNewSize, false );

	LOCK( g_sBlockCache.bc_hLock );
	psHeap->ch_bBusy = false;

	if ( nError < 0 )
	{
//      printk( "expand_heap() failed to resize area from %ld to %ld bytes\n", psHeap->ch_nSize, nNewSize );
		return ( -ENOMEM );
	}
	psHeap->ch_nSize = nNewSize;

	for ( i = 0; i < nNewBlocks; ++i )
	{
		psBlock->cb_psNext = psHeap->ch_psFirstFreeBlock;
		psHeap->ch_psFirstFreeBlock = psBlock;

		psBlock->cb_nFlags = psHeap->ch_nBlockSize;

		psBlock = ( CacheBlock_s * )( ( ( uint8 * )psBlock ) + nBlockSize );
	}
	return ( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static void init_heap( CacheHeap_s *psHeap )
{
	char *pPtr;
	uint32 nBufSize;
	uint32 nMaxBufSize = g_sSysBase.ex_nTotalPageCount * PAGE_SIZE;
	int nCnt = 0;

	if ( nMaxBufSize > 1024 * 1024 * 768 )
	{
		nMaxBufSize = 1024 * 1024 * 768;
	}
      retry:
	psHeap->ch_nSize = 4096 * 10;
	psHeap->ch_nUsedBlocks = 0;

	psHeap->ch_hAreaID = create_area( "bcache_1024", &psHeap->ch_pAddress, psHeap->ch_nSize, nMaxBufSize, AREA_FULL_ACCESS | AREA_KERNEL | AREA_UNMAP_PHYS, AREA_FULL_LOCK );
	if ( psHeap->ch_hAreaID < 0 )
	{
		if ( psHeap->ch_hAreaID == -ENOADDRSPC && nMaxBufSize > psHeap->ch_nSize * 2 )
		{
			nMaxBufSize >>= 1;
			goto retry;
		}
		panic( "init_heap() failed to create area for %d size blocks\n", psHeap->ch_nBlockSize );
	}
	pPtr = psHeap->ch_pAddress;
	nBufSize = psHeap->ch_nSize;

	while ( nBufSize >= sizeof( CacheBlock_s ) + psHeap->ch_nBlockSize )
	{
		CacheBlock_s *psBlock = ( CacheBlock_s * )pPtr;

		psBlock->cb_psNext = psHeap->ch_psFirstFreeBlock;
		psHeap->ch_psFirstFreeBlock = psBlock;

		psBlock->cb_nFlags = psHeap->ch_nBlockSize;

		pPtr += sizeof( CacheBlock_s ) + psHeap->ch_nBlockSize;
		nBufSize -= sizeof( CacheBlock_s ) + psHeap->ch_nBlockSize;
		nCnt++;
	}
}

int alloc_cache_blocks( CacheBlock_s **apsBlocks, int nCount, int nBlockSize, bool *pbDidBlock )
{
	int nOrder = get_block_order( nBlockSize );
	CacheHeap_s *psHeap = &g_asCacheHeaps[nOrder];
	CacheBlock_s *psBlock;
	int nRealBlockSize;
	int nMaxBlocks;
	int nFreeBlocks;
	int nAllocCount = 0;
	int i;

	*pbDidBlock = false;

	if ( psHeap->ch_pAddress == NULL )
	{
		psHeap->ch_nBlockSize = nBlockSize;
		init_heap( psHeap );
	}

      again:
	nRealBlockSize = psHeap->ch_nBlockSize + sizeof( CacheBlock_s );
	nMaxBlocks = psHeap->ch_nSize / nRealBlockSize;
	nFreeBlocks = nMaxBlocks - psHeap->ch_nUsedBlocks;

	if ( nFreeBlocks < nCount && atomic_read( &g_sSysBase.ex_nFreePageCount ) > 1024 )
	{
		expand_heap( psHeap );
		nMaxBlocks = psHeap->ch_nSize / nRealBlockSize;
		nFreeBlocks = nMaxBlocks - psHeap->ch_nUsedBlocks;
		*pbDidBlock = true;
	}

	if ( nFreeBlocks < nCount )
	{
		release_cache_blocks();
		shrink_cache_heaps( nOrder );
		*pbDidBlock = true;
		nMaxBlocks = psHeap->ch_nSize / nRealBlockSize;
		nFreeBlocks = nMaxBlocks - psHeap->ch_nUsedBlocks;
	}

	if ( psHeap->ch_psFirstFreeBlock == NULL )
	{
		goto again;
		return ( 0 );
	}

	for ( i = 0; i < nCount; ++i )
	{
		psBlock = psHeap->ch_psFirstFreeBlock;
		if ( psBlock == NULL )
		{
			break;
		}
		psBlock->cb_nDevice = -1;
		psBlock->cb_nFlags = nBlockSize | CBF_USED;
		psBlock->cb_nRefCount = 0;
		psBlock->cb_pFunc = NULL;
		psHeap->ch_psFirstFreeBlock = psBlock->cb_psNext;
		psHeap->ch_nUsedBlocks++;
		psBlock->cb_psNext = NULL;
		psBlock->cb_psPrev = NULL;

		g_sBlockCache.bc_nMemUsage += nBlockSize;
		atomic_add( &g_sSysBase.ex_nBlockCacheSize, nBlockSize );
		apsBlocks[nAllocCount++] = psBlock;
	}
	return ( nAllocCount );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void free_cache_block( CacheBlock_s *psBlock )
{
	int nBlockSize = CB_SIZE( psBlock );
	int nOrder = get_block_order( nBlockSize );
	CacheHeap_s *psHeap = &g_asCacheHeaps[nOrder];

	atomic_sub( &g_sSysBase.ex_nBlockCacheSize, nBlockSize );
	g_sBlockCache.bc_nMemUsage -= nBlockSize;

	psBlock->cb_nFlags &= CBF_SIZE_MASK;
	psBlock->cb_psNext = psHeap->ch_psFirstFreeBlock;
	psHeap->ch_psFirstFreeBlock = psBlock;
	psHeap->ch_nUsedBlocks--;
}


ssize_t shrink_cache_heaps( int nIgnoredOrder )
{
	int i;
	count_t nBytesFreed = 0;
	bool bBusy = false;

	for ( i = 0; i < MAX_CACHE_BLK_SIZE_ORDERS; ++i )
	{
		CacheHeap_s *psHeap = &g_asCacheHeaps[i];

		if ( i == nIgnoredOrder )
		{
			continue;
		}
	      again:
		if ( psHeap->ch_bBusy )
		{
			snooze( 10 );	// Make sure we don't starve the thread holding the lock
			bBusy = true;
			continue;
		}
		psHeap->ch_bBusy = true;
		if ( psHeap->ch_pAddress != NULL && psHeap->ch_psFirstFreeBlock != NULL )
		{
			const int nBlockSize = psHeap->ch_nBlockSize + sizeof( CacheBlock_s );
			int nMaxBlocks = psHeap->ch_nSize / nBlockSize;
			CacheBlock_s *psBlock;
			CacheBlock_s *psPrev = NULL;
			CacheBlock_s *psFirstLowBlock = NULL;
			const uint32 nNewSize = ( psHeap->ch_nUsedBlocks * nBlockSize + PAGE_SIZE - 1 ) & PAGE_MASK;
			const uint32 nNewCount = nNewSize / nBlockSize;
			const int nBlocksToRemove = nMaxBlocks - nNewCount;
			int j;

			if ( nNewSize == psHeap->ch_nSize )
			{
				psHeap->ch_bBusy = false;
				continue;
			}

			// Wait for locked blocks above the new size to be released
			psBlock = ( CacheBlock_s * )( ( ( uint8 * )psHeap->ch_pAddress ) + nNewCount * nBlockSize );
			for ( j = 0; j < nBlocksToRemove; ++j )
			{
				if ( ( ( psBlock->cb_nFlags & CBF_USED ) && psBlock->cb_nRefCount > 0 ) || ( psBlock->cb_nFlags & CBF_BUSY ) )
				{
//                  printk( "shrink_cache_heaps() Wait for block %p to be released (%d)\n", psBlock, psBlock->cb_nRefCount );
					psHeap->ch_bBusy = false;
					UNLOCK( g_sBlockCache.bc_hLock );
					snooze( 10000 );
					LOCK( g_sBlockCache.bc_hLock );
					// We has to start all over after the unlocking
					goto again;
				}
				psBlock = ( CacheBlock_s * )( ( ( uint8 * )psBlock ) + nBlockSize );
			}
			// Filter out all free blocks below the new area size.

			for ( psBlock = psHeap->ch_psFirstFreeBlock; psBlock != NULL; /*psBlock = psPrev->cb_psNext */  )
			{
				CacheBlock_s *psNext = psBlock->cb_psNext;
				int nBlockNum = ( ( ( uint8 * )psBlock ) - ( ( uint8 * )psHeap->ch_pAddress ) ) / nBlockSize;

				if ( nBlockNum < nNewCount )
				{
					if ( psPrev == NULL )
					{
						psHeap->ch_psFirstFreeBlock = psBlock->cb_psNext;
					}
					else
					{
						psPrev->cb_psNext = psBlock->cb_psNext;
					}
					psBlock->cb_psNext = psFirstLowBlock;
					psFirstLowBlock = psBlock;
				}
				else
				{
					psPrev = psBlock;
				}
				psBlock = psNext;
			}
			// Copy used block abow the new area size into free blocks below
			psBlock = ( CacheBlock_s * )( ( ( uint8 * )psHeap->ch_pAddress ) + nNewCount * nBlockSize );
			for ( j = 0; j < nBlocksToRemove; ++j )
			{
				if ( psBlock->cb_nFlags & CBF_USED )
				{
					CacheEntryList_s *psList;
					CacheBlock_s *psFreeBlock = psFirstLowBlock;

					int nBlockNum = ( ( ( uint8 * )psBlock ) - ( ( uint8 * )psHeap->ch_pAddress ) ) / nBlockSize;

					if ( psFirstLowBlock == NULL )
					{
						panic( "shrink_cache_heaps() no free block below new size (%d)\n", nBlockNum );
					}

					psFirstLowBlock = psFreeBlock->cb_psNext;

					if ( psBlock->cb_nRefCount == 0 )
					{
						psList = &g_sBlockCache.bc_sNormal;
					}
					else
					{
						psList = &g_sBlockCache.bc_sLocked;
					}
					if ( bc_hash_delete( &g_sBlockCache.bc_sHashTab, psBlock->cb_nDevice, psBlock->cb_nBlockNum ) != psBlock )
					{
						panic( "shrink_cache_heaps() failed to remove block from hash-table\n" );
					}
					memcpy( psFreeBlock, psBlock, nBlockSize );

					if ( psFreeBlock->cb_psPrev == NULL )
					{
						kassertw( psList->cl_psLRU == psBlock );
						psList->cl_psLRU = psFreeBlock;
					}
					else
					{
						kassertw( psFreeBlock->cb_psPrev->cb_psNext == psBlock );
						psFreeBlock->cb_psPrev->cb_psNext = psFreeBlock;
					}
					if ( psFreeBlock->cb_psNext == NULL )
					{
						if ( psList->cl_psMRU != psBlock )
						{
							printk( "LRU (%p) dont point to old block!\n", psList->cl_psMRU );
						}
						psList->cl_psMRU = psFreeBlock;
					}
					else
					{
						kassertw( psFreeBlock->cb_psNext->cb_psPrev == psBlock );
						psFreeBlock->cb_psNext->cb_psPrev = psFreeBlock;
					}
					bc_hash_insert( &g_sBlockCache.bc_sHashTab, psFreeBlock->cb_nDevice, psFreeBlock->cb_nBlockNum, psFreeBlock );
				}
				psBlock = ( CacheBlock_s * )( ( ( uint8 * )psBlock ) + nBlockSize );
			}
			// Free unused block headers
			while ( psHeap->ch_psFirstFreeBlock != NULL )
			{
				int nBlockNum;

				psBlock = psHeap->ch_psFirstFreeBlock;
				psHeap->ch_psFirstFreeBlock = psBlock->cb_psNext;
				nBlockNum = ( ( ( uint8 * )psBlock ) - ( ( uint8 * )psHeap->ch_pAddress ) ) / nBlockSize;
				if ( nBlockNum < nNewCount )
				{
					psBlock->cb_psNext = psFirstLowBlock;
					psFirstLowBlock = psBlock;
				}
			}
			while ( psFirstLowBlock != NULL )
			{
				psBlock = psFirstLowBlock;
				psFirstLowBlock = psBlock->cb_psNext;
				psBlock->cb_psNext = psHeap->ch_psFirstFreeBlock;
				psBlock->cb_psPrev = NULL;
				psHeap->ch_psFirstFreeBlock = psBlock;
			}
			nBytesFreed += psHeap->ch_nSize - nNewSize;
			psHeap->ch_nSize = nNewSize;
			UNLOCK( g_sBlockCache.bc_hLock );
			resize_area( psHeap->ch_hAreaID, psHeap->ch_nSize, true );
			LOCK( g_sBlockCache.bc_hLock );
		}
		psHeap->ch_bBusy = false;
	}
	return ( ( nBytesFreed > 0 ) ? nBytesFreed : ( ( bBusy ) ? -EAGAIN : 0 ) );
}
