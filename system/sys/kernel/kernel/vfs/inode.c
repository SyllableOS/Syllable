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
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/image.h>
#include <atheos/udelay.h>
#include <posix/limits.h>
#include <posix/errno.h>

#include <macros.h>

#include "inc/sysbase.h"
#include "vfs.h"

static FileSysDesc_s *load_filesystem( const char *pzName, const char *pzDevicePath );

bool init_dev_fs( void );
void init_pty( void );
int init_fifo( void );

static FSOperations_s g_sDummyOperations;

static const size_t HT_DEFAULT_SIZE = 128;
#define HASH(d, i)  ((d) ^ (i))

typedef struct
{
	Inode_s **ht_apsTable;
	int ht_nSize;		/* Size of ht_apsTable */
	int ht_nMask;		/* Used to mask out overflowing bits from the hash value */
	int ht_nCount;		/* Number of inserted elements */
} HashTable_s;

typedef struct
{
	Inode_s *il_psMRU;
	Inode_s *il_psLRU;
	int il_nCount;
} InodeList_s;


static HashTable_s g_sHashTable;
static InodeList_s g_sUsedList;
static InodeList_s g_sFreeList;

sem_id g_hInodeHashSem = -1;
static sem_id g_hFSListSema = -1;
static FileSysDesc_s *g_psFileSystems = NULL;
static Volume_s *g_psFirstVolume = NULL;


static Inode_s *hash_lookup_inode( HashTable_s * psTable, int nDev, ino_t nInode );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 *	Only called when mounting the virtual root file system and when
 *	registering the FIFO filesystem.
 * SEE ALSO:
 ****************************************************************************/


