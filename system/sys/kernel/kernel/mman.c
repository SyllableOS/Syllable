
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

#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/bcache.h>

#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <macros.h>

#include "inc/sysbase.h"
#include "inc/bcache.h"
#include "inc/areas.h"
#include "inc/swap.h"

Page_s *g_psFirstPage;
Page_s *g_psFirstFreePage = NULL;

//spinlock_t  g_nPageListSpinLock = 0;
SPIN_LOCK( g_sPageListSpinLock, "page_list_slock" );

//extern sem_id g_hAreaTableSema;

int g_nAllocatedPages = 0;

/*
void lock_pagelist( void )
{
	spinlock( &g_sPageListSpinLock );
}

void unlock_pagelist( void )
{
	spinunlock( &g_sPageListSpinLock );
}

uint32	get_free_pages( int nPageCount, int nFlags )
{
  int i = 0;
  int j = 0;
  int nStart = -1;
  
  for ( ; i < g_sSysBase.ex_nTotalPageCount / 32 ;  ) {
    
    if ( g_panMemPageBitmap[i] == ~0 ) {
      continue;
    }
    uint32 nMask = 1;
    for ( ; j < 32 ; ++j, nMask <<= 1 ) {
      if ( (g_panMemPageBitmap[i] & nMask) == 0 ) {
	nStart = i * 32 + j;
	goto find_end;
      }
    }
    j = 0;
  }
  return( 0 );
find_end:
  for ( ; i < g_sSysBase.ex_nTotalPageCount / 32 && nSize < nPageCount ;  ) {

    if ( g_panMemPageBitmap[i] == 0 ) {
      nSize += 32;
      continue;
    }
    uint32 nMask = 1 << j;
    for ( ; j < 32 ; ++j, nMask <<= 1 ) {
      if ( (g_panMemPageBitmap[i] & nMask) ) {
	break;
      }
      nSize++;
    }
  }
}
*/

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Page_s *get_page_desc( int nPageNum )
{
	return ( &g_psFirstPage[nPageNum] );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int32 get_free_page( int nFlags )
{
	return ( get_free_pages( 1, nFlags ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void validate_page_list( void )
{
	int i;

	for ( i = 0; i < g_sSysBase.ex_nTotalPageCount - 1; ++i )
	{
		int nCount = atomic_read( &g_psFirstPage[i].p_nCount );
		if ( nCount < 0 )
		{
			printk( "PANIC Page %d got reference count of %d\n", i, nCount );
		}
		if ( 0 == nCount && NULL == g_psFirstPage[i].p_psNext )
		{
			printk( "Page %d got reference count of 0, but does not belong to the freelist!!\n", i );
		}
		if ( 0 != nCount && NULL != g_psFirstPage[i].p_psNext )
		{
			printk( "Page %d got reference count of %d, while present at the freelist!!\n", i, nCount );
		}
		kassertw( g_psFirstPage[i].p_nPageNum == i );
	}
}

void protect_phys_pages( iaddr_t nAddress, int nCount )
{
	int i;

	for ( i = 0; i < nCount; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, nAddress );
		pte_t *pPte = pte_offset( pPgd, nAddress );

		PTE_VALUE( *pPte ) &= ~PTE_PRESENT;
		nAddress += PAGE_SIZE;
	}
}

void unprotect_phys_pages( iaddr_t nAddress, int nCount )
{
	int i;

	for ( i = 0; i < nCount; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, nAddress );
		pte_t *pPte = pte_offset( pPgd, nAddress );

		PTE_VALUE( *pPte ) |= PTE_PRESENT;
		nAddress += PAGE_SIZE;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 get_free_pages( int nPageCount, int nFlags )
{
	Page_s *psPage;
	Page_s **ppsStart;
	Page_s *psPrev;
	uint32 nPage = 0;
	uint32 nEFlg;

	nEFlg = spinlock_disable( &g_sPageListSpinLock );

	if ( NULL != g_psFirstFreePage )
	{
		if ( 1 == nPageCount )
		{
			psPage = g_psFirstFreePage;

			g_psFirstFreePage = psPage->p_psNext;
			psPage->p_psNext = NULL;

			kassertw( 0 == atomic_read( &psPage->p_nCount ) );

			atomic_inc( &psPage->p_nCount );
			g_nAllocatedPages++;
			atomic_dec( &g_sSysBase.ex_nFreePageCount );

			nPage = psPage->p_nPageNum * PAGE_SIZE;
		}
		else
		{
//      static int nCalls = 0;
//      static int nTotLoops = 0;

//      nCalls++;

			psPrev = g_psFirstFreePage;
			ppsStart = &g_psFirstFreePage;

			for ( psPage = g_psFirstFreePage->p_psNext; NULL != psPage; psPage = psPage->p_psNext )
			{
//      nTotLoops++;
				if ( ( psPage - psPrev ) > 1 )
				{
					ppsStart = &psPrev->p_psNext;
				}
				else
				{
					if ( ( psPage - ( *ppsStart ) ) >= nPageCount - 1 )
					{
						Page_s *psTmp;

						psTmp = *ppsStart;

						nPage = ( *ppsStart )->p_nPageNum * PAGE_SIZE;
						*ppsStart = psPage->p_psNext;

						for ( ; psTmp <= psPage; psTmp++ )
						{
							if ( 0 != atomic_read( &psTmp->p_nCount ) )
							{
								printk( "Page %d present on free list with count of %d\n", psTmp->p_nPageNum, atomic_read( &psTmp->p_nCount ) );
							}
							atomic_inc( &psTmp->p_nCount );
							psTmp->p_psNext = NULL;
							g_nAllocatedPages++;
							atomic_dec( &g_sSysBase.ex_nFreePageCount );
						}
						break;
					}
				}
				psPrev = psPage;
			}
		}
	}
	if ( 0 != nPage )
	{
		unprotect_phys_pages( nPage, nPageCount );
	}
	spinunlock_enable( &g_sPageListSpinLock, nEFlg );

	if ( 0 != nPage )
	{
		memset( ( void * )nPage, 0, PAGE_SIZE * nPageCount );
	}
	flush_tlb_global();
	
	return ( nPage );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_free_pages( uint32 nPage, int nCount )
{
	Page_s *psPage = &g_psFirstPage[nPage >> PAGE_SHIFT];
	Page_s **ppsNext = NULL;
	uint32 nEFlg;
	int i;

	nEFlg = spinlock_disable( &g_sPageListSpinLock );

	for ( i = 0; i < nCount; ++i, ++psPage, nPage += PAGE_SIZE )
	{
		atomic_dec( &psPage->p_nCount );

		kassertw( atomic_read( &psPage->p_nCount ) >= 0 );

		if ( 0 == atomic_read( &psPage->p_nCount ) )
		{
			g_nAllocatedPages--;
			atomic_inc( &g_sSysBase.ex_nFreePageCount );
			protect_phys_pages( nPage, 1 );

			if ( ppsNext == NULL )
			{
				if ( g_psFirstFreePage == NULL || psPage < g_psFirstFreePage )
				{
					psPage->p_psNext = g_psFirstFreePage;
					g_psFirstFreePage = psPage;
					ppsNext = &psPage->p_psNext;
				}
				else
				{
					int j = psPage->p_nPageNum - 1;
					Page_s *psPrev = &g_psFirstPage[j];

					for ( ; j >= 0; --j, --psPrev )
					{
						if ( 0 == atomic_read( &psPrev->p_nCount ) )
						{
							psPage->p_psNext = psPrev->p_psNext;
							psPrev->p_psNext = psPage;
							ppsNext = &psPage->p_psNext;
							break;
						}
					}
					if ( j < 0 )
					{
						printk( "PANIC : free_page() failed to link page %p to free list\n", ( void * )nPage );
					}
				}
			}
			else
			{
				psPage->p_psNext = *ppsNext;
				*ppsNext = psPage;
				ppsNext = &psPage->p_psNext;
			}
		}
	}
	spinunlock_enable( &g_sPageListSpinLock, nEFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void free_pages( uint32 nPage, int nCount )
{
	do_free_pages( nPage, nCount );
	flush_tlb_global();
}

int shrink_caches( int nBytesNeeded )
{
	return ( shrink_block_cache( nBytesNeeded ) );
}
