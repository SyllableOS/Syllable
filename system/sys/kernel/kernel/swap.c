
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

#include <posix/errno.h>
#include <posix/fcntl.h>
#include <posix/unistd.h>
#include <posix/stat.h>

#include <atheos/time.h>

#include <atheos/types.h>

#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/mman.h"
#include "inc/areas.h"
#include "inc/swap.h"
#include "vfs/vfs.h"

#define	NUM_SWAP_PAGES	(1024 * 1024 * 64 / PAGE_SIZE)

static uint16 g_anSwapInfo[NUM_SWAP_PAGES];
static int g_anAgeCounts[256];
static File_s *g_psSwapFile;
static int g_nTotalSwapPages;
static int g_nFreeSwapPages;
static int g_nPageIn = 0;
static int g_nPageOut = 0;

typedef struct
{
	MemArea_s *sn_psArea;
	pte_t *sn_psPte;
	uint32 sn_nAddress;
	Page_s *sn_psPage;
} SwapNode_s;

typedef struct
{
	off_t nStart;
	int nLen;
} SFBlockRun_s;

static SFBlockRun_s *g_pasSwapFileBlocks;
static int g_nSwapDev = -1;
static int g_nBlockRunCount = 0;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static uint32 alloc_swap_page( void )
{
	int i;

	for ( i = 1; i < NUM_SWAP_PAGES; ++i )
	{
		if ( g_anSwapInfo[i] == 0 )
		{
			g_anSwapInfo[i] = 1;
			g_nFreeSwapPages--;
			if ( g_nFreeSwapPages < 0 )
			{
				printk( "PANIC: alloc_swap_page() Got %d of %ld swap pages free!\n", g_nFreeSwapPages, NUM_SWAP_PAGES );
			}
			return ( i << 12 );
		}
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void dup_swap_page( int nPage )
{
	nPage = nPage & ~0x80000000;
	nPage >>= 12;
//  printk( "dup swap page %d\n", nPage );

	if ( nPage >= NUM_SWAP_PAGES )
	{
		printk( "PANIC: dup_swap_page() attempt to dup page %d while only %ld pages available!\n", nPage, NUM_SWAP_PAGES );
		return;
	}
	if ( g_anSwapInfo[nPage] == 0 )
	{
		printk( "PANIC: dup_swap_page() attempt to dup unused page %d\n", nPage );
	}
	g_anSwapInfo[nPage]++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void free_swap_page( int nPage )
{
	nPage = nPage & ~0x80000000;
	nPage >>= 12;
	if ( nPage >= NUM_SWAP_PAGES )
	{
		printk( "PANIC: free_swap_page() attempt to free page %d while only %ld pages available!\n", nPage, NUM_SWAP_PAGES );
		return;
	}
	if ( g_anSwapInfo[nPage] == 0 )
	{
		printk( "PANIC: free_swap_page() attempt to free unused page %d\n", nPage );
		return;
	}

	g_anSwapInfo[nPage]--;

	if ( g_anSwapInfo[nPage] == 0 )
	{
		g_nFreeSwapPages++;
		if ( g_nFreeSwapPages > g_nTotalSwapPages )
		{
			printk( "PANIC: free_swap_page() Got %d of %d swap pages free!\n", g_nFreeSwapPages, g_nTotalSwapPages );
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void register_swap_page( uint32 nAddress )
{
	Page_s *psPage = &g_psFirstPage[nAddress >> PAGE_SHIFT];

	psPage->p_nAge = INITIAL_PAGE_AGE;

	g_anAgeCounts[psPage->p_nAge]++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void unregister_swap_page( uint32 nAddress )
{
	Page_s *psPage = &g_psFirstPage[nAddress >> PAGE_SHIFT];

	g_anAgeCounts[psPage->p_nAge]--;

//  kassertw( g_anAgeCounts[psPage->p_nAge] >= 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static inline void touch_page( Page_s *psPage )
{
	g_anAgeCounts[psPage->p_nAge]--;
//  kassertw( g_anAgeCounts[psPage->p_nAge] >= 0 );
	if ( psPage->p_nAge < ( MAX_PAGE_AGE - PAGE_ADVANCE ) )
	{
		psPage->p_nAge += PAGE_ADVANCE;
	}
	else
	{
		psPage->p_nAge = MAX_PAGE_AGE;
	}
	g_anAgeCounts[psPage->p_nAge]++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static inline void age_page( Page_s *psPage )
{
	g_anAgeCounts[psPage->p_nAge]--;
//  kassertw( g_anAgeCounts[psPage->p_nAge] >= 0 );
	if ( psPage->p_nAge > PAGE_DECLINE )
	{
		psPage->p_nAge -= PAGE_DECLINE;
	}
	else
	{
		psPage->p_nAge = 0;
	}
	g_anAgeCounts[psPage->p_nAge]++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void find_swap_block( int nOffset, off_t *pnStart, int *pnSize )
{
	int nPos = 0;
	int i;


	for ( i = 0; i < g_nBlockRunCount; ++i )
	{
		if ( nOffset < nPos + g_pasSwapFileBlocks[i].nLen )
		{
			*pnStart = g_pasSwapFileBlocks[i].nStart + ( nOffset - nPos );
			*pnSize = g_pasSwapFileBlocks[i].nLen - ( nOffset - nPos );
			return;
		}
		nPos += g_pasSwapFileBlocks[i].nLen;
	}
	panic( "find_swap_block() failed to find block for offset %d\n", nOffset );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int swap_out( pte_t * pPte, Page_s *psPage, uint32 nAddress )
{
	uint32 nPageAddress = PTE_PAGE( *pPte );
	int nSwapPage;
	int nOffset;
	int nBlocksLeft;
	char *pBuffer;
	
	nSwapPage = alloc_swap_page();

  //printk( "swap out %d (%08x) of %i\n", nSwapPage, nAddress, g_nFreeSwapPages );

	if ( nSwapPage == 0 )
	{
		printk( "PANIC: out of swap space :(\n" );
		return ( -ENOMEM );
	}

	g_nPageOut++;

	nOffset = nSwapPage / 1024;
	nBlocksLeft = PAGE_SIZE / 1024;

	pBuffer = ( char * )( psPage->p_nPageNum * PAGE_SIZE );
	while ( nBlocksLeft > 0 )
	{
		off_t nRunStart = 0;
		int nRunLength = 0;
		int nBlockCount = 0;

		find_swap_block( nOffset, &nRunStart, &nRunLength );

		nBlockCount = min( nBlocksLeft, nRunLength );

    //printk( "write %d blocks at %d\n", nBlockCount, (int)nRunStart  );

		write_phys_blocks( g_nSwapDev, nRunStart, pBuffer, nBlockCount, 1024 );
		nBlocksLeft -= nBlockCount;
		pBuffer += nBlockCount * 1024;
		nOffset += nBlockCount;
	}
	
	PTE_VALUE( *pPte ) = ( 0x80000000 | nSwapPage | ( PTE_VALUE( *pPte ) & ~PAGE_MASK ) ) & ~PTE_PRESENT;

	free_pages( nPageAddress, 1 );

	g_anAgeCounts[psPage->p_nAge]--;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int swap_in( pte_t * pPte )
{
	uint32 nNewPage = get_free_pages( 1, MEMF_CLEAR );
	Page_s *psPage = &g_psFirstPage[nNewPage >> PAGE_SHIFT];
	int nOffset;
	int nBlocksLeft;
	char *pBuffer;
	int nSwapPage = PTE_PAGE( *pPte ) & ~0x80000000;
	

  //printk( "swap in %d (%08x)\n", nSwapPage, nNewPage );
  
	if ( nNewPage == 0 )
	{
		printk( "PANIC: swap_in() out of memory\n" );
		return( -ENOMEM );
	}

	g_nPageIn++;

	nOffset = nSwapPage / 1024;
	nBlocksLeft = PAGE_SIZE / 1024;

	pBuffer = ( char * )( psPage->p_nPageNum * PAGE_SIZE );
	
	
	
	while ( nBlocksLeft > 0 )
	{
		off_t nRunStart = 0;
		int nRunLength = 0;
		int nBlockCount;
		
		find_swap_block( nOffset, &nRunStart, &nRunLength );
		
		nBlockCount = min( nBlocksLeft, nRunLength );

    //printk( "read %d blocks at %d\n", nBlockCount, (int)nRunStart );
    	UNLOCK( g_hAreaTableSema );
		read_phys_blocks( g_nSwapDev, nRunStart, pBuffer, nBlockCount, 1024 );
		LOCK( g_hAreaTableSema );
		
		nBlocksLeft -= nBlockCount;
		pBuffer += nBlockCount * 1024;
		nOffset += nBlockCount;
	}

	PTE_VALUE( *pPte ) = nNewPage | ( PTE_VALUE( *pPte ) & ~PAGE_MASK ) | PTE_WRITE | PTE_PRESENT;

	free_swap_page( nSwapPage );

	psPage->p_nAge = INITIAL_PAGE_AGE;
	g_anAgeCounts[psPage->p_nAge]++;
	
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static int swap_tick( void )
{
	static int nAreaID = -1;
	static uint32 nOffset = 0;
	MemArea_s *psArea;
	Page_s *psPage;
	pgd_t *pPgd;
	pte_t *pPte;
	uint32 nLinAddress;
	int nPageAddr;
	int i = 0;
	bool bNewArea = false;
	
	do
	{
		LOCK( g_hAreaTableSema );
		psArea = get_area_from_handle( nAreaID );
		UNLOCK( g_hAreaTableSema );

		if ( NULL == psArea || ( nOffset > psArea->a_nEnd - psArea->a_nStart ) /*|| psArea->a_psNextShared != psArea */  )
		{
			if ( NULL != psArea )
			{
				put_area( psArea );
				psArea = NULL;
			}
			nAreaID = get_next_area( nAreaID );
			bNewArea = true;
			nOffset = 0;
		}
	}
	while ( psArea == NULL && ++i < ( 1 << 24 ) );

	if ( psArea == NULL )
	{
		printk( "PANIC: swap_tick() no areas to flush :(\n" );
		return ( 0 );
	}
  if ( bNewArea ) {
    /*printk( "Check area %d (%08x-%08x) (%d)\n", nAreaID, psArea->a_nStart, psArea->a_nEnd, psArea->a_nRefCount );
  	if ( psArea->a_nProtection & AREA_REMAPPED )
  		printk( "Remapped\n" );
  	if( psArea->a_nLockMode & AREA_FULL_LOCK )
  		printk( "Full lock\n" );
  	if( ( psArea->a_nProtection & AREA_KERNEL ) == 0 )
  		printk( "User \n" );*/
  }
  
	LOCK( g_hAreaTableSema );
  
	nLinAddress = psArea->a_nStart + nOffset;
	pPgd = pgd_offset( psArea->a_psContext, nLinAddress );
	pPte = pte_offset( pPgd, nLinAddress );
	nOffset += PAGE_SIZE;

	if ( psArea->a_nProtection & AREA_REMAPPED )
	{
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( 0 );
	}
	if ( psArea->a_nLockMode & AREA_FULL_LOCK )
	{
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( 0 );
	}
	if ( PTE_ISPRESENT( *pPte ) == 0 )
	{
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( 0 );
	}

	nPageAddr = PTE_PAGE( *pPte );

	if ( nPageAddr == ( uint32 )g_sSysBase.ex_pNullPage )
	{
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( 0 );
	}

	psPage = &g_psFirstPage[nPageAddr >> PAGE_SHIFT];

	if ( PTE_ISACCESSED( *pPte ) )
	{
		touch_page( psPage );
		PTE_VALUE( *pPte ) &= ~PTE_ACCESSED;
	}
	age_page( psPage );

	if ( psPage->p_nCount != 1 )
	{
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( 0 );
	}

	if ( psPage->p_nAge == 0 )
	{
		UNLOCK( g_hAreaTableSema );
   //printk( "Swap out page %d\n", psPage->p_nPageNum );
    	psPage->p_nAge = INITIAL_PAGE_AGE;
//    swap_out( pPte, psPage, nLinAddress );
		put_area( psArea );
		return ( 1 );
	}
	UNLOCK( g_hAreaTableSema );

	put_area( psArea );
	return ( 0 );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void insert_swap_node( SwapNode_s * apsList, MemArea_s *psArea, Page_s *psPage, pte_t * psPte, uint32 nAddress )
{
	int i;

	if ( apsList[15].sn_psPage != NULL && apsList[15].sn_psPage->p_nAge <= psPage->p_nAge )
	{
		return;
	}

	for ( i = 0; i < 16; ++i )
	{
		if ( apsList[i].sn_psPage == NULL || psPage->p_nAge <= apsList[i].sn_psPage->p_nAge )
		{
			memmove( &apsList[i + 1], &apsList[i], ( 15 - i ) * sizeof( SwapNode_s ) );
			apsList[i].sn_psArea = psArea;
			apsList[i].sn_psPte = psPte;
			apsList[i].sn_nAddress = nAddress;
			apsList[i].sn_psPage = psPage;
			break;
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int swap_out_pages( int nCount )
{
	SwapNode_s asSwapList[16];
	area_id nAreaID;
	int i;
	bigtime_t nStartTime;
	bigtime_t nEndTime;
	int nSwappedOut = 0;
	
	memset( asSwapList, 0, sizeof( SwapNode_s ) * 16 );

	nStartTime = get_real_time();

	for ( nAreaID = 0; nAreaID != -1 && nCount >= 0; nAreaID = get_next_area( nAreaID ) )
	{
		MemArea_s *psArea;
		uint32 nAddress;
		LOCK( g_hAreaTableSema );
		psArea = get_area_from_handle( nAreaID );
		

		if ( psArea == NULL )
		{
			UNLOCK( g_hAreaTableSema );
			continue;
		}

		if ( psArea->a_nProtection & AREA_REMAPPED )
		{
			UNLOCK( g_hAreaTableSema );
			put_area( psArea );
			continue;
		}
		
		if ( psArea->a_nProtection & AREA_KERNEL )
		{
			UNLOCK( g_hAreaTableSema );
			put_area( psArea );
			continue;
		}
		
		
		if ( psArea->a_nLockMode & AREA_FULL_LOCK )
		{
			UNLOCK( g_hAreaTableSema );
			put_area( psArea );
			continue;
		}
		
		if( psArea->a_psNextShared != psArea || psArea->a_psPrevShared != psArea )
		{
			UNLOCK( g_hAreaTableSema );
			put_area( psArea );
			continue;
		}
		
		if( psArea->a_psFile != NULL )
		{
			UNLOCK( g_hAreaTableSema );
			put_area( psArea );
			continue;
		}
		
		for ( nAddress = psArea->a_nStart; ( nAddress - 1 ) < psArea->a_nEnd && nCount >= 0; nAddress += PAGE_SIZE )
		{
			pgd_t *pPgd;
			pte_t *pPte;
			uint32 nPageAddr;
			Page_s *psPage;

			pPgd = pgd_offset( psArea->a_psContext, nAddress );
			pPte = pte_offset( pPgd, nAddress );


			if ( PTE_ISPRESENT( *pPte ) == false )
			{
				continue;
			}

			nPageAddr = PTE_PAGE( *pPte );

			if ( nPageAddr == ( uint32 )g_sSysBase.ex_pNullPage )
			{
				continue;
			}

			psPage = &g_psFirstPage[nPageAddr >> PAGE_SHIFT];

			if ( PTE_ISACCESSED( *pPte ) )
			{
				touch_page( psPage );
				PTE_VALUE( *pPte ) &= ~PTE_ACCESSED;
			}
			age_page( psPage );

			if ( atomic_read( &psPage->p_nCount ) != 1 )
			{
				continue;
			}
			insert_swap_node( asSwapList, psArea, psPage, pPte, nAddress );
			

			if ( psPage->p_nAge == 0 )
			{
				        //printk( "Swap out page %d Addr = %08x (%08x-%08x)\n",
                //psPage->p_nPageNum, nAddress, psArea->a_nStart, psArea->a_nEnd );

//        swap_out( pPte, psPage, nAddress );
				nCount--;
			}
		}
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
	}
	nEndTime = get_real_time();

	if ( asSwapList[0].sn_psPage == NULL )
	{
		printk( "PANIC: No pages to swap out\n" );
		snooze( 1000000 );
		
		return( 0 );
	}

	for ( i = 0; i < 16; ++i )
	{
		if ( asSwapList[i].sn_psPage != NULL )
		{
			nSwappedOut++;
			swap_out( asSwapList[i].sn_psPte, asSwapList[i].sn_psPage, asSwapList[i].sn_nAddress );
		}
	}
	
	flush_tlb_global();
	
	//printk( "%i pages swapped out", nSwappedOut );
		
	return( nSwappedOut );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static int age_daemon( void *pData )
{
	for ( ;; )
	{
		int j;

		for( j = 0; j < 10; j++ )
		{
			if( swap_tick() > 0 )
				break;
		}
		
		snooze( 10000 );
		handle_signals( 0 );
	}
	return( 0 );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int swap_daemon( void *pData )
{
//  int i = 0;

	for ( ;; )
	{
		if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) < 2000 )
		{
			shrink_caches( PAGE_SIZE );
		}
		if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) < 2000 )
		{
			swap_out_pages( 16 );
		}
		if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) > 500 )
		{
			snooze( 100000 );
		}
		handle_signals( 0 );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_list_swap_counts( int argc, char **argv )
{
	int i;

	dbprintf( DBP_DEBUGGER, "Age counts:\n" );
	for ( i = 0; i <= MAX_PAGE_AGE; ++i )
	{
		dbprintf( DBP_DEBUGGER, "%02d -> %04d\n", i, g_anAgeCounts[i] );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_swap_blocks( File_s *psFile, int nSize )
{
	int nBlockCount = nSize / 1024;
	int nActualCount;
	int nError = -EINVAL;

//  int    nRunCount = 0;
	off_t nPos = 0;
	int i;

	off_t nStart;

	g_nBlockRunCount = 0;

	while ( nBlockCount > 0 )
	{
		nError = get_file_blocks_p( psFile, nPos, nBlockCount, &nStart, &nActualCount );
		if ( nError < 0 )
		{
			printk( "Error1: failed to get swap-file blocks (Err=%d)\n", nError );
			return ( nError );
		}

		nBlockCount -= nActualCount;
		nPos += nActualCount;

		if ( nBlockCount < 0 )
		{
			printk( "PANIC1: get_file_blocks_p() returned more blocks than requested! (%d)\n", nBlockCount );
		}
		g_nBlockRunCount++;
	}
	printk( "Swapfile consist of %d fragments\n", g_nBlockRunCount );

	g_pasSwapFileBlocks = kmalloc( g_nBlockRunCount * sizeof( SFBlockRun_s ), MEMF_KERNEL );

	if ( g_pasSwapFileBlocks == NULL )
	{
		printk( "get_swap_blocks() no memory for block table\n" );
	}

	nBlockCount = nSize / 1024;
	nPos = 0;

	for ( i = 0; i < g_nBlockRunCount; ++i )
	{
		nError = get_file_blocks_p( psFile, nPos, nBlockCount, &g_pasSwapFileBlocks[i].nStart, &g_pasSwapFileBlocks[i].nLen );
		if ( nError < 0 )
		{
			printk( "Error2: failed to get swap-file blocks (Err=%d)\n", nError );
			return ( nError );
		}
		nBlockCount -= g_pasSwapFileBlocks[i].nLen;
		nPos += g_pasSwapFileBlocks[i].nLen;

		if ( nBlockCount < 0 )
		{
			printk( "PANIC2: get_file_blocks_p() returned more blocks than requested! (%d)\n", nBlockCount );
		}
	}
	if ( nBlockCount != 0 )
	{
		panic( "get_swap_blocks() : Seccond pass did not returned same number of blocks as first! (%d)\n", nBlockCount );
		return ( -EIO );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_swap_info( SwapInfo_s * psInfo )
{
	psInfo->si_nTotSize = g_nTotalSwapPages * PAGE_SIZE;
	psInfo->si_nFreeSize = g_nFreeSwapPages * PAGE_SIZE;
	psInfo->si_nPageIn = g_nPageIn;
	psInfo->si_nPageOut = g_nPageOut;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_swapper( void )
{
	int nSwapFile;
	thread_id hThread;
	
	//hThread = spawn_kernel_thread( "ager", age_daemon, 0, DEFAULT_KERNEL_STACK, NULL );
	//wakeup_thread( hThread, false );

	hThread = spawn_kernel_thread( "swapper", swap_daemon, 100, DEFAULT_KERNEL_STACK, NULL );
	wakeup_thread( hThread, false );

	register_debug_cmd( "lspc", "list number of memory pages in each age group.", db_list_swap_counts );
	
	nSwapFile = open( "/boot/syllable.swp", O_RDWR | O_CREAT );
	kassertw( nSwapFile >= 0 );



	g_psSwapFile = get_fd( true, nSwapFile );
	g_nTotalSwapPages = NUM_SWAP_PAGES;
	g_nFreeSwapPages = NUM_SWAP_PAGES;

	lseek( nSwapFile, NUM_SWAP_PAGES * PAGE_SIZE, SEEK_SET );	// One byte to big so we avoids marking any swap-blocks dirty
	kassertw( write( nSwapFile, "", 1 ) == 1 );	// Expand the swap file

	g_nSwapDev = get_swap_blocks( g_psSwapFile, NUM_SWAP_PAGES * PAGE_SIZE );
	
}


























