void add_mount_point( Volume_s *psVol )
{
	psVol->v_hMutex = create_semaphore( "volume_mutex", VOLUME_MUTEX_COUNT, 0 );

	psVol->v_psNext = g_psFirstVolume;
	g_psFirstVolume = psVol;
	printk( "Volume %d (%p) added\n", psVol->v_nDevNum, psVol->v_psOperations );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static Volume_s *find_mount_point( kdev_t nDevNum )
{
	Volume_s *psVol;

	for ( psVol = g_psFirstVolume; NULL != psVol; psVol = psVol->v_psNext )
	{
		if ( psVol->v_nDevNum == nDevNum )
		{
			return ( psVol );
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

void dump_hash_table( HashTable_s * psTable )
{
	int i;

	for ( i = 0; i < psTable->ht_nCount; ++i )
	{
		if ( psTable->ht_apsTable[i] != NULL )
		{
			Inode_s *psInode;

			dbprintf( DBP_DEBUGGER, "Hash %d:\n", i );
			for ( psInode = psTable->ht_apsTable[i]; NULL != psInode; psInode = psInode->i_psHashNext )
			{
				dbprintf( DBP_DEBUGGER, "%03d::%08Ld cnt=%d vol=%p mnt=%p vno=%p busy=%d\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode, atomic_read( &psInode->i_nCount ), psInode->i_psVolume, psInode->i_psMount, psInode->i_pFSData, psInode->i_bBusy );
			}
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int init_hash_table( HashTable_s * psTable )
{
	psTable->ht_nSize = HT_DEFAULT_SIZE;
	psTable->ht_nMask = psTable->ht_nSize - 1;
	psTable->ht_nCount = 0;

	psTable->ht_apsTable = kmalloc( psTable->ht_nSize * sizeof( Inode_s * ), MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == psTable->ht_apsTable )
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

static int resize_hash_table( HashTable_s * psTable )
{
	int nPrevSize;
	int nNewSize;
	int nNewMask;
	int i;
	int nHash;
	Inode_s **pasNewTable;

	if ( psTable->ht_nSize & psTable->ht_nMask )
	{
		panic( "resize_hash_table() (vfs): inconsistency between hashtable size %d and mask %d!\n", psTable->ht_nSize, psTable->ht_nMask );
	}

	nPrevSize = psTable->ht_nSize;
	nNewSize = nPrevSize * 2;
	nNewMask = nNewSize - 1;

	pasNewTable = kmalloc( nNewSize * sizeof( Inode_s * ), MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == pasNewTable )
	{
		return ( -ENOMEM );
	}

	for ( i = 0; i < nPrevSize; ++i )
	{
		Inode_s *psInode;
		Inode_s *psNext = NULL;

		for ( psInode = psTable->ht_apsTable[i]; NULL != psInode; psInode = psNext )
		{
			nHash = psInode->i_nHashVal & nNewMask;

			psNext = psInode->i_psHashNext;

			psInode->i_psHashNext = pasNewTable[nHash];
			psInode->i_psHashPrev = NULL;

			if ( pasNewTable[nHash] != NULL )
			{
				pasNewTable[nHash]->i_psHashPrev = psInode;
			}
			pasNewTable[nHash] = psInode;
		}
	}
	kfree( psTable->ht_apsTable );

	psTable->ht_apsTable = pasNewTable;
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

static int hash_insert_inode( HashTable_s * psTable, Inode_s *psInode )
{
	Inode_s *psTmp;
	int nHash;

	if ( NULL == psInode->i_psVolume )
	{
		panic( "hash_insert_inode() inode has no volume!\n" );
	}

	nHash = HASH( psInode->i_psVolume->v_nDevNum, psInode->i_nInode ) & psTable->ht_nMask;


	for ( psTmp = psTable->ht_apsTable[nHash]; psTmp != NULL; psTmp = psTmp->i_psHashNext )
	{
		if ( NULL == psTmp->i_psVolume )
		{
			panic( "hash_insert_inode() found inode without volume in hashtable!\n" );
		}

		if ( psTmp->i_nInode == psInode->i_nInode && psTmp->i_psVolume->v_nDevNum == psInode->i_psVolume->v_nDevNum )
		{
			if ( NULL == hash_lookup_inode( psTable, psInode->i_psVolume->v_nDevNum, psInode->i_nInode ) )
			{
				printk( "PANIC : Inode exists in hash table, but hash_lookup_inode() can't find it\n" );
			}
			panic( "hash_insert_inode() Entry already in the hash table! Dev=%d , Inode=%Ld\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode );
			return ( -EEXIST );
		}
	}

	psInode->i_nHashVal = HASH( psInode->i_psVolume->v_nDevNum, psInode->i_nInode );

	if ( psTable->ht_apsTable[nHash] != NULL )
	{
		psTable->ht_apsTable[nHash]->i_psHashPrev = psInode;
	}
	psInode->i_psHashNext = psTable->ht_apsTable[nHash];
	psInode->i_psHashPrev = NULL;
	psTable->ht_apsTable[nHash] = psInode;

	psTable->ht_nCount++;

	atomic_inc( &g_sSysBase.ex_nLoadedInodeCount );

	if ( psTable->ht_nCount >= ( ( psTable->ht_nSize * 3 ) / 4 ) )
	{
		return ( resize_hash_table( psTable ) );
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

static Inode_s *hash_lookup_inode( HashTable_s * psTable, int nDev, ino_t nInode )
{
	int nHash = HASH( nDev, nInode ) & psTable->ht_nMask;
	Inode_s *psInode;

	for ( psInode = psTable->ht_apsTable[nHash]; psInode != NULL; psInode = psInode->i_psHashNext )
	{
		if ( psInode->i_nInode == nInode && psInode->i_psVolume->v_nDevNum == nDev )
		{
			break;
		}
	}
	return ( psInode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void hash_delete_inode( HashTable_s * psTable, Inode_s *psInode )
{
	int nHash = HASH( psInode->i_psVolume->v_nDevNum, psInode->i_nInode ) & psTable->ht_nMask;

	if ( psTable->ht_apsTable[nHash] == psInode )
	{
		psTable->ht_apsTable[nHash] = psInode->i_psHashNext;
	}

	if ( psInode->i_psHashNext != NULL )
	{
		psInode->i_psHashNext->i_psHashPrev = psInode->i_psHashPrev;
	}
	if ( psInode->i_psHashPrev != NULL )
	{
		psInode->i_psHashPrev->i_psHashNext = psInode->i_psHashNext;
	}
	psInode->i_psHashPrev = psInode->i_psHashNext = NULL;

	psTable->ht_nCount--;

	atomic_dec( &g_sSysBase.ex_nLoadedInodeCount );

	if ( psTable->ht_nCount < 0 )
	{
		panic( "hash_delete_inode() : ht_nCount got negative value %d!!!!!\n", psTable->ht_nCount );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void dump_list( InodeList_s * psList )
{
	Inode_s *psInode;
	
	printk( "Dump List!\n" );

	for ( psInode = psList->il_psLRU; NULL != psInode; psInode = psInode->i_psNext )
	{
		dbprintf( DBP_DEBUGGER, "%03d::%08Ld cnt=%d vol=%p mnt=%p vno=%p busy=%d\n", 0/*psInode->i_psVolume->v_nDevNum*/, psInode->i_nInode, atomic_read( &psInode->i_nCount ), psInode->i_psVolume, psInode->i_psMount, psInode->i_pFSData, psInode->i_bBusy );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void add_file_to_inode( Inode_s *psInode, File_s *psFile )
{
	LOCK( g_hInodeHashSem );
	if ( NULL != psFile->f_psNext || NULL != psFile->f_psPrev )
	{
		printk( "PANIC : add_file_to_inode() Attempt to add file twice!\n" );
	}

	if ( psInode->i_psFirstFile != NULL )
	{
		psInode->i_psFirstFile->f_psPrev = psFile;
	}
	if ( psInode->i_psLastFile == NULL )
	{
		psInode->i_psLastFile = psFile;
	}

	psFile->f_psNext = psInode->i_psFirstFile;
	psFile->f_psPrev = NULL;
	psInode->i_psFirstFile = psFile;
	UNLOCK( g_hInodeHashSem );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void remove_file_from_inode( Inode_s *psInode, File_s *psFile )
{
	LOCK( g_hInodeHashSem );
	if ( NULL != psFile->f_psNext )
	{
		psFile->f_psNext->f_psPrev = psFile->f_psPrev;
	}
	if ( NULL != psFile->f_psPrev )
	{
		psFile->f_psPrev->f_psNext = psFile->f_psNext;
	}

	if ( psInode->i_psFirstFile == psFile )
	{
		psInode->i_psFirstFile = psFile->f_psNext;
	}
	if ( psInode->i_psLastFile == psFile )
	{
		psInode->i_psLastFile = psFile->f_psPrev;
	}

	psFile->f_psNext = NULL;
	psFile->f_psPrev = NULL;
	UNLOCK( g_hInodeHashSem );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void add_to_head( InodeList_s * psList, Inode_s *psInode )
{
	if ( NULL != psInode->i_psNext || NULL != psInode->i_psPrev )
	{
		printk( "PANIC : add_to_head() Attempt to add inode twice!\n" );
	}
	if ( NULL != psList->il_psMRU )
	{
		psList->il_psMRU->i_psNext = psInode;
	}
	if ( NULL == psList->il_psLRU )
	{
		psList->il_psLRU = psInode;
	}

	psInode->i_psNext = NULL;
	psInode->i_psPrev = psList->il_psMRU;
	psList->il_psMRU = psInode;
	psList->il_nCount++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void add_to_tail( InodeList_s * psList, Inode_s *psInode )
{
	if ( NULL != psInode->i_psNext || NULL != psInode->i_psPrev )
	{
		printk( "PANIC : add_to_tail() Attempt to add inode twice!\n" );
	}

	if ( NULL != psList->il_psLRU )
	{
		psList->il_psLRU->i_psPrev = psInode;
	}
	if ( NULL == psList->il_psMRU )
	{
		psList->il_psMRU = psInode;
	}

	psInode->i_psNext = psList->il_psLRU;
	psInode->i_psPrev = NULL;
	psList->il_psLRU = psInode;
	psList->il_nCount++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void remove_from_list( InodeList_s * psList, Inode_s *psInode )
{
	if ( NULL != psInode->i_psNext )
	{
		psInode->i_psNext->i_psPrev = psInode->i_psPrev;
	}
	if ( NULL != psInode->i_psPrev )
	{
		psInode->i_psPrev->i_psNext = psInode->i_psNext;
	}

	if ( psList->il_psLRU == psInode )
	{
		psList->il_psLRU = psInode->i_psNext;
	}
	if ( psList->il_psMRU == psInode )
	{
		psList->il_psMRU = psInode->i_psPrev;
	}

	psInode->i_psNext = NULL;
	psInode->i_psPrev = NULL;
	psList->il_nCount--;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static Inode_s *remove_lru_node( InodeList_s * psList )
{
	Inode_s *psInode;

	psInode = psList->il_psLRU;

	if ( psInode != NULL )
	{
		if ( psInode->i_psPrev != NULL )
		{
			panic( "remove_lru_node() i_psPrev == %px\n", psInode->i_psPrev );
		}
		if ( NULL != psInode->i_psNext )
		{
			psInode->i_psNext->i_psPrev = NULL;
		}
		else
		{
			psList->il_psMRU = NULL;
		}
		psList->il_psLRU = psInode->i_psNext;
		psInode->i_psNext = NULL;
		psList->il_nCount--;
	}
	return ( psInode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int grow_inodes( void )
{
	Inode_s *pasInodes;
	int i;

	pasInodes = kmalloc( PAGE_SIZE, MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

	if ( NULL != pasInodes )
	{
		for ( i = 0; i < PAGE_SIZE / sizeof( Inode_s ); ++i )
		{
			add_to_tail( &g_sFreeList, &pasInodes[i] );
		}
		atomic_add( &g_sSysBase.ex_nAllocatedInodeCount, PAGE_SIZE / sizeof( Inode_s ) );
//      printk( "grow_inodes() : %d inodes added (f: %d, u: %d, h: %d)\n",
//              i, g_sFreeList.il_nCount, g_sUsedList.il_nCount, g_sHashTable.ht_nCount );

		return ( 0 );
	}
	return ( -ENOMEM );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void clear_inode( Inode_s *psInode )
{
	psInode->i_nInode = 0;
	atomic_set( &psInode->i_nCount, 0 );
	psInode->i_psVolume = NULL;
	psInode->i_psOperations = NULL;
	psInode->i_psMount = NULL;
	psInode->i_psAreaList = NULL;
	psInode->i_pFSData = NULL;
	psInode->i_nFlags = 0;
	psInode->i_bBusy = false;
	psInode->i_bDeleted = false;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void write_inode( Inode_s *psInode )
{
	if ( atomic_read( &psInode->i_nCount ) != 0 )
	{
		panic( "write_inode() : inode has count of %d\n", atomic_read( &psInode->i_nCount ) );
	}

	if ( NULL != psInode->i_psOperations->write_inode )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		if ( psInode->i_psOperations->write_inode( psInode->i_psVolume->v_pFSData, psInode->i_pFSData ) < 0 )
		{
			printk( "PANIC : failed to write inode %d::%Ld\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode );
		}
	}
	atomic_dec( &psInode->i_psVolume->v_nOpenCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Inode_s *do_get_inode( kdev_t nDevNum, ino_t nInoNum, bool bCrossMount, bool bOkToFlush )
{
	Inode_s *psInode;
	int nRetries = 0;

	LOCK( g_hInodeHashSem );
	for ( ;; )
	{
		psInode = hash_lookup_inode( &g_sHashTable, nDevNum, nInoNum );

		if ( NULL != psInode )
		{
			int nCount;

			if ( psInode->i_psVolume->v_bUnmounted )
			{
				printk( "get_inode() found i-node from unmounted file-system in the hash table\n" );
				UNLOCK( g_hInodeHashSem );
				return ( NULL );
			}
			if ( psInode->i_bBusy )
			{
				UNLOCK( g_hInodeHashSem );
				snooze( 1000 );
				if ( nRetries++ > 30000 )
				{
					if ( nRetries < 30020 )
					{
						printk( "Error:  inode %d::%Ld stuck in busy state!\n", nDevNum, nInoNum );
					}
					else
					{
						printk( "Error:  get_inode() give up! Someone won't release inode %d::%Ld\n", nDevNum, nInoNum );
						return ( NULL );
					}
				}
				LOCK( g_hInodeHashSem );
				continue;
			}
			if ( bCrossMount && NULL != psInode->i_psMount )
			{
				psInode = psInode->i_psMount;
			}

			nCount = atomic_inc_and_read( &psInode->i_nCount );

			if ( nCount == 0 )
			{
				remove_from_list( &g_sUsedList, psInode );
				atomic_inc( &g_sSysBase.ex_nUsedInodeCount );
			}
			UNLOCK( g_hInodeHashSem );
			return ( psInode );
		}
		else
		{
			Volume_s *psVolume = find_mount_point( nDevNum );

			if ( psVolume == NULL )
			{
				printk( "Error : get_inode() failed to find mount point for device %d\n", nDevNum );
				UNLOCK( g_hInodeHashSem );
				return ( NULL );
			}
			if ( psVolume->v_bUnmounted )
			{
				printk( "Attempt to load inode from an unmounted file system\n" );
				UNLOCK( g_hInodeHashSem );
				return ( NULL );
			}

			// The inode was not loaded already, so we attempt to find an unused, non valid inode.

			if ( bOkToFlush == false || g_sUsedList.il_nCount < 128 )
			{
				psInode = remove_lru_node( &g_sFreeList );

				if ( NULL != psInode )
				{
					int nError;

					if ( psInode->i_psVolume != NULL )
					{
						panic( "get_inode() : found free inode with i_psVolume = %p\n", psInode->i_psVolume );
						UNLOCK( g_hInodeHashSem );
						return ( NULL );
					}
					// We found it, so it should be ok to use it.
					psInode->i_psVolume = psVolume;
					psInode->i_psOperations = psVolume->v_psOperations;
					psInode->i_nInode = nInoNum;
					psInode->i_bBusy = true;	// make sure nobody start using it before it's read

					hash_insert_inode( &g_sHashTable, psInode );

					UNLOCK( g_hInodeHashSem );
					kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
					nError = psInode->i_psOperations->read_inode( psInode->i_psVolume->v_pFSData, nInoNum, &psInode->i_pFSData );

					if ( nError < 0 )
					{
						LOCK( g_hInodeHashSem );
						hash_delete_inode( &g_sHashTable, psInode );
						clear_inode( psInode );
						add_to_head( &g_sFreeList, psInode );

						psInode->i_bBusy = false;
						psInode = NULL;
						UNLOCK( g_hInodeHashSem );
//                      printk( "Error : get_inode( %d, %d, %d ) failed to load i-node\n", nDevNum, (int)nInoNum, bCrossMount );
						return ( NULL );
					}
					else
					{
						atomic_inc( &psVolume->v_nOpenCount );
						atomic_inc( &psInode->i_nCount );
						psInode->i_bBusy = false;	// tell the world that a new i-node is born
						atomic_inc( &g_sSysBase.ex_nUsedInodeCount );
						return ( psInode );
					}
				}
			}
			// No free i-nodes, we try to steal a valid, but unreferenced i-node.
			if ( bOkToFlush )
			{
				psInode = remove_lru_node( &g_sUsedList );
				if ( NULL != psInode )
				{
					psInode->i_bBusy = true;

					if ( atomic_read( &psInode->i_nCount ) != 0 )
					{
						panic( "get_inode() inode %d::%Ld in used list has count %d busy=%d\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode, atomic_read( &psInode->i_nCount ), psInode->i_bBusy );
					}
					// Lets inform the file-system about our intentions.
					UNLOCK( g_hInodeHashSem );
					write_inode( psInode );
					LOCK( g_hInodeHashSem );


					hash_delete_inode( &g_sHashTable, psInode );

					if ( psInode->i_psVolume->v_bUnmounted == true && atomic_read( &psInode->i_psVolume->v_nOpenCount ) == 0 )
					{
						printk( "Last inode released from unmounted volume, deletes it...\n" );
						delete_semaphore( psInode->i_psVolume->v_hMutex );
						kfree( psInode->i_psVolume );
					}

					clear_inode( psInode );
					psInode->i_bBusy = false;
					add_to_head( &g_sFreeList, psInode );
					continue;
				}
			}
			// Bad luck, no unreferenced i-nodes at all. We must attempt to allocate some more of them.
			if ( grow_inodes() != 0 )
			{
				printk( "Error : No memory for new inodes\n" );
				UNLOCK( g_hInodeHashSem );
				return ( NULL );
			}
		}
	}
}

Inode_s *get_inode( kdev_t nDevNum, ino_t nInoNum, bool bCrossMount )
{
	return ( do_get_inode( nDevNum, nInoNum, bCrossMount, true ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void put_inode( Inode_s *psInode )
{
	if ( NULL == psInode )
	{
		printk( "PANIC : put_inode() called with NULL pointer!!\n" );
		return;
	}

	LOCK( g_hInodeHashSem );

	if ( atomic_dec_and_test( &psInode->i_nCount ) )
	{
		kassertw( psInode->i_psFirstFile == NULL );
		if ( psInode->i_psVolume->v_bUnmounted )
		{

			hash_delete_inode( &g_sHashTable, psInode );

			if ( atomic_dec_and_test( &psInode->i_psVolume->v_nOpenCount ) )
			{
				printk( "put_inode() Last inode released from unmounted volume, delete it...\n" );
				delete_semaphore( psInode->i_psVolume->v_hMutex );
				kfree( psInode->i_psVolume );
			}
			clear_inode( psInode );
			add_to_head( &g_sFreeList, psInode );
		}
		else
		{
			if ( psInode->i_bDeleted )
			{
				psInode->i_bBusy = true;
				UNLOCK( g_hInodeHashSem );
				write_inode( psInode );
				LOCK( g_hInodeHashSem );

				hash_delete_inode( &g_sHashTable, psInode );

				if ( psInode->i_psVolume->v_bUnmounted == true && atomic_read( &psInode->i_psVolume->v_nOpenCount ) == 0 )
				{
					printk( "Last inode released from unmounted volume, delete it...\n" );
					delete_semaphore( psInode->i_psVolume->v_hMutex );
					kfree( psInode->i_psVolume );
				}

				clear_inode( psInode );
				psInode->i_bBusy = false;
				add_to_head( &g_sFreeList, psInode );
			}
			else
			{
				add_to_head( &g_sUsedList, psInode );
			}
		}
		atomic_dec( &g_sSysBase.ex_nUsedInodeCount );
	}
	UNLOCK( g_hInodeHashSem );

}

/** Free an unused inode
 * \par Description:
 *	Inodes are kept by the kernel for a while even after the last process
 *	has released it. This may save an read_inode() if the inode is requested
 *	again shortly, but it can cause problems for kertain file-systems that
 *	can't do kertain operations on an inode while the kernel holds it. This
 *	function can then be used to flush out an unused inode.
 * \par Note:
 * \param nDev - Filesystem ID of the FS owning the inode.
 * \param nInode - The inode number identifying the inode.
 * \return If the inode was indeed loaded but unreferenced the inode is written
 *	   to disk and 0 is returned. If the inode was found, but still in use
 *	   -EBUSY is returned. If the inode was not loaded at all -ENOENT is returned
 * \sa get_inode(), put_inode(), get_vnode(), put_vnode()
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

int flush_inode( int nDev, ino_t nInode )
{
	Inode_s *psInode;
	int nError;

	LOCK( g_hInodeHashSem );
	psInode = hash_lookup_inode( &g_sHashTable, nDev, nInode );
	if ( psInode == NULL )
	{
		nError = -ENOENT;
		goto error;
	}
	if ( atomic_read( &psInode->i_nCount ) != 0 )
	{
		nError = -EBUSY;
		goto error;
	}

	remove_from_list( &g_sUsedList, psInode );
	psInode->i_bBusy = true;
	UNLOCK( g_hInodeHashSem );
	write_inode( psInode );
	LOCK( g_hInodeHashSem );

	hash_delete_inode( &g_sHashTable, psInode );

	if ( psInode->i_psVolume->v_bUnmounted == true && atomic_read( &psInode->i_psVolume->v_nOpenCount ) == 0 )
	{
		printk( "Last inode released from unmounted volume, deletes it...\n" );
		delete_semaphore( psInode->i_psVolume->v_hMutex );
		kfree( psInode->i_psVolume );
	}

	clear_inode( psInode );
	psInode->i_bBusy = false;
	add_to_head( &g_sFreeList, psInode );
	nError = 0;
      error:
	UNLOCK( g_hInodeHashSem );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_vnode( int nDev, ino_t nInode, void **pNode )
{
	Inode_s *psInode = do_get_inode( nDev, nInode, false, false );

	if ( NULL != psInode )
	{
		*pNode = psInode->i_pFSData;
		return ( 0 );
	}
	else
	{
		printk( "Error : get_vnode() failed to load inode!\n" );
		return ( -EINVAL );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int put_vnode( int nDev, ino_t nInode )
{
	Inode_s *psInode;
	int nError = 0;

	psInode = do_get_inode( nDev, nInode, false, false );

	if ( NULL == psInode )
	{
		printk( "Panic : put_vnode() failed to locate inode!\n" );
		nError = -EINVAL;
		goto error;
	}
	kassertw( atomic_read( &psInode->i_nCount ) > 1 );

	atomic_dec( &psInode->i_nCount );	// One for get_inode() above

	put_inode( psInode );	// And one for the original get_vnode()
      error:
	return ( nError );
}

int set_inode_deleted_flag( int nDev, ino_t nInode, bool bIsDeleted )
{
	Inode_s *psInode;
	int nRetries = 0;

      again:
	LOCK( g_hInodeHashSem );
	psInode = hash_lookup_inode( &g_sHashTable, nDev, nInode );

	if ( NULL == psInode )
	{
		UNLOCK( g_hInodeHashSem );
		return ( -ENOENT );
	}

	if ( psInode->i_bBusy )
	{
		UNLOCK( g_hInodeHashSem );
		if ( nRetries++ > 30000 )
		{
			if ( nRetries < 30020 )
			{
				printk( "Error: set_vnode_deleted_flag() inode %d::%Ld stuck in busy state!\n", nDev, nInode );
			}
			else
			{
				printk( "Error: set_vnode_deleted_flag() give up! Someone won't release inode %d::%Ld\n", nDev, nInode );
				return ( -EINVAL );
			}
		}
		snooze( 1000 );
		goto again;
	}

	psInode->i_bDeleted = bIsDeleted;
	UNLOCK( g_hInodeHashSem );
	return ( 0 );
}

int get_inode_deleted_flag( int nDev, ino_t nInode )
{
	Inode_s *psInode;
	int nRetries = 0;
	bool bIsDeleted;

      again:
	LOCK( g_hInodeHashSem );
	psInode = hash_lookup_inode( &g_sHashTable, nDev, nInode );

	if ( NULL == psInode )
	{
		UNLOCK( g_hInodeHashSem );
		return ( -ENOENT );
	}

	if ( psInode->i_bBusy )
	{
		UNLOCK( g_hInodeHashSem );
		if ( nRetries++ > 30000 )
		{
			if ( nRetries < 30020 )
			{
				printk( "Error: get_vnode_deleted_flag() inode %d::%Ld stuck in busy state!\n", nDev, nInode );
			}
			else
			{
				printk( "Error: get_vnode_deleted_flag() give up! Someone won't release inode %d::%Ld\n", nDev, nInode );
				return ( -EINVAL );
			}
		}
		snooze( 1000 );
		goto again;
	}

	bIsDeleted = psInode->i_bDeleted;
	UNLOCK( g_hInodeHashSem );
	return ( ( bIsDeleted ) ? 1 : 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

FileSysDesc_s *register_file_system( const char *pzName, FSOperations_s * psOps, int nAPIVersion )
{
	FileSysDesc_s *psDesc;

	psDesc = kmalloc( sizeof( FileSysDesc_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == psDesc )
	{
		printk( "Error: register_file_system() no memory for fs descriptor. FS %s not registered\n", pzName );
		return ( NULL );
	}

	strcpy( psDesc->fs_zName, pzName );
	psDesc->fs_psOperations = psOps;
	atomic_set( &psDesc->fs_nRefCount, 1 );
	psDesc->fs_nImage = -1;
	psDesc->fs_nAPIVersion = nAPIVersion;

	psDesc->fs_psNext = g_psFileSystems;
	g_psFileSystems = psDesc;

	return ( psDesc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void release_fs( FileSysDesc_s *psDesc )
{
	FileSysDesc_s **ppsTmp;
	char zPath[256];
	int i;

	if ( !atomic_dec_and_test( &psDesc->fs_nRefCount ) )
	{
		return;
	}
	if ( psDesc->fs_nImage < 0 )
	{
		printk( "Panic: attempt to unload builtin filesystem %s\n", psDesc->fs_zName );
		return;
	}

	/* Do not unload bootmodule fs drivers */
	for ( i = 0; i < g_sSysBase.ex_nBootModuleCount; i++ )
	{
		strcpy( zPath, "/system/drivers/fs/" );
		strcat( zPath, psDesc->fs_zName );
		if ( !strcmp( zPath, g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs ) )
		{
			return;
		}
	}


	LOCK( g_hFSListSema );
	for ( ppsTmp = &g_psFileSystems; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->fs_psNext )
	{
		if ( *ppsTmp == psDesc )
		{
			*ppsTmp = psDesc->fs_psNext;
			break;
		}
	}
	unload_kernel_driver( psDesc->fs_nImage );
	UNLOCK( g_hFSListSema );

	/* If we allocated a patched operations struct, free it here.  */
	if ( psDesc->fs_nAPIVersion != FSDRIVER_API_VERSION )
	{
		kfree( psDesc->fs_psOperations );
	}

	kfree( psDesc );
}

static FileSysDesc_s *init_fs_driver( int nDriver, const char *pzName )
{
	FileSysDesc_s *psDesc;
	init_fs_func *pInitFunc;
	FSOperations_s *psOps = NULL;
	int nError;

	nError = get_symbol_address( nDriver, "fs_init", -1, ( void ** )&pInitFunc );

	if ( nError < 0 )
	{
		printk( "Failed to find filesystem init function\n" );
		goto error1;
	}
	nError = pInitFunc( pzName, &psOps );

	if ( nError < 0 )
	{
		goto error1;
	}

	if ( nError == 1 )	// API version 1 has no op_truncate
	{
		// Allocate new operations struct of the correct size.
		FSOperations_s *psOps2 = kmalloc( sizeof( FSOperations_s ), MEMF_KERNEL | MEMF_CLEAR );
		if ( psOps2 == NULL )
		{
			goto error1;
		}
		memcpy( psOps2, psOps, ( sizeof( FSOperations_s ) - 4 ) );
		psOps = psOps2;
	}
	else if ( nError != FSDRIVER_API_VERSION )
	{
		printk( "Error: Filesystem driver '%s' has unknown version number %d\n", pzName, nError );
		goto error1;
	}

	if ( psOps == NULL )
	{
		printk( "Panic: load_filesystem() filesystem init function did not set the function table pointer\n" );
		goto error1;
	}
	psDesc = register_file_system( pzName, psOps, nError );
	psDesc->fs_nImage = nDriver;
	return ( psDesc );
      error1:
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FileSysDesc_s *find_fs_driver( const char *pzName )
{
	FileSysDesc_s *psDesc;

	for ( psDesc = g_psFileSystems; NULL != psDesc; psDesc = psDesc->fs_psNext )
	{
		if ( strcmp( psDesc->fs_zName, pzName ) == 0 )
		{
			return ( psDesc );
		}
	}
	return ( NULL );
}

static int probe_device_path( const char *pzDriverDir, const char *pzDevicePath, fs_info * psFSInfo, FileSysDesc_s **ppsDesc )
{
	struct kernel_dirent sDirEnt;
	FileSysDesc_s *psDesc;
	int nDir;
	int nError;

	nError = -EUNKNOWNFS;
	for ( psDesc = g_psFileSystems; NULL != psDesc; psDesc = psDesc->fs_psNext )
	{
		if ( psDesc->fs_psOperations->probe == NULL )
		{
			continue;
		}
		strncpy( psFSInfo->fi_device_path, pzDevicePath, sizeof( psFSInfo->fi_device_path ) );
		psFSInfo->fi_device_path[sizeof( psFSInfo->fi_device_path ) - 1] = '\0';
		strcpy( psFSInfo->fi_driver_name, psDesc->fs_zName );

		if ( psDesc->fs_psOperations->probe( pzDevicePath, psFSInfo ) < 0 )
		{
			continue;
		}
		if ( psFSInfo->fi_flags & FS_CAN_MOUNT )
		{
			atomic_inc( &psDesc->fs_nRefCount );
			nError = 0;
			goto done;
		}
	}

	nDir = open( pzDriverDir, O_RDONLY );
	if ( nDir < 0 )
	{
		printk( "Error: load_filesystem() failed to open directory %s\n", pzDriverDir );
		return ( nDir );
	}

	nError = -EUNKNOWNFS;
	while ( getdents( nDir, &sDirEnt, 1 ) == 1 )
	{
		if ( strcmp( sDirEnt.d_name, "." ) == 0 || strcmp( sDirEnt.d_name, ".." ) == 0 )
		{
			continue;
		}
		if ( find_fs_driver( sDirEnt.d_name ) != NULL )
		{
			continue;
		}
		psDesc = load_filesystem( sDirEnt.d_name, NULL );
		if ( psDesc == NULL )
		{
			continue;
		}
		if ( psDesc->fs_psOperations->probe == NULL )
		{
			release_fs( psDesc );
			continue;
		}
		strncpy( psFSInfo->fi_device_path, pzDevicePath, sizeof( psFSInfo->fi_device_path ) );
		psFSInfo->fi_device_path[sizeof( psFSInfo->fi_device_path ) - 1] = '\0';
		strcpy( psFSInfo->fi_driver_name, psDesc->fs_zName );
		if ( psDesc->fs_psOperations->probe( pzDevicePath, psFSInfo ) < 0 )
		{
			release_fs( psDesc );
			continue;
		}
		if ( psFSInfo->fi_flags & FS_CAN_MOUNT )
		{
			close( nDir );
			nError = 0;
			goto done;
		}
	}
	close( nDir );
	return ( nError );
      done:
	*ppsDesc = psDesc;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FileSysDesc_s *load_filesystem( const char *pzName, const char *pzDevicePath )
{
	char zPath[256];
	int nPathLen;
	FileSysDesc_s *psDesc;
	int nDriver;

	if ( pzName == NULL && pzDevicePath == NULL )
	{
		return ( NULL );
	}
	LOCK( g_hFSListSema );
	if ( pzName != NULL )
	{
		psDesc = find_fs_driver( pzName );

		if ( psDesc != NULL )
		{
			atomic_inc( &psDesc->fs_nRefCount );
			UNLOCK( g_hFSListSema );
			return ( psDesc );
		}
	}
	sys_get_system_path( zPath, 256 );
	nPathLen = strlen( zPath );

	if ( '/' != zPath[nPathLen - 1] )
	{
		zPath[nPathLen] = '/';
		zPath[nPathLen + 1] = '\0';
	}
	strcat( zPath, "system/drivers/fs/" );

	if ( pzName != NULL )
	{
		strcat( zPath, pzName );

		nDriver = load_kernel_driver( zPath );

		if ( nDriver < 0 )
		{
			printk( "load_filesystem() failed to load driver\n" );
			goto error1;
		}

		psDesc = init_fs_driver( nDriver, pzName );
		if ( psDesc == NULL )
		{
			goto error2;
		}
	}
	else
	{
		fs_info *psInfo = kmalloc( sizeof( fs_info ), MEMF_KERNEL | MEMF_CLEAR );

		if ( psInfo != NULL )
		{
			if ( probe_device_path( zPath, pzDevicePath, psInfo, &psDesc ) < 0 )
			{
				psDesc = NULL;
			}
			kfree( psInfo );
		}
		else
		{
			psDesc = NULL;
		}
	}
	UNLOCK( g_hFSListSema );
	return ( psDesc );
      error2:
	unload_kernel_driver( nDriver );
      error1:
	UNLOCK( g_hFSListSema );
	return ( NULL );
}

int get_mount_point_count( void )
{
	Volume_s *psVol;
	int nCount = 0;

	LOCK( g_hInodeHashSem );
	for ( psVol = g_psFirstVolume; psVol != NULL; psVol = psVol->v_psNext )
	{
		if ( psVol->v_psMountPoint != NULL )
		{
			nCount++;
		}
	}
	UNLOCK( g_hInodeHashSem );
	return ( nCount );
}

int sys_get_mount_point_count( void )
{
	return ( get_mount_point_count() );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_get_mount_point( int nIndex, char *pzBuffer, int nBufLen )
{
	Volume_s *psVol;
	Inode_s *psMntPnt = NULL;
	char *pzPath;
	int nPos;
	int nError;

	if ( nBufLen < 2 )
	{
		return ( -EINVAL );
	}
	LOCK( g_hInodeHashSem );

	for ( psVol = g_psFirstVolume; psVol != NULL && ( nIndex > 0 || psVol->v_psMountPoint == NULL ); psVol = psVol->v_psNext )
	{
		if ( psVol->v_psMountPoint != NULL )
		{
			nIndex--;
		}
	}

	if ( psVol != NULL )
	{
		psMntPnt = psVol->v_psMountPoint;
		if ( psMntPnt != NULL )
		{
			atomic_inc( &psMntPnt->i_nCount );
		}
	}
	UNLOCK( g_hInodeHashSem );
	if ( psVol == NULL )
	{
		return ( -EBADINDEX );
	}
	if ( psMntPnt == NULL )
	{
		if ( strcpy_to_user( pzBuffer, "/" ) < 0 )
		{
			return ( -EFAULT );
		}
		return ( 0 );
	}
	pzPath = kmalloc( PATH_MAX, MEMF_KERNEL | MEMF_OKTOFAIL );
	if ( pzPath == NULL )
	{
		return ( -ENOMEM );
	}
	nPos = PATH_MAX;
	pzPath[--nPos] = '\0';
	nError = 0;
	while ( nError >= 0 )
	{
		Inode_s *psParent;
		struct kernel_dirent sDirEnt;
		int nDir;

		LOCK( g_hInodeHashSem );
		if ( psMntPnt->i_nInode == psMntPnt->i_psVolume->v_nRootInode )
		{
			if ( psMntPnt->i_psVolume->v_psMountPoint == NULL )
			{
				nError = 0;
				UNLOCK( g_hInodeHashSem );
				break;
			}
			else
			{
				psParent = psMntPnt->i_psVolume->v_psMountPoint;
				atomic_inc( &psParent->i_nCount );
				put_inode( psMntPnt );
				psMntPnt = psParent;
			}
		}
		UNLOCK( g_hInodeHashSem );
		nError = get_named_inode( psMntPnt, "..", &psParent, false, false );

		if ( nError < 0 || psParent == NULL )
		{
			if ( nError == -ENOENT )
			{
				nError = 0;	// Hit the root of the root FS.
			}
			break;
		}
		nDir = open_inode( true, psParent, FDT_DIR, O_RDONLY );
		if ( nDir < 0 )
		{
			nError = nDir;
			put_inode( psParent );
			break;
		}
		nError = -ENOENT;
		while ( getdents( nDir, &sDirEnt, sizeof( sDirEnt ) ) == 1 )
		{
			if ( sDirEnt.d_ino == psMntPnt->i_nInode )
			{
				if ( sDirEnt.d_namlen + 1 > nPos )
				{
					nError = -ENAMETOOLONG;
				}
				else
				{
					nPos -= sDirEnt.d_namlen;
					memcpy( pzPath + nPos, sDirEnt.d_name, sDirEnt.d_namlen );
					pzPath[--nPos] = '/';
					nError = 0;
				}
				break;
			}
		}
		close( nDir );
		put_inode( psMntPnt );
		psMntPnt = psParent;
	}
	put_inode( psMntPnt );
	if ( nError >= 0 )
	{
		int nLen = PATH_MAX - nPos;

		if ( nLen > nBufLen )
		{
			nError = -ENAMETOOLONG;
		}
		else
		{
			nError = 0;
			if ( memcpy_to_user( pzBuffer, pzPath + nPos, nLen ) < 0 )
			{
				nError = -EFAULT;
			}
		}
	}
	kfree( pzPath );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_get_fs_info( bool bKernel, int nFile, fs_info * psInfo )
{
	File_s *psFile = get_fd( bKernel, nFile );
	Inode_s *psInode;
	int nError;

	if ( NULL == psFile )
	{
		return ( -EBADF );
	}
	psInode = psFile->f_psInode;

	if ( psInode->i_psOperations->rfsstat != NULL )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		strcpy( psInfo->fi_device_path, psInode->i_psVolume->v_zDevicePath );
		strcpy( psInfo->fi_driver_name, psInode->i_psVolume->v_psFSDesc->fs_zName );
		nError = psInode->i_psOperations->rfsstat( psInode->i_psVolume->v_pFSData, psInfo );
	}
	else
	{
		nError = -EPERM;
	}
	put_fd( psFile );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_get_fs_info( int nVersion, int nFile, fs_info * psInfo )
{
	fs_info sInfo;
	int nError;

	if ( nVersion != FSINFO_VERSION )
	{
		printk( "sys_get_fs_info() called with unknown version %d\n", nVersion );
		return ( -EINVAL );
	}

	memset( &sInfo, 0, sizeof( sInfo ) );

	nError = do_get_fs_info( false, nFile, &sInfo );

	if ( nError >= 0 )
	{
		if ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) < 0 )
		{
			return ( -EFAULT );
		}
	}
	return ( nError );
}

status_t sys_probe_fs( int nVersion, const char *pzDevicePath, fs_info * psInfo )
{
	FileSysDesc_s *psDesc;
	char zPath[256];
	int nPathLen;
	int nError = 0;

	if ( verify_mem_area( psInfo, sizeof( *psInfo ), true ) < 0 )
	{
		return ( -EFAULT );
	}
	memset( psInfo, 0, sizeof( *psInfo ) );
	sys_get_system_path( zPath, 256 );
	nPathLen = strlen( zPath );

	if ( '/' != zPath[nPathLen - 1] )
	{
		zPath[nPathLen] = '/';
		zPath[nPathLen + 1] = '\0';
	}
	strcat( zPath, "system/drivers/fs/" );

	LOCK( g_hFSListSema );
	nError = probe_device_path( zPath, pzDevicePath, psInfo, &psDesc );
	if ( nError >= 0 )
	{
		release_fs( psDesc );
	}
	UNLOCK( g_hFSListSema );


	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_initialize_fs( const char *pzDevPath, const char *pzFsType, const char *pzVolName, void *pArgs, int nArgLen )
{
	FileSysDesc_s *psDesc;
	int nError;

	psDesc = load_filesystem( pzFsType, NULL );

	if ( NULL == psDesc )
	{
		return ( -ENODEV );
	}
	if ( NULL != psDesc->fs_psOperations->initialize )
	{
		nError = psDesc->fs_psOperations->initialize( pzDevPath, pzVolName, pArgs, nArgLen );
	}
	else
	{
		nError = -EINVAL;
	}

	release_fs( psDesc );

	return ( nError );
}

int sys_sync( void )
{
	Volume_s *psVol;
	int nError = 0;

	for ( psVol = g_psFirstVolume; psVol != NULL; psVol = psVol->v_psNext )
	{
		int nRes;

		if ( psVol->v_psOperations == NULL || psVol->v_psOperations->sync == NULL )
		{
			continue;
		}
		nRes = psVol->v_psOperations->sync( psVol->v_pFSData );
		if ( nRes < 0 )
		{
			nError = nRes;
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

static int do_mount( const char *pzDevName, const char *pzDirName, const char *pzFSName, int nFlags, int nFSID, const void *pData )
{
	Volume_s *psVol;
	FileSysDesc_s *psDesc;
	Inode_s *psDir;
	int nError;
	int nArgLen = 123;	/* FIXME : This should be a parameter */
	int nRetries;

	nError = get_named_inode( NULL, pzDirName, &psDir, false, false );

	if ( nError < 0 )
	{
		goto error1;
	}

	if ( psDir->i_psMount != NULL )
	{
		nError = -EBUSY;
		goto error2;
	}

	psVol = kmalloc( sizeof( Volume_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );

	if ( NULL == psVol )
	{
		nError = -ENOMEM;
		goto error2;
	}

	psVol->v_hMutex = create_semaphore( "volume_mutex", VOLUME_MUTEX_COUNT, 0 );

	if ( psVol->v_hMutex < 0 )
	{
		nError = psVol->v_hMutex;
		goto error3;
	}

	psDesc = load_filesystem( pzFSName, pzDevName );
	LOCK( g_hInodeHashSem );

	if ( NULL == psDesc )
	{
		nError = -ENODEV;
		goto error4;
	}

	psVol->v_nDevNum = nFSID;
	psVol->v_psOperations = psDesc->fs_psOperations;
	psVol->v_psFSDesc = psDesc;
	psVol->v_psNext = g_psFirstVolume;
	g_psFirstVolume = psVol;

	psDir->i_bBusy = true;

	UNLOCK( g_hInodeHashSem );

	if ( pzDevName == NULL )
	{
		pzDevName = "";
	}

	if ( nFlags & MNTF_SLOW_DEVICE )
	{
		for ( nRetries = 0; nRetries < 5; nRetries++ )
		{
			nError = psDesc->fs_psOperations->mount( psVol->v_nDevNum, pzDevName, nFlags, pData, nArgLen, &psVol->v_pFSData, &psVol->v_nRootInode );

			if ( nError >= 0 )
				break;

			udelay( 1000000 );
			Schedule();
		}
	}
	else
		nError = psDesc->fs_psOperations->mount( psVol->v_nDevNum, pzDevName, nFlags, pData, nArgLen, &psVol->v_pFSData, &psVol->v_nRootInode );

	if ( nError < 0 )
	{
		psDir->i_bBusy = false;
		LOCK( g_hInodeHashSem );
		goto error5;
	}
	strncpy( psVol->v_zDevicePath, pzDevName, sizeof( psVol->v_zDevicePath ) );
	psVol->v_zDevicePath[sizeof( psVol->v_zDevicePath ) - 1] = '\0';

	psDir->i_psMount = get_inode( psVol->v_nDevNum, psVol->v_nRootInode, false );
	psVol->v_psMountPoint = psDir;
	psDir->i_bBusy = false;
	return ( 0 );
      error5:
	g_psFirstVolume = psVol->v_psNext;
	release_fs( psDesc );
      error4:
	delete_semaphore( psVol->v_hMutex );
	UNLOCK( g_hInodeHashSem );
      error3:
	kfree( psVol );
      error2:
	put_inode( psDir );
      error1:
	return ( nError );
}

int sys_mount( const char *pzDevName, const char *pzDirName, const char *pzFSName, int nFlags, const void *pData )
{
	static atomic_t nVolNum = ATOMIC_INIT( 124 );

	return ( do_mount( pzDevName, pzDirName, pzFSName, nFlags, atomic_inc_and_read( &nVolNum ), pData ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool umount_flush_inode( Inode_s *psInode )
{
	FSOperations_s *psOps = psInode->i_psOperations;

	if ( atomic_read( &psInode->i_nCount ) == 0 )
	{
		remove_from_list( &g_sUsedList, psInode );
		psInode->i_bBusy = true;
		UNLOCK( g_hInodeHashSem );

		if ( NULL != psOps->write_inode )
		{
			if ( psOps->write_inode( psInode->i_psVolume->v_pFSData, psInode->i_pFSData ) < 0 )
			{
				printk( "Panic : failed to write inode %d::%Ld\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode );
			}
		}
		LOCK( g_hInodeHashSem );
		atomic_dec( &psInode->i_psVolume->v_nOpenCount );

		hash_delete_inode( &g_sHashTable, psInode );
		clear_inode( psInode );
		psInode->i_bBusy = false;
		add_to_head( &g_sFreeList, psInode );


		return ( false );
	}
	else
	{
		psInode->i_bBusy = true;
		psInode->i_psOperations = &g_sDummyOperations;
		atomic_inc( &psInode->i_nCount );
		UNLOCK( g_hInodeHashSem );

		if ( psOps->close != NULL )
		{
			while ( psInode->i_psFirstFile != NULL )
			{
				File_s *psFile = psInode->i_psFirstFile;

				printk( "umount_flush_inode() close file\n" );
				psOps->close( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psFile->f_pFSCookie );
				psFile->f_pFSCookie = NULL;
				remove_file_from_inode( psInode, psFile );
			}
		}

		if ( NULL != psOps->write_inode )
		{
			if ( psOps->write_inode( psInode->i_psVolume->v_pFSData, psInode->i_pFSData ) < 0 )
			{
				printk( "Panic : umount_flush_inode() failed to write inode %d::%Ld\n", psInode->i_psVolume->v_nDevNum, psInode->i_nInode );
			}
		}
		psInode->i_bBusy = false;
		put_inode( psInode );
		LOCK( g_hInodeHashSem );
		return ( true );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool flush_volume_inodes( Volume_s *psVolume )
{
	bool bNodesFound = false;

	for ( ;; )
	{
		Inode_s *psInode = NULL;
		int i;

		for ( i = 0; i < g_sHashTable.ht_nSize && psInode == NULL; ++i )
		{
			for ( psInode = g_sHashTable.ht_apsTable[i]; psInode != NULL; psInode = psInode->i_psHashNext )
			{
				if ( psInode->i_psVolume == psVolume && psInode->i_nInode != psVolume->v_nRootInode )
				{
					if ( psInode->i_psOperations != &g_sDummyOperations )
					{
						break;
					}
				}
			}
		}
		if ( psInode == NULL )
		{
			return ( bNodesFound );
		}
		umount_flush_inode( psInode );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool check_fs_open_files( Volume_s *psVolume )
{
	Inode_s *psInode = NULL;
	int i;

	for ( i = 0; i < g_sHashTable.ht_nSize && psInode == NULL; ++i )
	{
		for ( psInode = g_sHashTable.ht_apsTable[i]; psInode != NULL; psInode = psInode->i_psHashNext )
		{
			if ( psInode->i_psVolume == psVolume && atomic_read( &psInode->i_nCount ) > 0 && ( psInode->i_nInode != psVolume->v_nRootInode || atomic_read( &psInode->i_nCount ) > 1 ) )
			{
				return ( true );
			}
		}
	}
	return ( false );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_unmount( const char *pzPath, bool bForce )
{
	Inode_s *psInode;
	Inode_s *psMountPnt;
	Volume_s *psVol;
	Volume_s **ppsTmp;
	int nError;
	bool bHasOpenFiles;

	nError = get_named_inode( NULL, pzPath, &psInode, false, false );

	if ( nError < 0 )
	{
		goto error1;
	}

	LOCK( g_hInodeHashSem );

	if ( NULL == psInode->i_psMount )
	{
		nError = -EINVAL;
		UNLOCK( g_hInodeHashSem );
		goto error2;
	}
	psMountPnt = psInode->i_psMount;
	psVol = psMountPnt->i_psVolume;

	if ( psVol->v_bUnmounted )
	{
		nError = -EINVAL;
		UNLOCK( g_hInodeHashSem );
		printk( "Error: sys_unmount() filesystem already unmounted\n" );
		goto error2;
	}
	if ( psVol->v_psOperations->unmount == NULL )
	{
		nError = -ENOSYS;
		UNLOCK( g_hInodeHashSem );
		goto error2;
	}

	psVol->v_bUnmounted = true;	// make sure nobody loads more inodes from this file system
	psInode->i_bBusy = true;	// make sure nobody follows the mount-point when we unlock.

	UNLOCK( g_hInodeHashSem );

	nError = LOCK_INODE_RW( psMountPnt );

	if ( nError < 0 )
	{
		goto error3;
	}

	LOCK( g_hInodeHashSem );


	bHasOpenFiles = check_fs_open_files( psVol );


	if ( bForce == false && bHasOpenFiles )
	{
		nError = -EBUSY;
		printk( "sys_unmount() Filesystem has open files, can't unmount\n" );
		UNLOCK( g_hInodeHashSem );
		goto error4;
	}

	flush_volume_inodes( psVol );
	kassertw( atomic_read( &psMountPnt->i_nCount ) == 1 );

	psInode->i_psMount = NULL;
	atomic_dec( &psMountPnt->i_nCount );
	umount_flush_inode( psMountPnt );

	UNLOCK( g_hInodeHashSem );

	if ( bHasOpenFiles )
	{
		printk( "Force-unmount filesystem while %d files are open\n", atomic_read( &psVol->v_nOpenCount ) );
	}

	nError = psVol->v_psOperations->unmount( psVol->v_pFSData );
	psInode->i_bBusy = false;

	LOCK( g_hInodeHashSem );

	for ( ppsTmp = &g_psFirstVolume; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->v_psNext )
	{
		if ( *ppsTmp == psVol )
		{
			*ppsTmp = psVol->v_psNext;
			break;
		}
	}
	UNLOCK( g_hInodeHashSem );
	release_fs( psVol->v_psFSDesc );

	atomic_dec( &psInode->i_nCount );	// release the mount point
	put_inode( psInode );	// reverse the get_inode() above.
	if ( atomic_read( &psVol->v_nOpenCount ) == 0 )
	{
		delete_semaphore( psVol->v_hMutex );
		kfree( psVol );
	}
	else
	{
		unlock_semaphore_ex( psVol->v_hMutex, VOLUME_MUTEX_COUNT );
	}
	return ( 0 );
      error4:
	UNLOCK_INODE_RW( psMountPnt );
      error3:
	psVol->v_bUnmounted = false;
	psInode->i_bBusy = false;
      error2:
	put_inode( psInode );	// reverse the get_inode() above.
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void dump_tables( int argc, char **argv )
{
	bool bListFree = ( argc == 1 );
	bool bListUsed = ( argc == 1 );
	bool bListHash = ( argc == 1 );
	int i;

	for ( i = 1; i < argc; ++i )
	{
		if ( strcmp( argv[i], "free" ) == 0 )
		{
			bListFree = true;
		}
		else if ( strcmp( argv[i], "used" ) == 0 )
		{
			bListUsed = true;
		}
		else if ( strcmp( argv[i], "hash" ) == 0 )
		{
			bListHash = true;
		}
		else if ( strcmp( argv[i], "all" ) == 0 )
		{
			bListFree = true;
			bListUsed = true;
			bListHash = true;
		}
	}

	if ( bListFree )
	{
		dbprintf( DBP_DEBUGGER, "Free inodes:\n" );
		dump_list( &g_sFreeList );
	}
	if ( bListUsed )
	{
		dbprintf( DBP_DEBUGGER, "Used inodes:\n" );
		dump_list( &g_sUsedList );
	}
	if ( bListHash )
	{
		dbprintf( DBP_DEBUGGER, "Locked inodes:\n" );
		dump_hash_table( &g_sHashTable );
	}

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void shutdown_vfs( void )
{
	printk( "Unmounting file systems...\n" );

	while ( NULL != g_psFirstVolume )
	{
		Volume_s *psVol = g_psFirstVolume;
		
		g_psFirstVolume = psVol->v_psNext;

		if ( NULL != psVol->v_psOperations )
		{
			if ( NULL != psVol->v_psOperations->unmount )
			{
				if ( NULL == psVol->v_psMountPoint )
				{
					printk( "PANIC : volume has no mount point!!\n" );
					continue;
				}
				if ( NULL == psVol->v_psMountPoint->i_psMount )
				{
					printk( "PANIC : volume points to a mount point where i_psMount == NULL!!\n" );
					continue;
				}
				
				psVol->v_bUnmounted = true;
				flush_volume_inodes( psVol );
		
				atomic_set( &psVol->v_psMountPoint->i_psMount->i_nCount, 0 );
				umount_flush_inode( psVol->v_psMountPoint->i_psMount );
				
				put_inode( psVol->v_psMountPoint );	// release the mount point
				psVol->v_psMountPoint->i_psMount = NULL;		
				psVol->v_psOperations->unmount( psVol->v_pFSData );
			}
		}
	}
	printk( "File systems unmounted\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_vfs_module( void )
{
	printk( "Init virtual file system\n" );

	memset( &g_sDummyOperations, 0, sizeof( g_sDummyOperations ) );

	g_hInodeHashSem = create_semaphore( "vfs_mutex", 1, SEM_RECURSIVE );
	g_hFSListSema = create_semaphore( "fs_driver_list_mutex", 1, SEM_RECURSIVE );
	g_sFreeList.il_psLRU = NULL;
	g_sFreeList.il_psMRU = NULL;

	g_sUsedList.il_psLRU = NULL;
	g_sUsedList.il_psMRU = NULL;

	init_hash_table( &g_sHashTable );
	init_vfs();

	mount_root_fs();
	init_dev_fs();

	mkdir( "/dev", 0 );
	do_mount( "", "/dev", "_device_fs", 0, FSID_DEV, NULL );
	init_fifo();
	init_pty();

	mkdir( "/dev/pty", 0 );
	do_mount( "", "/dev/pty", "_pty_device_", 0, FSID_PTY, NULL );

	register_debug_cmd( "dump_ino_tab", "list inodes in used/free/hash tables", dump_tables );
}

