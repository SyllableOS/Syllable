
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

#ifndef __F_BCACHE_H__
#define __F_BCACHE_H__

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

#include <atheos/bcache.h>

typedef struct _CacheBlock CacheBlock_s;


typedef struct
{
	CacheBlock_s **ht_apsTable;
	area_id ht_hArea;	/* Area for ht_apsTable */
	int ht_nSize;		/* Size of ht_apsTable */
	int ht_nMask;		/* Used to mask out overflowing bits from the hash value */
	int ht_nCount;		/* Number of inserted elements */
} HashTable_s;

struct _CacheBlock
{
	CacheBlock_s *cb_psNextHash;
	CacheBlock_s *cb_psNext;
	CacheBlock_s *cb_psPrev;
	int cb_nDevice;
	off_t cb_nBlockNum;
	volatile flags_t cb_nFlags;
	int cb_nRefCount;
	cache_callback *cb_pFunc;
	void *cb_pArg;
};

static const flags_t CBF_SIZE_MASK = 0x000fffff;
static const flags_t CBF_DIRTY     = 0x00100000;
static const flags_t CBF_BUSY      = 0x00200000;
static const flags_t CBF_USED      = 0x00400000;
static const flags_t CBF_INLIST    = 0x00800000;

#define CB_SIZE(b) ((b)->cb_nFlags & CBF_SIZE_MASK)

static inline void *CB_DATA( CacheBlock_s *psBuffer )
{
	return ( psBuffer + 1 );
}

typedef struct
{
	CacheBlock_s *cl_psMRU;
	CacheBlock_s *cl_psLRU;
	int cl_nCount;
} CacheEntryList_s;

#define MAX_CACHE_BLK_SIZE_ORDERS	16

typedef struct
{
	CacheEntryList_s bc_sNormal;
	CacheEntryList_s bc_sLocked;

	HashTable_s bc_sHashTab;

	int bc_nMemUsage;
	atomic_t bc_nDirtyCount;
	sem_id bc_hLock;
} BlockCache_s;


int bc_hash_insert( HashTable_s * psTable, int nDev, off_t nBlockNum, CacheBlock_s *psBuffer );
void *bc_hash_delete( HashTable_s * psTable, int nDev, off_t nBlockNum );
void bc_add_to_head( CacheEntryList_s * psList, CacheBlock_s *psEntry );
void bc_remove_from_list( CacheEntryList_s * psList, CacheBlock_s *psEntry );

int alloc_cache_blocks( CacheBlock_s **apsBlocks, int nCount, int nBlockSize, bool *pbDidBlock );
void free_cache_block( CacheBlock_s *psBlock );
void release_cache_mem( void );

size_t shrink_block_cache( size_t nBytesNeeded );
ssize_t shrink_cache_heaps( int nIgnoredOrder );
void release_cache_blocks( int nBlockSize );
void flush_block_cache( void );
void init_block_cache( void );
int shutdown_block_cache( void );


#ifdef __cplusplus
}
#endif

#endif /* __F_BCACHE_H__ */
