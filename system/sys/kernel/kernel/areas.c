
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

/** \file areas.c
 * This file contains functions for manipulating regions of virtual memory
 * known as areas.  An area can exist in one of several states depending on its
 * access privileges and whether or not it is currently located in physical
 * memory.
 *
 * \par Read only:
 *     Both the area and the PTE are marked read only.
 * \par Cloned as copy-on-write:
 *     The PTE is marked as read-only and the area is marked as read-write.
 * \par Memory mapped/not present:
 *     The area has a non-<code>NULL</code> <code>a_psFile</code> entry and
 *     the PTE has the value 0.
 * \par Memory mapped/present:
 *     The area has a non-<code>NULL</code> <code>a_psFile</code> entry and
 *     the PTE is marked present.
 * \par Shared/not-present:
 *     The area has the <code>AREA_SHARED</code> flag set and the PTE is
 *     marked not present.
 * \par Swapped out:
 *     The PTE is marked not-present but the address is non-null.
 */

#include <posix/errno.h>
#include <posix/unistd.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/areas.h"
#include "inc/bcache.h"
#include "inc/swap.h"

/** A private shared MultiArray for managing areas. */
MultiArray_s g_sAreas;
/** A semaphore to control access to g_sAreas. */
sem_id g_hAreaTableSema = -1;

/** The table of memory area operations. */
static MemAreaOps_s sMemMapOps = {
	NULL,			/* open */
	NULL,			/* close */
	NULL,			/* unmap        */
	NULL,			/* protect */
	NULL,			/* sync */
	NULL,			/* advise */
	memmap_no_page,
	NULL,			/* wppage       */
	NULL,			/* swapout      */
	NULL,			/* swapin       */
};

/**
 * Prints all areas in the specified memory context to the debug console.
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> structure to print.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void list_areas( MemContext_s *psCtx )
{
	MemArea_s *psArea;

//  LOCK( g_hAreaTableSema );
	for ( count_t i = 0; i < psCtx->mc_nAreaCount; ++i )
	{
		psArea = psCtx->mc_apsAreas[i];

		if ( NULL == psArea )
		{
			printk( "Error: list_areas() index %d of %d has a NULL pointer!\n", i, psCtx->mc_nAreaCount );
			break;
		}
		if ( i != 0 && psArea->a_psPrev != psCtx->mc_apsAreas[i - 1] )
		{
			printk( "Error: list_areas() area at index %d (%p) has invalid prev pointer %p\n", i, psArea, psArea->a_psPrev );
		}
		if ( i < psCtx->mc_nAreaCount - 1 && psArea->a_psNext != psCtx->mc_apsAreas[i + 1] )
		{
			printk( "Error: list_areas() area at index %d (%p) has invalid next pointer %p\n", i, psArea, psArea->a_psNext );
		}
		printk( "Area %04d (%d) -> 0x%08x-0x%08x %p %02d %s\n", i, psArea->a_nAreaID, psArea->a_nStart, psArea->a_nEnd, psArea->a_psFile, atomic_read( &psArea->a_nRefCount ), psArea->a_zName );
	}
//  UNLOCK( g_hAreaTableSema );
}

void validate_kernel_pages(void)
{
	bool bError = false;
	printk( "Validating kernel pages...\n");
	for ( count_t i = 0; i < g_sSysBase.ex_nTotalPageCount; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, i * PAGE_SIZE );
		pte_t *pPte = pte_offset( pPgd, i * PAGE_SIZE );
		if( PTE_PAGE( *pPte ) != i * PAGE_SIZE )
		{
			printk( "Kernel pages at %08x corrupted: %08x\n", (uint)( i * PAGE_SIZE ), PTE_VALUE( *pPte ) );
			bError = true;
		}
	}
	if( !bError )
		printk( "Kernel pages validated\n" );
}

/**
 * Clears all page-table entries for the virtual memory region from nAddress
 *  to (nAddress + nSize - 1) and frees the associated physical memory pages.
 *  Only the entries in the page table containing the entry for the start
 *  address will be updated.
 * \internal
 * \ingroup Areas
 * \param pPgd a pointer to the page directory containing the address of the
 *     page table for the region to free.
 * \param nAddress the first virtual memory address of the region to free.
 * \param nSize the size in bytes of the memory region to free.
 * \return <code>-EINVAL</code> if there is no entry in the page directory for
 *     <i>nAddress</i>; <code>0</code> otherwise.
 * \sa free_pages()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static int clear_pagedir( MemArea_s *psArea, pgd_t * pPgd, uintptr_t nAddress, size_t nSize )
{
	uintptr_t nEnd;
	uintptr_t nStartAddress = nAddress;
	pte_t *pPte = pte_offset( pPgd, nAddress );

	nAddress &= ~PGDIR_MASK;
	nEnd = nAddress + nSize - 1;

	if ( nEnd >= PGDIR_SIZE )
	{
		nEnd = PGDIR_SIZE - 1;
	}
	
	if( !( PGD_VALUE( *pPgd ) & PTE_PRESENT ) )
	{
		printk( "Error: clear_pagedir() Page table at %08x not present\n", nStartAddress );
		printk( "Area %s ( %08x -> %08x ) PGD: %x\n", psArea->a_zName, psArea->a_nStart, psArea->a_nEnd, PGD_VALUE( *pPgd ) );
	}

	if ( 0 == PGD_PAGE( *pPgd ) )
	{
		printk( "Error: clear_pagedir() No page directory for address %08x\n", nStartAddress );
		printk( "Area %s ( %08x -> %08x ) PGD: %x\n", psArea->a_zName, psArea->a_nStart, psArea->a_nEnd, PGD_VALUE( *pPgd ) );
		printk( "Area list:\n" );
		list_areas( psArea->a_psContext );
		validate_kernel_pages();
		return ( -EINVAL );
	}

	do
	{
		if ( PTE_ISPRESENT( *pPte ) )
		{
			uintptr_t nPage = PTE_PAGE( *pPte );
			count_t nPageNr = PAGE_NR( nPage );

			if ( 0 == nPage )
			{
				printk( "ERROR : Page table referenced physical address 0\n" );
				PTE_VALUE( *pPte++ ) = ( uintptr_t )( g_sSysBase.ex_pNullPage ) | PTE_PRESENT | PTE_USER;
				nAddress += PAGE_SIZE;
				continue;
			}
			if ( nPage == (uintptr_t)( g_sSysBase.ex_pNullPage ) )
			{
				PTE_VALUE( *pPte++ ) = ( uintptr_t )( g_sSysBase.ex_pNullPage ) | PTE_PRESENT | PTE_USER;
				nAddress += PAGE_SIZE;
				continue;
			}
			if ( nPageNr >= g_sSysBase.ex_nTotalPageCount )
			{
				PTE_VALUE( *pPte++ ) = ( uintptr_t )( g_sSysBase.ex_pNullPage ) | PTE_PRESENT | PTE_USER;
				nAddress += PAGE_SIZE;
				continue;
			}
			if ( atomic_read( &g_psFirstPage[nPageNr].p_nCount ) == 1 )
			{
				unregister_swap_page( nPage );
			}
			if ( atomic_read( &g_psFirstPage[nPageNr].p_nCount ) < 1 )
			{
				printk( "panic: clear_pagedir() page %08x got ref count of %d\n", nPage, atomic_read( &g_psFirstPage[nPageNr].p_nCount ) - 1 );
				atomic_set( &g_psFirstPage[nPageNr].p_nCount, 1000 );
			}
			do_free_pages( nPage, 1 );
		}
		else
		{
			if ( PTE_PAGE( *pPte ) != 0 )
			{
				free_swap_page( PTE_PAGE( *pPte ) );
			}
		}
		PTE_VALUE( *pPte++ ) = ( uintptr_t )( g_sSysBase.ex_pNullPage ) | PTE_PRESENT | PTE_USER;
		nAddress += PAGE_SIZE;
	}
	while ( nAddress < nEnd );
	flush_tlb_global();

	return ( 0 );
}

/**
 * Frees all page-table entries in <i>psArea</i> for the virtual memory
 *  region from nAddress to (nAddress + nSize - 1).
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> containing the
 *     page-table entries to free.
 * \param nStart the first virtual memory address of the region to free.
 * \param nEnd the last virtual memory address of the region to free.
 * \return Always returns <code>0</code>.
 * \sa clear_pagedir()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static int free_area_pages( MemArea_s *psArea, uint32_t nStart, uint32_t nEnd )
{
	pgd_t *pPgd;

	pPgd = pgd_offset( psArea->a_psContext, nStart );

	while ( nStart - 1 < nEnd )
	{
		clear_pagedir( psArea, pPgd++, nStart, nEnd - nStart + 1 );
		nStart = ( nStart + PGDIR_SIZE ) & PGDIR_MASK;
	}
	return ( 0 );
}

/*
 * Frees all page-directory entries in <i>psArea</i> for the virtual memory
 *  region from nAddress to (nAddress + nSize - 1).
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> containing the
 *     page-directory entries to free.
 * \param nStart the first virtual memory address of the region to free.
 * \param nEnd the last virtual memory address of the region to free.
 * \param bResized <code>true</code> if PDEs still being used by the resized
 *     area should be skipped; <code>false</code> otherwise.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static void free_area_page_tables( MemArea_s *psArea, uint32_t nStart, uint32_t nEnd, bool bResized )
{
	MemContext_s *psCtx = psArea->a_psContext;
	uint32_t nFirstTab = nStart >> PGDIR_SHIFT;
	uint32_t nLastTab = nEnd >> PGDIR_SHIFT;
	MemArea_s *psPrev = psArea->a_psPrev;
	MemArea_s *psNext = psArea->a_psNext;
	uint32_t i;

	for ( i = nFirstTab; i <= nLastTab; ++i )
	{
		uint32_t nCurAddr = i << PGDIR_SHIFT;
		uint32_t nCurEnd = nCurAddr + PGDIR_SIZE - 1;
		uint32_t nPage;
		count_t nPageNr;

		// Skip tables still used by the resized area
		if ( bResized && nCurAddr <= psArea->a_nEnd )
		{
			continue;
		}
		// Skip tables shared by the previous area
		if ( psPrev != NULL && nCurAddr <= psPrev->a_nEnd )
		{
			continue;
		}
		// Skip tables shared by the next area
		if ( psNext != NULL && nCurEnd >= psNext->a_nStart )
		{
			continue;
		}
		nPage = PGD_PAGE( psCtx->mc_pPageDir[i] );
		if ( nPage == 0 )
		{
			printk( "Error: free_area_page_tables() found NULL pointer in page table at address 0x%x8\n", nCurAddr );
			continue;
		}
		nPageNr = PAGE_NR( nPage );
		PGD_VALUE( psCtx->mc_pPageDir[i] ) = PTE_DELETED;
		if ( nCurEnd < AREA_FIRST_USER_ADDRESS )
		{
			MemContext_s *psTmp;

			for ( psTmp = psCtx->mc_psNext; psTmp != psCtx; psTmp = psTmp->mc_psNext )
			{
				psTmp->mc_pPageDir[i] = psCtx->mc_pPageDir[i];
			}
		}
		if ( atomic_read( &g_psFirstPage[nPageNr].p_nCount ) != 1 )
			printk( "Error: free_area_page_tables(%s) has reference counter %i at address 0x%x8\n\n", 
					psArea->a_zName, atomic_read( &g_psFirstPage[nPageNr].p_nCount ), nCurAddr ); 
		
		do_free_pages( nPage, 1 );
	}
	flush_tlb_global();
}

/**
 * Deletes the specified area from the memory context <i>psCtx</i>.
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> for the area to
 *     delete.
 * \param psArea a pointer to the <code>MemArea_s</code> to delete.
 * \return <code>-EINVAL</code> if the area's reference count isn't zero;
 *     <code>0</code> otherwise.
 * \sa free_area_pages(), free_area_page_tables(), remove_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static int do_delete_area( MemContext_s *psCtx, MemArea_s *psArea )
{
	if ( atomic_read( &psArea->a_nRefCount ) != 0 )
	{
		panic( "do_delete_area() area ref count = %d\n", atomic_read( &psArea->a_nRefCount ) );
		return ( -EINVAL );
	}
	if ( psArea->a_bDeleted == true )
	{
		panic( "do_delete_area() tried to delete already deleted area %s\n", psArea->a_zName );
		return ( -EINVAL );
	}
	if ( psArea->a_psFile != NULL )
	{
		if ( psArea->a_psNextShared == psArea )
		{
			psArea->a_psFile->f_psInode->i_psAreaList = NULL;
		}
		else if ( psArea->a_psFile->f_psInode->i_psAreaList == psArea )
		{
			psArea->a_psFile->f_psInode->i_psAreaList = psArea->a_psNextShared;
		}
	}

	// Unlink from shared list.
	psArea->a_psPrevShared->a_psNextShared = psArea->a_psNextShared;
	psArea->a_psNextShared->a_psPrevShared = psArea->a_psPrevShared;

	psArea->a_bDeleted = true;	// Make sure nobody maps in pages while we wait for IO

	while ( atomic_read( &psArea->a_nIOPages ) > 0 )
	{
		UNLOCK( g_hAreaTableSema );
		printk( "do_delete_area() wait for IO to finish\n" );
		snooze( 20000 );
		LOCK( g_hAreaTableSema );
	}

	if ( psArea->a_psFile != NULL )
	{
		UNLOCK( g_hAreaTableSema );
		put_fd( psArea->a_psFile );
		LOCK( g_hAreaTableSema );
	}
	
	/* Free mttr descriptor */
	if( psArea->a_nProtection & AREA_WRCOMB )
	{
		uintptr_t nPhysAddress;
		pgd_t *pPgd;
		pte_t *pPte;
		pPgd = pgd_offset( psArea->a_psContext, psArea->a_nStart );
		if( PGD_PAGE( *pPgd ) > 0 ) 
		{
			pPte = pte_offset( pPgd, psArea->a_nStart );
			if ( PTE_ISPRESENT( *pPte ) )
			{
				nPhysAddress = PTE_PAGE( *pPte );
				if( free_mtrr_desc( nPhysAddress ) != 0 )
					printk( "Could not free mtrr descriptor for base address 0x%u\n", (uint)nPhysAddress );
			}
		}
	}

	if( ( psArea->a_nProtection & AREA_REMAPPED ) == 0 )
		free_area_pages( psArea, psArea->a_nStart, psArea->a_nEnd );
	free_area_page_tables( psArea, psArea->a_nStart, psArea->a_nEnd, false );
	flush_tlb_global();

//  delete_semaphore( psArea->a_hMutex1 );

	remove_area( psCtx, psArea );
	kfree( psArea );
	return ( 0 );
}

/*
 * Decrements the reference count of <i>psArea</i> and deletes it when its
 *  reference count reaches <code>0</code>.
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> to free.
 * \return The error code from do_delete_area() on failure; <code>0</code>
 *     otherwise.
 * \sa lock_area(), do_delete_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t put_area( MemArea_s *psArea )
{
	status_t nError = 0;

//  kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	if ( psArea != NULL )
	{
		if ( atomic_dec_and_test( &psArea->a_nRefCount ) )
		{
			lock_area( psArea, LOCK_AREA_WRITE );
			nError = do_delete_area( psArea->a_psContext, psArea );
		}
	}
	UNLOCK( g_hAreaTableSema );
	return ( nError );
//    return( delete_area( psArea ) );
}

/**
 * Allocates page-table entries in <i>psArea</i> for the virtual memory
 *  region from <code>nStart</code> to <code>nEnd</code>.
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> containing the
 *     page-table entries to allocate.
 * \param nStart the first virtual memory address of the region to allocate.
 * \param nEnd the last virtual memory address of the region to allocate.
 * \return <code>-ENOMEM</code> if there are no free pages; <code>0</code>
 *     otherwise.
 * \sa get_free_page()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static int alloc_area_page_tables( MemArea_s *psArea, uintptr_t nStart, uintptr_t nEnd )
{
	MemContext_s *psCtx = psArea->a_psContext;
	uintptr_t nFirstPTE = nStart >> PAGE_SHIFT;
	uintptr_t nLastPTE = nEnd >> PAGE_SHIFT;
	
	/* Allocate page tables */
	for ( uintptr_t i = nFirstPTE; i <= nLastPTE; ++i )
	{
		uintptr_t nDir = i >> ( PGDIR_SHIFT - PAGE_SHIFT );
		uint32_t *pDir = ( uint32_t * )PGD_PAGE( psCtx->mc_pPageDir[nDir] );
		if ( NULL == pDir )
		{
			/* Allocate new page table */
			pDir = ( uint32_t * )get_free_page( MEMF_CLEAR );
			if ( pDir == NULL )
			{
				printk( "Error: alloc_area_page_tables() out of memory\n" );
				return ( -ENOMEM );
			}
			PGD_VALUE( psCtx->mc_pPageDir[nDir] ) = MK_PGDIR( ( uint32_t )pDir );
			/* Copy page tables to other contexts if we have created a kernel area */
			if ( nStart < AREA_FIRST_USER_ADDRESS )
			{
				MemContext_s *psTmp;

				for ( psTmp = psCtx->mc_psNext; psTmp != psCtx; psTmp = psTmp->mc_psNext )
				{
					psTmp->mc_pPageDir[nDir] = psCtx->mc_pPageDir[nDir];
				}
			}
		}
		kassertw( pDir != NULL );
		pDir[i % 1024] = ( uintptr_t )( g_sSysBase.ex_pNullPage ) | PTE_PRESENT | PTE_USER;
	}

	flush_tlb_global();
	return ( 0 );
}


/**
 * Allocates pages in <i>psArea</i> for the virtual memory region from
 *  <code>nStart</code> to <code>nEnd</code>.
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> to allocate pages for.
 * \param nStart the first virtual memory address of the region to allocate.
 * \param nEnd the last virtual memory address of the region to allocate.
 * \param bWriteAccess <code>true</code> if writable pages are required;
 *     <code>false</code> otherwise.
 * \return The error code from load_area_page() on failure; <code>0</code>
 *     otherwise.
 * \sa load_area_page()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t alloc_area_pages( MemArea_s *psArea, uintptr_t nStart,
				  uintptr_t nEnd, bool bWriteAccess )
{
	MemContext_s *psSeg = psArea->a_psContext;
	flags_t nFlags = PTE_PRESENT;
	status_t nError = 0;
	uintptr_t nAddr;

	if ( psArea->a_nProtection & AREA_WRITE )
	{
		nFlags |= PTE_WRITE;
	}
	if ( ( psArea->a_nProtection & AREA_KERNEL ) == 0 )
	{
		nFlags |= PTE_USER;
	}
	for ( nAddr = nStart; nAddr < nEnd; nAddr += PAGE_SIZE )
	{
		pgd_t *pPgd = pgd_offset( psSeg, nAddr );
		pte_t *pPte = pte_offset( pPgd, nAddr );

		if ( PTE_ISPRESENT( *pPte ) )
		{
			if ( bWriteAccess == false || PTE_ISWRITE( *pPte ) )
			{
				continue;	// It is present and writable so nothing to do here.
			}
		}
		nError = load_area_page( psArea, nAddr, bWriteAccess );
//          nError = handle_copy_on_write( psArea, pPte, nAddr );
		if ( nError < 0 )
		{
			break;
		}
	}
	return ( nError );
}


/**
 * Allocates contiguous pages in <i>psArea</i> for the virtual memory region from
 *  <code>nStart</code> to <code>nEnd</code>.
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> to allocate pages for.
 * \param nStart the first virtual memory address of the region to allocate.
 * \param nEnd the last virtual memory address of the region to allocate.
 * \param bWriteAccess <code>true</code> if writable pages are required;
 *     <code>false</code> otherwise.
 * \return The error code from load_area_page() on failure; <code>0</code>
 *     otherwise.
 * \sa load_area_page()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t alloc_area_pages_contiguous( MemArea_s *psArea, uintptr_t nStart,
				  uintptr_t nEnd, bool bWriteAccess )
{
	MemContext_s *psSeg = psArea->a_psContext;
	flags_t nFlags = PTE_PRESENT;
	status_t nError = 0;
	uintptr_t nAddr;
	size_t nPages = ( nEnd - nStart + 1 ) >> PAGE_SHIFT;
	uintptr_t nPhysAddr = get_free_pages( nPages, MEMF_CLEAR );
	
	if( nPhysAddr == 0 )
	{
		return ( -ENOMEM );
	}
	

	if ( psArea->a_nProtection & AREA_WRITE )
	{
		nFlags |= PTE_WRITE;
	}
	if ( ( psArea->a_nProtection & AREA_KERNEL ) == 0 )
	{
		nFlags |= PTE_USER;
	}
	for ( nAddr = nStart; nAddr < nEnd; nAddr += PAGE_SIZE, nPhysAddr += PAGE_SIZE )
	{
		pgd_t *pPgd = pgd_offset( psSeg, nAddr );
		pte_t *pPte = pte_offset( pPgd, nAddr );
		
		PTE_VALUE( *pPte ) = nPhysAddr | nFlags;
	}
	flush_tlb_global();
	return ( nError );
}

/**
 * Creates a new memory area in <i>psCtx</i> from nAddress to
 *  (nAddress + nSize - 1).
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> structure to use.
 * \param nAddress the first virtual memory address for the requested area.
 * \param nSize the size in bytes of the requested area.
 * \param nMaxSize the maximum size in bytes of the requested area.
 * \param bAllocTables <code>true</code> if page tables and page-table entries
 *     should be allocated.
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \return <code>NULL</code> if the request failed; a pointer to the new
 *     <code>MemArea_s</code> otherwise.
 * \sa insert_area(), alloc_area_page_tables(), alloc_area_pages()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static MemArea_s *do_create_area( MemContext_s *psCtx, uintptr_t nAddress,
				  size_t nSize, size_t nMaxSize,
				  bool bAllocTables, flags_t nProtection,
				  flags_t nLockMode )
{
	MemArea_s *psArea;
	status_t nError;

	psArea = kmalloc( sizeof( MemArea_s ), MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );

	if ( NULL == psArea )
	{
		printk( "Error: do_create_area() out of memory\n" );
		goto error1;
	}
	// FIXME: create_semaphore() allocates memory, and can deadlock the system.

	psArea->a_nStart = nAddress;
	psArea->a_nEnd = nAddress + nSize - 1;
	psArea->a_nMaxEnd = nAddress + nMaxSize - 1;

	psArea->a_psNext = NULL;
	psArea->a_psPrev = NULL;
	psArea->a_psNextShared = psArea;
	psArea->a_psPrevShared = psArea;
	psArea->a_psContext = psCtx;
	psArea->a_psOps = NULL;
	psArea->a_nProtection = nProtection;
	psArea->a_nLockMode = nLockMode;
	atomic_set( &psArea->a_nRefCount, 1 );

	nError = insert_area( psCtx, psArea );

	if ( nError < 0 )
	{
		goto error2;
	}
	if ( bAllocTables )
	{
		nError = alloc_area_page_tables( psArea, psArea->a_nStart, psArea->a_nEnd );
		if ( nError < 0 )
		{
			goto error3;
		}
		if( nLockMode == AREA_CONTIGUOUS )
		{
			nError = alloc_area_pages_contiguous( psArea, psArea->a_nStart, psArea->a_nEnd, true );
			if ( nError < 0 )
				goto error4;
		}
		else if( nLockMode == AREA_FULL_LOCK )
		{		
			nError = alloc_area_pages( psArea, psArea->a_nStart, psArea->a_nEnd, true );
			if ( nError < 0 )
				goto error4;
		}
	}
	return ( psArea );
      error4:
	free_area_pages( psArea, psArea->a_nStart, psArea->a_nEnd );
	free_area_page_tables( psArea, psArea->a_nStart, psArea->a_nEnd, false );
      error3:
	remove_area( psCtx, psArea );
      error2:
	kfree( psArea );
      error1:
	return ( NULL );
}

/**
 * Creates a new memory area in <i>psCtx</i> from nAddress to
 *  (nAddress + nSize - 1).  The area is given the specified name, protection
 *  mask, locking mode and maximum size.  The <code>area_id</code> of the new
 *  area is stored in <i>pnArea</i>.
 * \internal
 * \ingroup Areas
 * \param pzName a pointer to the string containing the name for the new area.
 * \param psCtx a pointer to the <code>MemContext_s</code> structure to use.
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode   the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \param nAddress the starting virtual memory address for the new area.
 * \param nSize the size in bytes of the requested area.
 * \param nMaxSize the maximum size in bytes of the requested area.
 * \param pnArea a pointer to the variable in which to store the new
 *     <code>area_id</code>.
 * \return <code>-ENOADDRSPC</code> if there isn't enough virtual address
 *     space, <code>-ENOMEM</code> if do_create_area() failed;
 *     <code>0</code> otherwise.
 * \sa do_create_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t alloc_area( const char *pzName, MemContext_s *psCtx,
			    flags_t nProtection, flags_t nLockMode,
			    uintptr_t nAddress, size_t nSize,
			    size_t nMaxSize, area_id *pnArea )
{
	MemArea_s *psArea;
	uintptr_t nNewAddr;
	status_t nError;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	nNewAddr = find_unmapped_area( psCtx, nProtection, nMaxSize, nAddress );

	if ( nNewAddr == (uintptr_t)(-1) && ( nProtection & AREA_ADDR_SPEC_MASK ) == AREA_BASE_ADDRESS )
	{
		nNewAddr = find_unmapped_area( psCtx, ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_ANY_ADDRESS, nMaxSize, nAddress );
	}

	if ( nNewAddr == (uintptr_t)(-1) )
	{
		printk( "Error: alloc_area() out of virtual space\n" );
		nError = -ENOADDRSPC;
		goto error;
	}

	if ( ( nProtection & AREA_ADDR_SPEC_MASK ) == AREA_EXACT_ADDRESS )
	{
		if ( nNewAddr != nAddress )
		{
			printk( "PANIC: find_unmapped_area() returned %p during attempt to alloc at exact %p\n", (void*)( nNewAddr ), (void*)( nAddress ) );
			nNewAddr = nAddress;
		}
	}
	if ( nNewAddr < AREA_FIRST_USER_ADDRESS && ( nProtection & AREA_KERNEL ) == 0 )
	{
		printk( "alloc_area() set AREA_KERNEL flag\n" );
		nProtection |= AREA_KERNEL;
	}

	psArea = do_create_area( psCtx, nNewAddr, nSize, nMaxSize, true, nProtection, nLockMode );

	if ( NULL == psArea )
	{
		nError = -ENOMEM;
		goto error;
	}
	strcpy( psArea->a_zName, pzName );
	*pnArea = psArea->a_nAreaID;

	return ( 0 );
      error:
	return ( nError );
}

/**
 * Allocates an array of areas with the specified names, starting offsets
 *  and sizes.  All areas will have the same protection mask and locking mode.
 * \ingroup DriverAPI
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \param nAddress the base virtual memory address for the new areas.
 *     Each area will start at <code>(nAddress + panOffsets[i])</code>.
 * \param nCount the number of areas to create.  <code>apzNames</code>,
 *     <code>panOffsets</code>, <code>panSizes</code>, and
 *     <code>panAreas</code> are arrays of this size.
 * \param apzNames an array of pointers to strings containing the names to
 *     use for each area.  Any entry can be <code>NULL</code> to create an
 *     unnamed area, or a <code>NULL</code> pointer can be passed here to
 *     create all areas without names.
 * \param panOffsets an array of byte offsets from <code>nAddress</code>
 *     to use as the start address for each area.
 * \param panSizes an array of sizes in bytes for each area.
 * \param panAreas the <code>area_id</code>s of the new areas will be copied
 *     to this array.
 * \return <code>-EINVAL</code> if any of the areas to create would overlap
 *     or if the starting offsets are not in sequential order,
 *     <code>-ENOADDRSPC</code> if there isn't enough free virtual address
 *     space for the new areas, <code>-ENOMEM</code> if there isn't enough
 *     physical memory available; <code>0</code> otherwise.
 * \sa find_unmapped_area(), do_create_area(), do_delete_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t alloc_area_list( uint32_t nProtection, uint32_t nLockMode,
			  uintptr_t nAddress, uint_fast32_t nCount,
			  const char *const *apzNames, size_t *panOffsets,
			  size_t *panSizes, area_id *panAreas )
{
	MemContext_s *psCtx;
	size_t nTotSize = panOffsets[nCount - 1] + panSizes[nCount - 1];
	uintptr_t nNewAddr;
	status_t nError;

	if ( nProtection & AREA_KERNEL )
	{
		psCtx = g_psKernelSeg;
	}
	else
	{
		psCtx = CURRENT_PROC->tc_psMemSeg;
	}

	// Verify parameters.
	for ( uint_fast32_t i = 0; i < nCount - 1; ++i )
	{
		if ( panOffsets[i] + panSizes[i] > panOffsets[i + 1] )
		{
			printk( "Error: alloc_area_list() area %d (%08x, %08x) overlaps area %d (%08x,%08x)\n", i, panOffsets[i], panSizes[i], i + 1, panOffsets[i + 1], panSizes[i + 1] );
			return ( -EINVAL );
		}
		if ( panOffsets[i] > panOffsets[i + 1] )
		{
			printk( "Error: alloc_area_list() area %d (%08x, %08x) addressed after area %d (%08x,%08x)\n", i, panOffsets[i], panSizes[i], i + 1, panOffsets[i + 1], panSizes[i + 1] );
			return ( -EINVAL );
		}
	}
      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	if ( psCtx->mc_bBusy )
	{
		UNLOCK( g_hAreaTableSema );
		printk( "alloc_area_list() wait for segment to become ready\n" );
		snooze( 10000 );
		goto again;
	}
	nNewAddr = find_unmapped_area( psCtx, nProtection, nTotSize, nAddress );

	if ( nNewAddr == (uintptr_t)(-1) && ( nProtection & AREA_ADDR_SPEC_MASK ) == AREA_BASE_ADDRESS )
	{
		nNewAddr = find_unmapped_area( psCtx, ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_ANY_ADDRESS, nTotSize, nAddress );
		printk( "alloc_area_list() failed to alloc %d areas at %p, got %p\n", nCount, (void*)( nAddress ), (void*)( nNewAddr ) );
	}
	if ( nNewAddr == (uintptr_t)(-1) )
	{
		printk( "Error: alloc_area_list() out of address space\n" );
		nError = -ENOADDRSPC;
		goto error;
	}

	for ( uint_fast32_t i = 0; i < nCount; ++i )
	{
		MemArea_s *psArea = do_create_area( psCtx, nNewAddr + panOffsets[i], panSizes[i], panSizes[i], true, nProtection, nLockMode );

		if ( psArea == NULL )
		{
			for ( uint_fast32_t j = 0; j < i; ++j )
			{
				psArea = get_area_from_handle( panAreas[j] );
				if ( psArea == NULL )
				{
					panic( "alloc_area_list() could not find newly created are!\n" );
				}
				kassertw( psArea->a_psFile == NULL );
				do_delete_area( psCtx, psArea );
			}
			nError = -ENOMEM;
			goto error;
		}
		if ( apzNames != NULL && apzNames[i] != NULL )
		{
			strcpy( psArea->a_zName, apzNames[i] );
		}
		panAreas[i] = psArea->a_nAreaID;
	}
	UNLOCK( g_hAreaTableSema );

	return ( 0 );
      error:
	UNLOCK( g_hAreaTableSema );
	if ( nError == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}
	return ( nError );
}

/**
 * Resizes the specified area.
 * \ingroup DriverAPI
 * \param hArea a handle to the area to resize.
 * \param nNewSize the new area size in bytes.
 * \param bAtomic if <code>true</code>, resize_area() will repeatedly shrink
 *     the block cache by 64K and try again if an <code>-ENOMEM</code> error
 *     is returned by alloc_area_pages() or alloc_area_page_tables().
 * \return <code>-EINVAL</code> if the area handle is invalid,
 *     <code>-ENOADDRSPC</code> if there isn't enough virtual address space
 *     available, <code>-ENOMEM</code> if there isn't enough physical memory
 *     available; <code>0</code> otherwise.
 * \sa alloc_area_page_tables(), alloc_area_pages(), free_area_page_tables(),
 *     free_area_pages(), shrink_caches()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t resize_area( area_id hArea, size_t nNewSize, bool bAtomic )
{
	MemArea_s *psArea;
	status_t nError = 0;

      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );

	if ( psArea == NULL )
	{
		printk( "Error: resize_area() invalid handle %d\n", hArea );
		nError = -EINVAL;
		goto error1;
	}

	// Check that the area won't wrap around the 4G limit
	if ( psArea->a_nStart + nNewSize < psArea->a_nStart )
	{
		nError = -ENOADDRSPC;
		printk( "resize_area() area too big %p -> 0x%08x\n", (void*)( psArea->a_nStart ), nNewSize );
		goto error2;
	}
	// Check that a kernel area won't extend into user-space.
	if ( psArea->a_nStart < AREA_FIRST_USER_ADDRESS )
	{
		if ( psArea->a_nStart + nNewSize > AREA_FIRST_USER_ADDRESS )
		{
			nError = -ENOADDRSPC;
			printk( "resize_area() area extends out of the kernel: 0x%08x\n", nNewSize );
			goto error2;
		}
	}

	if ( psArea->a_psNext != NULL )
	{
		// If it's not the last area, we must verify that it won't collide with the next.
		if ( psArea->a_nStart + nNewSize > psArea->a_psNext->a_nStart )
		{
			nError = -ENOADDRSPC;
//          printk( "resize_area() no space after area %p -> %08lx (%p)\n", (void*)psArea->a_nStart, nNewSize, (void*)psArea->a_psNext->a_nStart );
			goto error2;
		}
	}
	if ( nNewSize == psArea->a_nEnd - psArea->a_nStart + 1 )
	{
		goto error2;
	}
	if ( psArea->a_nEnd - psArea->a_nStart + 1 < nNewSize )
	{
		nError = alloc_area_page_tables( psArea, psArea->a_nEnd + 1, psArea->a_nStart + nNewSize - 1 );
		if ( nError < 0 )
		{
			free_area_page_tables( psArea, psArea->a_nEnd + 1, psArea->a_nStart + nNewSize - 1, true );
			goto error2;
		}
		if ( psArea->a_nLockMode == AREA_FULL_LOCK )
		{
			nError = alloc_area_pages( psArea, psArea->a_nEnd + 1, psArea->a_nStart + nNewSize - 1, true );
			if ( nError < 0 )
			{
				free_area_pages( psArea, psArea->a_nEnd + 1, psArea->a_nStart + nNewSize - 1 );
				free_area_page_tables( psArea, psArea->a_nEnd + 1, psArea->a_nStart + nNewSize - 1, true );
				goto error2;
			}
		}
		psArea->a_nEnd = psArea->a_nStart + nNewSize - 1;
		if ( psArea->a_nEnd > psArea->a_nMaxEnd )
		{
			psArea->a_nMaxEnd = psArea->a_nEnd;
		}
	}
	else
	{
		uint32_t nOldEnd = psArea->a_nEnd;

		if ( atomic_read( &psArea->a_nIOPages ) > 0 )
		{
			UNLOCK( g_hAreaTableSema );
			printk( "resize_area() wait for IO to finish\n" );
			snooze( 20000 );
			goto again;;
		}
		psArea->a_nEnd = psArea->a_nStart + nNewSize - 1;
		free_area_pages( psArea, psArea->a_nStart + nNewSize, nOldEnd );
		free_area_page_tables( psArea, psArea->a_nStart + nNewSize, nOldEnd, true );
	}
	flush_tlb_global();

      error2:
	put_area( psArea );
      error1:
	UNLOCK( g_hAreaTableSema );
	if ( nError == -ENOMEM && bAtomic )
	{
		shrink_caches( 65536 );
		goto again;
	}
	return ( nError );
}

/**
 * Deletes the specified area.
 * \ingroup DriverAPI
 * \param hArea a handle to the area to resize.
 * \return <code>-EINVAL</code> if the area handle is invalid,
 *     <code>-ENOADDRSPC</code> if there isn't enough virtual address space
 *     available, <code>-ENOMEM</code> if there isn't enough physical memory
 *     available; <code>0</code> otherwise.
 * \sa do_delete_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t delete_area( area_id hArea )
{
	MemArea_s *psArea;
	status_t nError = 0;

//  kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
      again:
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );

	if ( psArea != NULL )
	{
		if ( psArea->a_psContext->mc_bBusy )
		{
			atomic_dec( &psArea->a_nRefCount );
			UNLOCK( g_hAreaTableSema );
			printk( "delete_area(): wait for segment to become ready\n" );
			snooze( 10000 );
			goto again;
		}

		if ( atomic_sub_and_test( &psArea->a_nRefCount, 2 ) )
		{
//          lock_semaphore_ex( psArea->a_hMutex, AREA_MUTEX_COUNT, SEM_NOSIG, INFINITE_TIMEOUT );
			lock_area( psArea, LOCK_AREA_WRITE );
			nError = do_delete_area( psArea->a_psContext, psArea );
		}
	}
	UNLOCK( g_hAreaTableSema );
	return ( nError );
}

/**
 * Deletes all areas in the specified memory context.
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> containing areas
 *     to delete.
 * \warning The area table semaphore <code>g_hAreaTableSema</code> must already
 *     be locked.
 * \sa do_delete_area(), free_pages(), empty_mem_context()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static void delete_all_areas( MemContext_s *psCtx )
{
	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	while ( psCtx->mc_nAreaCount > 0 )
	{
		lock_area( psCtx->mc_apsAreas[0], LOCK_AREA_WRITE );

		if ( !atomic_dec_and_test( &psCtx->mc_apsAreas[0]->a_nRefCount ) )
		{
			printk( "Error : delete_all_areas() area '%s' (%d) for addr %p has ref count of %d\n", psCtx->mc_apsAreas[0]->a_zName, psCtx->mc_apsAreas[0]->a_nAreaID, (void*)( psCtx->mc_apsAreas[0]->a_nStart ), atomic_read( &psCtx->mc_apsAreas[0]->a_nRefCount ) + 1 );

			atomic_set( &psCtx->mc_apsAreas[0]->a_nRefCount, 0 );
		}
		do_delete_area( psCtx, psCtx->mc_apsAreas[0] );
	}

	for ( uint_fast32_t i = 512; i < 1024; ++i )
	{
		uint32_t nPage = PGD_PAGE( psCtx->mc_pPageDir[i] );

		if ( 0 != nPage )
		{
			printk( "delete_all_areas() page-table at %d forgotten\n", i );
			do_free_pages( nPage, 1 );
		}
		PGD_VALUE( psCtx->mc_pPageDir[i] ) = 0;
	}
	flush_tlb_global();
}

/**
 * Deletes all areas in the specified memory context.
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> containing areas to
 *     delete.
 * \sa delete_all_areas()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void empty_mem_context( MemContext_s *psCtx )
{
	LOCK( g_hAreaTableSema );
	delete_all_areas( psCtx );
	UNLOCK( g_hAreaTableSema );
}

/**
 * Deletes the specified memory context and all of its areas.
 * \internal
 * \ingroup Areas
 * \param psCtx a pointer to the <code>MemContext_s</code> to delete.
 * \sa delete_all_areas(), free_pages()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void delete_mem_context( MemContext_s *psCtx )
{
	MemContext_s *psTmp;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	for ( psTmp = psCtx->mc_psNext; NULL != psTmp; psTmp = psTmp->mc_psNext )
	{
		if ( psTmp->mc_psNext == psCtx )
		{
			psTmp->mc_psNext = psCtx->mc_psNext;
			break;
		}
	}

	delete_all_areas( psCtx );
	UNLOCK( g_hAreaTableSema );

	free_pages( ( uint32_t )psCtx->mc_pPageDir, 1 );
	if ( psCtx->mc_apsAreas != NULL )
	{
		kfree( psCtx->mc_apsAreas );
	}
	kfree( psCtx );
}

/**
 * Copies a page-table entry, optionally setting up copy-on-write semantics.
 * \internal
 * \ingroup Areas
 * \param pDst a pointer to the destination page-table entry.
 * \param pSrc a pointer to the source page-table entry.
 * \param bCow <code>true</code> if the new virtual page should be set up for
 *     copy-on-write semantics; <code>false</code> to share the physical page.
 * \return Always returns <code>0</code>.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
inline status_t clone_page_pte( pte_t * pDst, pte_t * pSrc, bool bCow )
{
	pte_t nPte;
	int nPageNr;
	int nPageAddress = PTE_PAGE( *pSrc );

	if ( bCow )
	{
		PTE_VALUE( *pSrc ) &= ~PTE_WRITE;
	}

	nPageNr = PAGE_NR( nPageAddress );

	nPte = *pSrc;

	if ( PTE_ISPRESENT( nPte ) )
	{
		if( nPageNr < g_sSysBase.ex_nTotalPageCount )
		{
			atomic_inc( &g_psFirstPage[nPageNr].p_nCount );
		}
	}
	else if ( PTE_PAGE( nPte ) != 0 )
	{
		dup_swap_page( PTE_PAGE( nPte ) );
	}
	*pDst = nPte;
	return ( 0 );
}

/**
 * Copies an area from one memory context into another.  The area will have
 *  the same virtual memory address range in both contexts.
 * \internal
 * \ingroup Areas
 * \param psDstSeg a pointer to the destination <code>MemContext_s</code>.
 * \param psSrcSeg a pointer to the source <code>MemContext_s</code>.
 * \param psArea a pointer to the <code>MemArea_s</code> to copy.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static void do_clone_area( MemContext_s *psDstSeg, MemContext_s *psSrcSeg, MemArea_s *psArea )
{
	uint32_t nSrcAddr;
	bool bCow, bCopy;

	bCow = ( psArea->a_nProtection & ( AREA_REMAPPED | AREA_SHARED | AREA_WRITE ) ) == AREA_WRITE;
	bCopy = ( psArea->a_nLockMode & ( AREA_FULL_LOCK | AREA_CONTIGUOUS ) );

	if( bCopy )
	{
		/* Copy pages */
		for ( nSrcAddr = psArea->a_nStart; ( nSrcAddr - 1 ) != psArea->a_nEnd; nSrcAddr += PAGE_SIZE )
		{
			pgd_t *pSrcPgd = pgd_offset( psSrcSeg, nSrcAddr );
			pte_t *pSrcPte = pte_offset( pSrcPgd, nSrcAddr );
			pgd_t *pDstPgd = pgd_offset( psDstSeg, nSrcAddr );
			pte_t *pDstPte = pte_offset( pDstPgd, nSrcAddr );

			kassertw( PGD_PAGE( *pDstPgd ) != 0 );
			kassertw( PTE_ISPRESENT( *pDstPte ) );			
			kassertw( PTE_ISPRESENT( *pSrcPte ) );
						
			memcpy( (void*)PTE_PAGE( *pDstPte ), (void*)PTE_PAGE( *pSrcPte ), PAGE_SIZE );
		}
	}
	else
	{
		/* Clone pagetable entries */
		for ( nSrcAddr = psArea->a_nStart; ( nSrcAddr - 1 ) != psArea->a_nEnd; nSrcAddr += PAGE_SIZE )
		{
			pgd_t *pSrcPgd = pgd_offset( psSrcSeg, nSrcAddr );
			pte_t *pSrcPte = pte_offset( pSrcPgd, nSrcAddr );
			pgd_t *pDstPgd = pgd_offset( psDstSeg, nSrcAddr );
			pte_t *pDstPte = pte_offset( pDstPgd, nSrcAddr );

			kassertw( PGD_PAGE( *pDstPgd ) != 0 );
		
			clone_page_pte( pDstPte, pSrcPte, bCow );
		}
	}
}

/**
 * Clones the specified memory context.
 * \internal
 * \ingroup Areas
 * \param psOrig a pointer to the memory context to clone.
 * \return a pointer to the new memory context.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
MemContext_s *clone_mem_context( MemContext_s *psOrig )
{
	MemContext_s *psNewSeg;
	status_t nError;

      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	if ( psOrig->mc_bBusy )
	{
		UNLOCK( g_hAreaTableSema );
		printk( "clone_mem_context(): wait for segment to become ready\n" );
		snooze( 10000 );
		goto again;
	}
	psOrig->mc_bBusy = true;	// Make sure nobody creates or deletes any areas while we are cloning

	psNewSeg = kmalloc( sizeof( MemContext_s ), MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );

	if ( NULL == psNewSeg )
	{
		printk( "Error: clone_mem_context() out of memory\n" );
		nError = -ENOMEM;
		goto error1;
	}

	psNewSeg->mc_pPageDir = ( pgd_t * ) get_free_page( MEMF_CLEAR );

	if ( psNewSeg->mc_pPageDir == NULL )
	{
		printk( "Error: clone_mem_context() out of memory\n" );
		nError = -ENOMEM;
		goto error2;
	}

	memcpy( psNewSeg->mc_pPageDir, psOrig->mc_pPageDir, 2048 );	/* Copy shared page table pointers */

	for ( count_t i = 0; i < ( volatile count_t )psOrig->mc_nAreaCount; ++i )	// XXX is "volatile" qualifier necessary? should we use an atomic_t?
	{
		MemArea_s *psOldArea = psOrig->mc_apsAreas[i];
		MemArea_s *psNewArea;

		if ( psOldArea->a_nProtection & AREA_KERNEL )
		{
			continue;
		}

		// Wait til all IO performed on pages in the area is finished
		while ( atomic_read( &psOldArea->a_nIOPages ) > 0 )
		{
			Thread_s *psThread = CURRENT_THREAD;
			WaitQueue_s sWaitNode;

			printk( "clone_mem_context() Wait for area %s to finish IO on %d pages\n", psOldArea->a_zName, atomic_read( &psOldArea->a_nIOPages ) );
			sWaitNode.wq_hThread = psThread->tr_hThreadID;

			flags_t nFlg = cli();	// Make sure we are not pre-empted until we are added to the waitlist
			sched_lock();
			add_to_waitlist( false, &psOldArea->a_psIOThreads, &sWaitNode );
			psThread->tr_nState = TS_WAIT;
			sched_unlock();
			UNLOCK( g_hAreaTableSema );
			put_cpu_flags( nFlg );
			Schedule();
			LOCK( g_hAreaTableSema );
			remove_from_waitlist( true, &psOldArea->a_psIOThreads, &sWaitNode );
		}

		lock_area( psOldArea, LOCK_AREA_READ );

		psNewArea = do_create_area( psNewSeg, psOldArea->a_nStart, psOldArea->a_nEnd - psOldArea->a_nStart + 1, psOldArea->a_nMaxEnd - psOldArea->a_nStart + 1, true, psOldArea->a_nProtection, psOldArea->a_nLockMode );
		if ( psNewArea == NULL )
		{
			printk( "clone_mem_context() failed to alloc area\n" );
			unlock_area( psOldArea, LOCK_AREA_READ );
			nError = -ENOMEM;
			goto error3;
		}
		memcpy( psNewArea->a_zName, psOldArea->a_zName, sizeof( psNewArea->a_zName ) );
		psNewArea->a_psFile = psOldArea->a_psFile;
		psNewArea->a_nFileOffset = psOldArea->a_nFileOffset;
		psNewArea->a_nFileLength = psOldArea->a_nFileLength;
		psNewArea->a_psOps = psOldArea->a_psOps;

		do_clone_area( psNewSeg, psOrig, psNewArea );

		// If the source area is mapped to a file the new better be to.
		if ( psOldArea->a_psFile != NULL )
		{
			MemArea_s *psTmp;

			psNewArea->a_psNextShared = psOldArea->a_psNextShared;
			psNewArea->a_psNextShared->a_psPrevShared = psNewArea;
			psOldArea->a_psNextShared = psNewArea;
			psNewArea->a_psPrevShared = psOldArea;

			for ( psTmp = psNewArea->a_psNextShared; psTmp != psNewArea; psTmp = psTmp->a_psNextShared )
			{
				kassertw( psTmp->a_psFile != NULL );
				kassertw( psTmp->a_psFile->f_psInode->i_nInode == psNewArea->a_psFile->f_psInode->i_nInode );
			}
			atomic_inc( &psNewArea->a_psFile->f_nRefCount );
		}
//      unlock_semaphore_ex( psOldArea->a_hMutex, 1 );
		unlock_area( psOldArea, LOCK_AREA_READ );
	}
	psNewSeg->mc_psNext = psOrig->mc_psNext;
	psOrig->mc_psNext = psNewSeg;
	flush_tlb_global();
	psOrig->mc_bBusy = false;
	UNLOCK( g_hAreaTableSema );

	return ( psNewSeg );
      error3:
	delete_all_areas( psNewSeg );
	free_pages( ( uint32_t )psNewSeg->mc_pPageDir, 1 );
      error2:
	kfree( psNewSeg );
      error1:
	psOrig->mc_bBusy = false;
	UNLOCK( g_hAreaTableSema );
	if ( nError == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}
	return ( NULL );
}

/**
 * Unmaps the page-table entries from <code>nAddress</code> to
 *  <code>(nAddress + nSize - 1)</code>.
 * \internal
 * \ingroup Areas
 * \param pPgd a pointer to the page directory containing the address of
 *     the page table with entries to unmap.
 * \param nAddress the first memory address of the region to free.
 * \param nSize the size in bytes of the memory region to clear.
 * \param bFreeOldPages <code>true</code> to free the unmapped pages;
 *     <code>false</code> otherwise.
 * \return Always returns <code>0</code>.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t unmap_pagedir( pgd_t * pPgd, uintptr_t nAddress, count_t nSize,
			       bool bFreeOldPages )
{
	uintptr_t nEnd;

	pte_t *pPte = pte_offset( pPgd, nAddress );

	nAddress &= ~PGDIR_MASK;
	nEnd = nAddress + nSize - 1;

	if ( nEnd > PGDIR_SIZE )
	{
		nEnd = PGDIR_SIZE;
	}
	do
	{
		if ( bFreeOldPages )
		{
			if ( PTE_ISPRESENT( *pPte ) )
			{
				uintptr_t nOldPage = PTE_PAGE( *pPte );
				count_t nPageNr = PAGE_NR( nOldPage );

				if ( 0 != nOldPage && nOldPage != ( uintptr_t )( g_sSysBase.ex_pNullPage ) )
				{
					if ( atomic_read( &g_psFirstPage[nPageNr].p_nCount ) == 1 )
					{
						unregister_swap_page( nOldPage );
					}
					do_free_pages( nOldPage, 1 );
				}
			}
			else if ( PTE_PAGE( *pPte ) != 0 )
			{
				free_swap_page( PTE_PAGE( *pPte ) );
			}
		}
		PTE_VALUE( *pPte ) = 0;	// PTE_USER;
		pPte++;

		nAddress += PAGE_SIZE;
	}
	while ( nAddress < nEnd );
	flush_tlb_global();

	return ( 0 );
}

/**
 * Maps the specified area onto a file.
 * \internal
 * \ingroup Areas
 * \param hArea a handle to the area to map to the file.
 * \param psFile a pointer to the <code>File_s</code> to map the area onto.
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nOffset the offset from the start of <i>psFile</i> to use for the
 *     first byte of the mapped region of the file.
 * \param nSize the size of the new area in bytes.
 * \return <code>-EINVAL</code> if <i>hArea</i> is invalid or has no associated
 *     memory context; <code>0</code> otherwise.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t map_area_to_file( area_id hArea, File_s *psFile,
			   flags_t nProtection	__attribute__ ((unused)),
			   off_t nOffset, size_t nSize )
{
	MemContext_s *psCtx;
	MemArea_s *psArea;
	Inode_s *psInode = psFile->f_psInode;
	uintptr_t nAddress;
	uintptr_t nEnd;
	pgd_t *pPgd;
	bool bFreeOldPages;
	status_t nError = 0;
	MemArea_s *psTmp;

	kassertw( psFile != NULL );
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );
	if ( psArea == NULL )
	{
		printk( "Error: map_area_to_file() called with invalid area %d\n", hArea );
		UNLOCK( g_hAreaTableSema );
		return ( -EINVAL );
	}
	psCtx = psArea->a_psContext;

	nAddress = psArea->a_nStart;
	nEnd = psArea->a_nEnd;

	if ( NULL == psCtx )
	{
		printk( "PANIC: map_area_to_file() area has no memcontext!\n" );
		UNLOCK( g_hAreaTableSema );
		return ( -EINVAL );
	}

//    lock_semaphore_ex( psArea->a_hMutex, AREA_MUTEX_COUNT, SEM_NOSIG, INFINITE_TIMEOUT );
	lock_area( psArea, LOCK_AREA_WRITE );

	bFreeOldPages = ( psArea->a_nProtection & AREA_REMAPPED ) == 0;
	pPgd = pgd_offset( psArea->a_psContext, nAddress );

	psArea->a_nProtection &= ~AREA_REMAPPED;
	psArea->a_psFile = psFile;
	psArea->a_nFileOffset = nOffset;
	psArea->a_nFileLength = nSize;
	psArea->a_psOps = &sMemMapOps;


	while ( nAddress - 1 < nEnd )
	{
		int nSkipSize;

		unmap_pagedir( pPgd++, nAddress, nEnd - nAddress, bFreeOldPages );

		nSkipSize = ( ( nAddress + PGDIR_SIZE ) & PGDIR_MASK ) - nAddress;
		nAddress += nSkipSize;
	}

	if ( psInode->i_psAreaList == NULL )
	{
		psInode->i_psAreaList = psArea;
		psArea->a_psNextShared = psArea;
		psArea->a_psPrevShared = psArea;
	}
	else
	{
		MemArea_s *psSrcArea = psInode->i_psAreaList;

		psArea->a_psNextShared = psSrcArea->a_psNextShared;
		psArea->a_psNextShared->a_psPrevShared = psArea;
		psSrcArea->a_psNextShared = psArea;
		psArea->a_psPrevShared = psSrcArea;
	}

	for ( psTmp = psArea->a_psNextShared; psTmp != psArea; psTmp = psTmp->a_psNextShared )
	{
		kassertw( psTmp->a_psFile != NULL );
		kassertw( psTmp->a_psFile->f_psInode->i_nInode == psArea->a_psFile->f_psInode->i_nInode );
	}
//    unlock_semaphore_ex( psArea->a_hMutex, AREA_MUTEX_COUNT );
	unlock_area( psArea, LOCK_AREA_WRITE );
	put_area( psArea );
	UNLOCK( g_hAreaTableSema );

	atomic_inc( &psFile->f_nRefCount );
	flush_tlb_global();
	return ( nError );
}

/**
 * Clones the specified area at a new address.
 * \ingroup Syscalls
 * \param pzName a pointer to the string containing the name of the new area.
 * \param ppAddress a pointer to the variable where the start address of the
 *     new area will be stored, or <code>NULL</code>.  If non-
 *     <code>NULL</code>, the previous value of the variable it points
 *     to will be used as the preferred start address of the new area.
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \param hSrcArea the <code>area_id</code> of the area to clone.
 * \return <code>-EFAULT</code> if there is a memory access violation while
 *     copying <i>pzName</i>; <code>-EINVAL</code> if the preferred starting
 *     address at <i>*ppAddress</i> is not a multiple of <code>PAGE_SIZE</code>;
 *     the <code>area_id</code> of the new area otherwise.
 * \sa alloc_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
area_id sys_clone_area( const char *pzName, void **ppAddress,
		        flags_t nProtection, flags_t nLockMode,
			area_id hSrcArea )
{
	Process_s *psProc = CURRENT_PROC;
	MemArea_s *psSrcArea;
	MemArea_s *psNewArea;
	area_id nNewArea;
	uintptr_t nAddress;
	uintptr_t nSrcAddr;
	uintptr_t nDstAddr;
	status_t nError;
	MemContext_s *psDstSeg;
	MemContext_s *psSrcSeg;
	char zName[ OS_NAME_LENGTH ];

	if ( pzName != NULL )
	{
		if ( strncpy_from_user( zName, pzName, OS_NAME_LENGTH ) < 0 )
		{
			return ( -EFAULT );
		}
		zName[OS_NAME_LENGTH - 1] = '\0';
	}
	else
	{
		zName[0] = '\0';
	}
	if ( NULL == ppAddress )
	{
		nAddress = AREA_FIRST_USER_ADDRESS + 512 * 1024 * 1024;
		nProtection = ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_BASE_ADDRESS;
	}
	else
	{
		nAddress = ( uintptr_t )*ppAddress;
		if ( nAddress < AREA_FIRST_USER_ADDRESS )
		{
			nAddress = AREA_FIRST_USER_ADDRESS + 512 * 1024 * 1024;
			nProtection = ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_BASE_ADDRESS;
		}
		else if ( ( nAddress & ~PAGE_MASK ) != 0 )
		{
			printk( "Error: sys_clone_area() unaligned address %p\n", (void*)( nAddress ) );
			return ( -EINVAL );
		}
	}
      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	if ( psProc->tc_psMemSeg->mc_bBusy )
	{
		UNLOCK( g_hAreaTableSema );
		printk( "sys_clone_area(): wait for segment to be ready\n" );
		snooze( 10000 );
		goto again;
	}

	psSrcArea = get_area_from_handle( hSrcArea );

	if ( NULL == psSrcArea )
	{
		nError = -EINVAL;
		goto error;
	}
	lock_area( psSrcArea, LOCK_AREA_READ );


	nError = alloc_area( zName, psProc->tc_psMemSeg, nProtection, nLockMode, nAddress, psSrcArea->a_nEnd - psSrcArea->a_nStart + 1, psSrcArea->a_nMaxEnd - psSrcArea->a_nStart + 1, &nNewArea );
	if ( nError < 0 )
	{
		unlock_area( psSrcArea, LOCK_AREA_READ );
		put_area( psSrcArea );
		UNLOCK( g_hAreaTableSema );

		if ( nError == -ENOMEM )
		{
			shrink_caches( 65536 );
			goto again;
		}
		return ( nError );
	}
	psNewArea = get_area_from_handle( nNewArea );
	atomic_dec( &psNewArea->a_nRefCount );

	psDstSeg = psNewArea->a_psContext;
	psSrcSeg = psSrcArea->a_psContext;

	nDstAddr = psNewArea->a_nStart;
	for ( nSrcAddr = psSrcArea->a_nStart; nSrcAddr < psSrcArea->a_nEnd; nSrcAddr += PAGE_SIZE, nDstAddr += PAGE_SIZE )
	{
		pgd_t *pSrcPgd = pgd_offset( psSrcSeg, nSrcAddr );
		pte_t *pSrcPte = pte_offset( pSrcPgd, nSrcAddr );
		pgd_t *pDstPgd = pgd_offset( psDstSeg, nDstAddr );
		pte_t *pDstPte = pte_offset( pDstPgd, nDstAddr );
		
		clone_page_pte( pDstPte, pSrcPte, false );

	}
	psSrcArea->a_nProtection |= AREA_SHARED;
	psNewArea->a_nProtection |= AREA_SHARED;
	if ( psSrcArea->a_nProtection & AREA_REMAPPED )
	{
		psNewArea->a_nProtection |= AREA_REMAPPED;
	}

	psNewArea->a_psNextShared = psSrcArea->a_psNextShared;
	psNewArea->a_psNextShared->a_psPrevShared = psNewArea;
	psSrcArea->a_psNextShared = psNewArea;
	psNewArea->a_psPrevShared = psSrcArea;

	if ( NULL != ppAddress )
	{
		*ppAddress = ( void * )psNewArea->a_nStart;
	}
	unlock_area( psSrcArea, LOCK_AREA_READ );
	put_area( psSrcArea );
	flush_tlb_global();
	UNLOCK( g_hAreaTableSema );
	return ( psNewArea->a_nAreaID );
      error:
	UNLOCK( g_hAreaTableSema );
	return ( nError );
}

/**
 * Creates a new memory area in the current process.
 * \ingroup DriverAPI
 * \param pzName a pointer to the string containing the name for the new area.
 * \param ppAddress a pointer to the variable where the start address of the
 *     new area will be stored, or <code>NULL</code>.  If non-
 *     <code>NULL</code>, the previous value of the variable it points
 *     to will be used as the preferred start address of the new area.
 * \param nSize the size in bytes of the requested area.
 * \param nMaxSize the maximum size in bytes of the requested area.
 * \param nProtection a protection bitmask containing any combination of:
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \return <code>-EINVAL</code> if the preferred starting address at
 *     <i>*ppAddress</i> is not a multiple of <code>PAGE_SIZE</code>;
 *     <code>-ENOADDRSPC</code> if there isn't enough virtual address space,
 *     <code>-ENOMEM</code> if there isn't enough physical memory;
 *     the <code>area_id</code> of the new area otherwise.
 * \sa alloc_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
area_id create_area( const char *pzName, void **ppAddress, size_t nSize,
		     size_t nMaxSize, flags_t nProtection, flags_t nLockMode )
{
	MemContext_s *psSeg;
	area_id hArea;
	uintptr_t nAddress;
	status_t nError;

	if ( NULL == ppAddress )
	{
		nAddress = 0;
		nProtection = ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_ANY_ADDRESS;
	}
	else
	{
		nAddress = ( uintptr_t )*ppAddress;
		if ( ( nAddress & ~PAGE_MASK ) != 0 )
		{
			printk( "Error: create_area() unaligned address %p\n", (void*)(nAddress) );
			return ( -EINVAL );
		}
	}

	if ( nProtection & AREA_KERNEL )
	{
		psSeg = g_psKernelSeg;
	}
	else
	{
		psSeg = CURRENT_PROC->tc_psMemSeg;
	}
      again:
	LOCK( g_hAreaTableSema );
	if ( psSeg->mc_bBusy )
	{			// True while context is being cloned
		UNLOCK( g_hAreaTableSema );
		printk( "create_area(): wait for segment to be ready\n" );
		snooze( 10000 );
		goto again;
	}
	nError = alloc_area( pzName, psSeg, nProtection, nLockMode, nAddress, nSize, nMaxSize, &hArea );
	UNLOCK( g_hAreaTableSema );
	if ( nError == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}
	if ( nError < 0 )
	{
		return ( nError );
	}

	if ( ppAddress != NULL )
	{
		AreaInfo_s sInfo;

		get_area_info( hArea, &sInfo );
		*ppAddress = sInfo.pAddress;
	}
	return ( hArea );
}

/**
 * Creates a new memory area with the specified attributes.
 * \ingroup Syscalls
 * \param pzName a pointer to the string containing the name for the new area.
 * \param ppAddress a pointer to the variable where the start address of the
 *     new area will be stored, or <code>NULL</code>.  If non-
 *     <code>NULL</code>, the previous value of the variable it points
 *     to will be used as the preferred start address of the new area.
 * \param nSize the size in bytes of the requested area.
 * \param nProtection a protection bitmask containing any combination of
 *     <code>AREA_READ</code>, <code>AREA_WRITE</code>, <code>AREA_EXEC</code>,
 *     <code>AREA_KERNEL</code>, and <code>AREA_WRCOMB</code>.
 * \param nLockMode the locking mode to use: <code>AREA_NO_LOCK</code>,
 *     <code>AREA_LAZY_LOCK</code>, <code>AREA_FULL_LOCK</code>, or
 *     <code>AREA_CONTIGUOUS</code>.
 * \return <code>-EFAULT</code> if there is a memory access violation while
 *     copying <i>pzName</i>; <code>-EINVAL</code> if the preferred start
 *     address at <i>*ppAddress</i> is not a multiple of <code>PAGE_SIZE</code>;
 *     <code>-ENOADDRSPC</code> if there isn't enough virtual address space,
 *     <code>-ENOMEM</code> if there isn't enough physical memory; the
 *     <code>area_id</code> of the new area otherwise.
 * \sa alloc_area(), do_create_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
area_id sys_create_area( const char *pzName, void **ppAddress, size_t nSize,
			 flags_t nProtection, flags_t nLockMode )
{
	char zName[ OS_NAME_LENGTH ];
	area_id hArea;
	void *pAddress;

	if ( nProtection & AREA_KERNEL )
	{
		printk( "Error: sys_create_area() attempt to create kernel area from user-space\n" );
		return ( -EINVAL );
	}

	if ( pzName != NULL )
	{
		if ( strncpy_from_user( zName, pzName, OS_NAME_LENGTH ) < 0 )
		{
			return ( -EFAULT );
		}
		zName[OS_NAME_LENGTH - 1] = '\0';
	}
	else
	{
		zName[0] = '\0';
	}

	if ( NULL == ppAddress )
	{
		pAddress = ( void * )( AREA_FIRST_USER_ADDRESS + 512 * 1024 * 1024 );
		nProtection = ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_BASE_ADDRESS;
	}
	else
	{
		if ( memcpy_from_user( &pAddress, ppAddress, sizeof( pAddress ) ) < 0 )
		{
			return ( -EFAULT );
		}
		if ( ( uintptr_t )pAddress < AREA_FIRST_USER_ADDRESS )
		{
			pAddress = ( void * )( AREA_FIRST_USER_ADDRESS + 512 * 1024 * 1024 );
			nProtection = ( nProtection & ~AREA_ADDR_SPEC_MASK ) | AREA_BASE_ADDRESS;
		}
		else if ( ( ( ( uintptr_t )pAddress ) & ~PAGE_MASK ) != 0 )
		{
			printk( "Error: sys_create_area() unaligned address %p\n", pAddress );
			return ( -EINVAL );
		}
	}

	hArea = create_area( zName, &pAddress, nSize, nSize, nProtection, nLockMode );

	if ( hArea >= 0 && ppAddress != NULL )
	{
		memcpy_to_user( ppAddress, &pAddress, sizeof( pAddress ) );
	}
	return ( hArea );
}

/**
 * Deletes the specified area.
 * \ingroup Syscalls
 * \param hArea a handle to the area to delete.
 * \return <code>-EINVAL</code> if the area handle is invalid or if the area
 *     doesn't belong to this process; <code>0</code> otherwise.
 * \sa do_delete_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t sys_delete_area( area_id hArea )
{
	MemContext_s *psCtx = CURRENT_PROC->tc_psMemSeg;
	MemArea_s *psArea;
	status_t nError = 0;

      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );

	if ( psArea == NULL || psArea->a_psContext != psCtx )
	{
		if ( psArea == NULL )
		{
			printk( "Error: sys_delete_area() attempt to delete invalid area %d\n", hArea );
		}
		else
		{
			printk( "Error: sys_delete_area() attempt to delete area %d not belonging to us\n", hArea );
		}
		nError = -EINVAL;
		goto error;
	}

	if ( psCtx->mc_bBusy )
	{
		atomic_dec( &psArea->a_nRefCount );
		UNLOCK( g_hAreaTableSema );
		printk( "sys_delete_area(): wait for segment to become ready\n" );
		snooze( 10000 );
		goto again;
	}

	if ( atomic_sub_and_test( &psArea->a_nRefCount, 2 ) )
	{
		lock_area( psArea, LOCK_AREA_WRITE );

		nError = do_delete_area( psCtx, psArea );
	}
      error:
	UNLOCK( g_hAreaTableSema );
	return ( nError );

}

/**
 * Returns the starting physical address for the specified area.
 * \internal
 * \ingroup Areas
 * \param hArea a handle to the area.
 * \param pnAddress a pointer to the variable where the starting physical
 *     address for <i>hArea</i> will be copied.
 * \return <code>-EINVAL</code> if <i>hArea</i> is invalid; <code>0</code>
 *     otherwise.
 * \warning The area table semaphore <code>g_hAreaTableSema</code> must already
 *     be locked.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t do_get_area_physical_address( area_id hArea, uintptr_t* pnAddress )
{
	MemArea_s *psArea;
	status_t nError = 0;
	pgd_t *pPgd;
	pte_t *pPte;
	uintptr_t nPageAddress;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	psArea = get_area_from_handle( hArea );

	if ( NULL == psArea )
	{
		nError = -EINVAL;
		goto error;
	}
	
	pPgd = pgd_offset( psArea->a_psContext, psArea->a_nStart );
	
	if( PGD_PAGE( *pPgd ) == 0 ) 
	{
		/* Page Table not present */
		printk( "Error: get_area_physical_address() No page directory for address %p\n", (void*)( psArea->a_nStart ) );
		return ( -EINVAL );
	}
	
	pPte = pte_offset( pPgd, psArea->a_nStart );

	if ( PTE_ISPRESENT( *pPte ) )
	{
		nPageAddress = PTE_PAGE( *pPte );
		*pnAddress = nPageAddress;
	} else {
		nError = -EINVAL;
	}

	put_area( psArea );
      error:
	return ( nError );
}

/**
 * Returns the starting physical memory address for the specified area.
 * \ingroup DriverAPI
 * \param hArea a handle to the area.
 * \param pnAddress a pointer to the variable where the starting physical
 *     address for <i>hArea</i> will be copied.
 * \return <code>-EINVAL</code> if <i>hArea</i> is invalid; <code>0</code>
 *     otherwise.
 * \sa claim_device(), unregister_device(), do_get_area_physical_address()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
status_t get_area_physical_address( area_id hArea, uintptr_t* pnAddress )
{
	status_t nError;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	nError = do_get_area_physical_address( hArea, pnAddress );
	UNLOCK( g_hAreaTableSema );

	return ( nError );
}

/**
 * Returns an <code>AreaInfo_s</code> structure for the specified area.
 * \internal
 * \ingroup Areas
 * \param hArea a handle to the area for which to return information.
 * \param psInfo a pointer to the <code>AreaInfo_s</code> in which to store
 *     the results.
 * \return <code>-EINVAL</code> if the area handle is invalid or the area
 *     doesn't belong to this process; <code>0</code> otherwise.
 * \warning The area table semaphore <code>g_hAreaTableSema</code> must already
 *     be locked.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t do_get_area_info( area_id hArea, AreaInfo_s * psInfo )
{
	MemArea_s *psArea;
	status_t nError = 0;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	psArea = get_area_from_handle( hArea );

	if ( NULL == psArea )
	{
		nError = -EINVAL;
		goto error;
	}

	psInfo->zName[0] = '\0';
	psInfo->hAreaID = psArea->a_nAreaID;
	psInfo->nSize = psArea->a_nEnd - psArea->a_nStart + 1;
	psInfo->nLock = psArea->a_nLockMode;
	psInfo->nProtection = psArea->a_nProtection;
//      psInfo->hProcess                =       psArea->;
//      psInfo->nAllocSize;
	psInfo->pAddress = ( void * )psArea->a_nStart;

	put_area( psArea );
      error:
	return ( nError );
}

/**
 * Returns an <code>AreaInfo_s</code> for the specified area.
 * \ingroup DriverAPI
 * \param hArea a handle to the area for which to return info.
 * \param psInfo a pointer to the <code>AreaInfo_s</code> in which to store the
 *     results.
 * \return <code>-EINVAL</code> if the area handle is invalid or the area
 *     doesn't belong to this process; <code>0</code> otherwise.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t get_area_info( area_id hArea, AreaInfo_s * psInfo )
{
	status_t nError;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	nError = do_get_area_info( hArea, psInfo );
	UNLOCK( g_hAreaTableSema );

	return ( nError );
}

/**
 * Returns an <code>AreaInfo_s</code> for the specified area.
 * \ingroup Syscalls
 * \param hArea a handle to the area for which to return info.
 * \param psInfo a pointer to the <code>AreaInfo_s</code> in which to store the
 *     results.
 * \return <code>-EINVAL</code> if the area handle is invalid or the area
 *     doesn't belong to this process; <code>-EFAULT</code> if there is a
 *     memory access violation while copying to <i>psInfo</i>; <code>0</code>
 *     otherwise.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t sys_get_area_info( area_id hArea, AreaInfo_s * psInfo )
{
	AreaInfo_s sInfo;
	status_t nError;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	nError = do_get_area_info( hArea, &sInfo );
	UNLOCK( g_hAreaTableSema );
	if ( nError >= 0 )
	{
		nError = memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) );
	}

	return ( nError );
}

/**
 * Remaps the specified region of virtual memory to point to a different
 *  region of physical memory.  Only the entries in the page table containing
 *  the entry for the start address will be updated.
 * \internal
 * \ingroup Areas
 * \param pPgd a pointer to the page directory containing the address of the
 *     page table for the region to remap.
 * \param nAddress the start memory address of the region to remap.
 * \param nSize the size in bytes of the memory region to remap.
 * \param nPhysAddress the new start address in physical memory for the region.
 * \param bFreeOldPages <code>true</code> to free the old pages before
 *     remapping them; <code>false</code> otherwise.
 * \return Always returns <code>0</code>.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t remap_pagedir( pgd_t * pPgd, uintptr_t nAddress, size_t nSize,
			       uintptr_t nPhysAddress, bool bFreeOldPages )
{
	uintptr_t nEnd;
	pte_t *pPte = pte_offset( pPgd, nAddress );

	nAddress &= ~PGDIR_MASK;
	nEnd = nAddress + nSize - 1;

	if ( nEnd > PGDIR_SIZE )
	{
		nEnd = PGDIR_SIZE;
	}

	do
	{
		if ( bFreeOldPages )
		{
			if ( PTE_ISPRESENT( *pPte ) )
			{
				uint32_t nOldPage = PTE_PAGE( *pPte );
				int nPageNr = PAGE_NR( nOldPage );

				if ( 0 != nOldPage && nOldPage != ( uint32_t )g_sSysBase.ex_pNullPage )
				{
					if ( atomic_read( &g_psFirstPage[nPageNr].p_nCount ) == 1 )
					{
						unregister_swap_page( nOldPage );
					}
					do_free_pages( nOldPage, 1 );
				}
			}
			else if ( PTE_PAGE( *pPte ) != 0 )
			{
				free_swap_page( PTE_PAGE( *pPte ) );
			}
		}
		PTE_VALUE( *pPte ) = nPhysAddress | PTE_PRESENT | PTE_WRITE | PTE_USER;
		pPte++;

		nPhysAddress += PAGE_SIZE;
		nAddress += PAGE_SIZE;
	}
	while ( nAddress < nEnd );
	flush_tlb_global();

	return ( 0 );
}

/**
 * Remaps the specified area to use a different region of physical memory.
 * \internal
 * \ingroup Areas
 * \param psArea a pointer to the <code>MemArea_s</code> to remap.
 * \param nPhysAddress the new start address in physical memory for the region.
 * \return <code>-EINVAL</code> if <i>nPhysAddress</i> isn't a multiple of
 *     <code>PAGE_SIZE</code>; <code>0</code> otherwise.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static status_t do_remap_area( MemArea_s *psArea, uintptr_t nPhysAddress )
{
	uintptr_t nAddress = psArea->a_nStart;
	uintptr_t nEnd = psArea->a_nEnd;
	uintptr_t nPhys = nPhysAddress;
	pgd_t *pPgd;
	status_t nError = 0;
	bool bFreeOldPages = ( psArea->a_nProtection & AREA_REMAPPED ) == 0;

	if ( nPhysAddress & ~PAGE_MASK )
	{
		printk( "Error : Attempt to remap area to unaligned physical address %p\n", (void*)( nPhysAddress ) );
		return ( -EINVAL );
	}

	pPgd = pgd_offset( psArea->a_psContext, nAddress );

	while ( nAddress - 1 < nEnd )
	{
		intptr_t nOffset;

		nError = remap_pagedir( pPgd++, nAddress, nEnd - nAddress, nPhysAddress, bFreeOldPages );
		if ( 0 != nError )
		{
			break;
		}
		nOffset = ( ( nAddress + PGDIR_SIZE ) & PGDIR_MASK ) - nAddress;
		nPhysAddress += nOffset;
		nAddress += nOffset;
	}
	psArea->a_nProtection |= AREA_REMAPPED;
	/* Allocate a mtrr descriptor and set it to write-combining */
	if( psArea->a_nProtection & AREA_WRCOMB )
	{
		if( alloc_mtrr_desc( nPhys, psArea->a_nEnd - psArea->a_nStart + 1, 1 ) != 0 )
			psArea->a_nProtection &= ~AREA_WRCOMB;
	}
	flush_tlb_global();
	return ( nError );
}

/**
 * Remaps the specified area to use a different region of physical memory.
 * \ingroup Syscalls
 * \param hArea a handle to the area to remap.
 * \param pPhysAddress the new start address in physical memory for the region.
 * \return <code>-EINVAL</code> if the area handle is invalid or if
 *     <i>nPhysAddress</i> isn't a multiple of <code>PAGE_SIZE</code>;
 *     <code>0</code> otherwise.
 * \sa do_remap_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t sys_remap_area( area_id hArea, void *pPhysAddress )
{
	MemArea_s *psArea;
	status_t nError = 0;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );

	if ( NULL == psArea )
	{
		nError = -EINVAL;
		goto error;
	}
	lock_area( psArea, LOCK_AREA_WRITE );
	nError = do_remap_area( psArea, ( uint32_t )pPhysAddress );
	unlock_area( psArea, LOCK_AREA_WRITE );
	put_area( psArea );
      error:
	UNLOCK( g_hAreaTableSema );
	return ( nError );
}

/**
 * Remaps the specified area to use a different region of physical memory.
 * \ingroup DriverAPI
 * \param hArea a handle to the area to remap.
 * \param pPhysAddress the new start address in physical memory for the region.
 * \return <code>-EINVAL</code> if the area handle is invalid or if
 *     <i>nPhysAddress</i> isn't a multiple of <code>PAGE_SIZE</code>;
 *     <code>0</code> otherwise.
 * \sa do_remap_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t remap_area( area_id hArea, void *pPhysAddress )
{
	MemArea_s *psArea;
	status_t nError = 0;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	psArea = get_area_from_handle( hArea );

	if ( NULL == psArea )
	{
		nError = -EINVAL;
		goto error;
	}
	lock_area( psArea, LOCK_AREA_WRITE );
	nError = do_remap_area( psArea, ( uint32_t )pPhysAddress );
	unlock_area( psArea, LOCK_AREA_WRITE );
	put_area( psArea );
      error:
	UNLOCK( g_hAreaTableSema );
	return ( nError );
}

/**
 * Copies a region of memory from kernel space into user space.
 * \ingroup DriverAPI
 * \param pDst the starting address of the destination region.
 * \param pSrc the starting address of the source region.
 * \param nSize the number of bytes to copy.
 * \return The error code from verify_mem_area() on failure; <code>0</code>
 *     otherwise.
 * \sa verify_mem_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t memcpy_to_user( void *pDst, const void *pSrc, size_t nSize )
{
	if ( g_bKernelInitialized )
	{
		status_t nError = verify_mem_area( pDst, nSize, true );

		if ( nError < 0 )
		{
			return ( nError );
		}
		memcpy( pDst, pSrc, nSize );
		return ( 0 );
	}
	else
	{
		memcpy( pDst, pSrc, nSize );
		return ( 0 );
	}
}

/**
 * Copies a region of memory from user space into kernel space.
 * \ingroup DriverAPI
 * \param pDst the starting address of the destination region.
 * \param pSrc the starting address of the source region.
 * \param nSize the number of bytes to copy.
 * \return The error code from verify_mem_area() on failure; <code>0</code>
 *     otherwise.
 * \sa verify_mem_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t memcpy_from_user( void *pDst, const void *pSrc, size_t nSize )
{
	if ( g_bKernelInitialized )
	{
		status_t nError = verify_mem_area( pSrc, nSize, false );

		if ( nError < 0 )
		{
			return ( nError );
		}
		memcpy( pDst, pSrc, nSize );
		return ( 0 );
	}
	else
	{
		memcpy( pDst, pSrc, nSize );
		return ( 0 );
	}
}

/**
 * Copies a string from user space into kernel space.
 * \ingroup DriverAPI
 * \param pzDst the starting address of the destination region.
 * \param pzSrc the address of the string to copy.
 * \param nMaxLen the maximum size of the string to copy.
 * \return The error code from verify_mem_area() on failure; <code>0</code>
 *     otherwise.
 * \sa verify_mem_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t strncpy_from_user( char *pzDst, const char *pzSrc, size_t nMaxLen )
{
	if ( g_bKernelInitialized )
	{
		size_t nMaxSize = ( ( ( ( uintptr_t )pzSrc ) + PAGE_SIZE ) & PAGE_MASK ) - ( ( uintptr_t )pzSrc );
		size_t nLen = 0;
		bool bDone = false;

		if ( nMaxSize > nMaxLen )
		{
			nMaxSize = nMaxLen;
		}

		while ( bDone == false && nLen < nMaxLen )
		{
			status_t nError = verify_mem_area( pzSrc, nMaxSize, false );

			if ( nError < 0 )
			{
				return ( nError );
			}
			for ( uint_fast32_t i = 0; i < nMaxSize; ++i )
			{
				pzDst[i] = pzSrc[i];
				if ( pzSrc[i] == '\0' )
				{
					bDone = true;
					break;
				}
				nLen++;
			}
			pzSrc += nMaxSize;
			pzDst += nMaxSize;
			nMaxSize = PAGE_SIZE;
			if ( nMaxSize > nMaxLen - nLen )
			{
				nMaxSize = nMaxLen - nLen;
			}
		}
		return ( nLen );
	}
	else
	{
		strncpy( pzDst, pzSrc, nMaxLen );
		return ( strlen( pzDst ) );
	}
}

/**
 * Copies a string from user space into a newly allocated kernel region.
 * \ingroup DriverAPI
 * \param pzSrc the address of the string to copy.
 * \param nMaxLen the maximum size of the string to copy.
 * \param ppzDst a pointer to the variable in which to store a pointer to
 *     the new string.
 * \return <code>-ENAMETOOLONG</code> if the length of the string, including
 *     the trailing <code>'\\0'</code>, is greater than <i>nMaxLen</i>,
 *     <code>-ENOMEM</code> if there isn't enough free memory for kmalloc(),
 *     any error code from verify_mem_area() on failure; <code>0</code>
 *     otherwise.
 * \sa verify_mem_area(), kmalloc()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t strndup_from_user( const char *pzSrc, size_t nMaxLen, char **ppzDst )
{
	ssize_t nLen = strlen_from_user( pzSrc );
	char *pzDst;

	if ( nLen < 0 )
	{
		return ( nLen );
	}
	if ( ( size_t )( nLen ) > nMaxLen - 1 )
	{
		return ( -ENAMETOOLONG );
	}
	pzDst = kmalloc( nLen + 1, MEMF_KERNEL | MEMF_OKTOFAIL );
	if ( pzDst == NULL )
	{
		return ( -ENOMEM );
	}

	status_t nError = strncpy_from_user( pzDst, pzSrc, nLen + 1 );
	if ( nError != nLen )
	{
		kfree( pzDst );
		if ( nError < 0 )
		{
			return ( nError );
		}
		else
		{
			return ( -EFAULT );	// The string was modified before we was able to copy it.
		}
	}
	*ppzDst = pzDst;
	return ( nLen );
}

/**
 * Copies a string from kernel space into user space.
 * \ingroup DriverAPI
 * \param pzDst the starting address of the destination region.
 * \param pzSrc the address of the string to copy.
 * \return The error code from verify_mem_area() on failure; <code>0</code>
 *     otherwise.
 * \sa verify_mem_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t strcpy_to_user( char *pzDst, const char *pzSrc )
{
	return ( memcpy_to_user( pzDst, pzSrc, strlen( pzSrc ) ) );
}

/**
 * Returns the length of the specified string.
 * \ingroup DriverAPI
 * \param pzString the starting address of the string.
 * \return The error code from verify_mem_area() on failure; the length of the
 *     string otherwise.
 * \sa verify_mem_area()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t strlen_from_user( const char *pzString )
{
	if ( g_bKernelInitialized )
	{
		size_t nMaxSize = ( ( ( ( uintptr_t )pzString ) + PAGE_SIZE ) & PAGE_MASK ) - ( ( uintptr_t )pzString );
		status_t nLen = 0;
		bool bDone = false;

		while ( bDone == false )
		{
			status_t nError = verify_mem_area( pzString, nMaxSize, false );

			if ( nError < 0 )
			{
				return ( nError );
			}
			for ( count_t i = 0; i < nMaxSize; ++i )
			{
				if ( pzString[i] == '\0' )
				{
					bDone = true;
					break;
				}
				nLen++;
			}
			pzString += nMaxSize;
			nMaxSize = PAGE_SIZE;
		}
		return ( nLen );
	}
	else
	{
		return ( strlen( pzString ) );
	}
}

/**
 * Verifies a region of user memory for kernel access.
 * \ingroup DriverAPI
 * \param pAddress the start address of the region to lock.
 * \param nSize the size in bytes of the region to lock.
 * \param bWriteAccess <code>true</code> to lock the region for write access;
 *     <code>false</code> otherwise.
 * \return <code>-EFAULT</code> if <i>pAddress</i> points to an address in
 *     kernel space, doesn't correspond to an area in user space, or if
 *     <code>(pAddress + nSize - 1)</code> extends beyond the end of the area;
 *     <code>-ENOMEM</code> if there isn't enough physical memory available;
 *     <code>0</code> otherwise.
 * \sa alloc_area_pages()
 * \author Kurt Skauen (kurt@atheos.cx)
 */
status_t verify_mem_area( const void *pAddress, size_t nSize, bool bWriteAccess )
{
	MemArea_s *psArea;
	uintptr_t nAddr = ( uintptr_t )pAddress;
	uintptr_t nCurrent = nAddr & PAGE_MASK;
	uintptr_t nEnd = ( nAddr + nSize + PAGE_SIZE - 1 ) & PAGE_MASK;
	status_t nError;
	bool bPagesAllocated = true;

	if ( nAddr < AREA_FIRST_USER_ADDRESS )
	{
		printk( "verify_mem_area() got kernel address %08x\n", nAddr );
		trace_stack( 0, NULL );
		return ( -EFAULT );
	}
	
	/* For performance reasons we are satified if all pages are allocated */
	for ( ; ( nCurrent < nEnd ) && bPagesAllocated; nCurrent += PAGE_SIZE )
	{
		pgd_t *pPgd = pgd_offset( CURRENT_PROC->tc_psMemSeg, nCurrent );
		pte_t *pPte = pte_offset( pPgd, nCurrent );

		if ( PTE_ISPRESENT( *pPte ) )
		{
			if ( bWriteAccess == false || PTE_ISWRITE( *pPte ) )
			{
				continue;	// It is present and writable so nothing to do here.
			}
		} else
			bPagesAllocated = false;
	}
	
	if( bPagesAllocated )
		return( 0 );
	
	    again:
	psArea = get_area( CURRENT_PROC->tc_psMemSeg, nAddr );

	if ( psArea == NULL )
	{
		printk( "verify_mem_area() no area for address %08x\n", nAddr );
		return ( -EFAULT );
	}
	
	LOCK( g_hAreaTableSema );

	if ( nAddr + nSize - 1 > psArea->a_nEnd )
	{
		printk( "verify_mem_area() adr=%08x size=%d does not fit in area %08x-%08x\n", nAddr, nSize, psArea->a_nStart, psArea->a_nEnd );
		UNLOCK( g_hAreaTableSema );
		put_area( psArea );
		return ( -EFAULT );
	}

	nError = alloc_area_pages( psArea, nAddr & PAGE_MASK, ( nAddr + nSize + PAGE_SIZE - 1 ) & PAGE_MASK, bWriteAccess );

	put_area( psArea );
	UNLOCK( g_hAreaTableSema );
	

	if ( nError < 0 )
	{
		if ( nError == -ENOMEM )
		{
			shrink_caches( 65536 );
			goto again;
		}
	}
	return ( nError );
}

/**
 * Adjusts the upper bound of a user process's data segment.
 * \ingroup Syscalls
 * \param nDelta the number of bytes by which to increase or decrease the
 *     data segment.
 * \return <code>-1</code> if there was an error adjusting the data segment;
 *     otherwise, the old upper bound of the process's data segment.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void *sys_sbrk( int nDelta )
{
	Process_s *psProc = CURRENT_PROC;
	void *pOldBrk = ( void * )-1;
	status_t nError = 0;
	size_t nOldSize;


	if ( nDelta > 0 )
	{
		nDelta = ( nDelta + PAGE_SIZE - 1 ) & PAGE_MASK;
	}
	else
	{
		nDelta = -( ( -nDelta ) & PAGE_MASK );
	}
      again:
	LOCK( g_hAreaTableSema );
	if ( psProc->tc_psMemSeg->mc_bBusy )
	{			// True while context is being cloned
		UNLOCK( g_hAreaTableSema );
		printk( "sys_sbrk(): wait for segment to be ready\n" );
		snooze( 10000 );
		goto again;
	}


	if ( psProc->pr_nHeapArea == -1 )
	{
		nOldSize = 0;
	}
	else
	{
		AreaInfo_s sInfo;

		do_get_area_info( psProc->pr_nHeapArea, &sInfo );
		nOldSize = sInfo.nSize;
	}


	if ( ( nOldSize + nDelta ) == 0 )
	{
		if ( psProc->pr_nHeapArea != -1 )
		{
			AreaInfo_s sInfo;

			do_get_area_info( psProc->pr_nHeapArea, &sInfo );

			pOldBrk = ( void * )( ( ( uint32_t )sInfo.pAddress ) + sInfo.nSize );
			printk( "Delete proc area, old address = %p\n", pOldBrk );
			delete_area( psProc->pr_nHeapArea );
			psProc->pr_nHeapArea = -1;
		}
		else
		{
			//printk( "sbrk() Search for address\n" );
			pOldBrk = ( void * )( find_unmapped_area( psProc->tc_psMemSeg, AREA_ANY_ADDRESS, PAGE_SIZE, 0 ) );
			//printk( "sbrk() next address would be %p\n", pOldBrk );
		}
		goto error;
	}

	if ( psProc->pr_nHeapArea == -1 )
	{
		count_t   nReserveSize = 512 * 1024 * 1024;
		uintptr_t nAddress = ( uintptr_t )( -1 );

		if ( nDelta < 0 )
		{
			pOldBrk = ( void * )-1;
			goto error;
		}
		while ( nReserveSize >= PAGE_SIZE )
		{
			nAddress = find_unmapped_area( psProc->tc_psMemSeg, AREA_ANY_ADDRESS, nReserveSize, 0 );
			if ( nAddress != (uintptr_t)(-1) )
			{
				break;
			}
			nReserveSize >>= 1;
		}
		if ( nAddress == (uintptr_t)(-1) )
		{
			nAddress = AREA_FIRST_USER_ADDRESS;
		}
		nAddress = AREA_FIRST_USER_ADDRESS + 128 * 1024 * 1024;
		nError = alloc_area( "heap", psProc->tc_psMemSeg, AREA_BASE_ADDRESS | AREA_FULL_ACCESS, AREA_NO_LOCK, nAddress, nDelta /*PGDIR_SIZE * 16 */ , nDelta, &psProc->pr_nHeapArea );
		if ( nError == -ENOMEM )
		{
			UNLOCK( g_hAreaTableSema );
			shrink_caches( 65536 );
			goto again;
		}
		if ( nError >= 0 )
		{
			AreaInfo_s sInfo;

			do_get_area_info( psProc->pr_nHeapArea, &sInfo );
			pOldBrk = sInfo.pAddress;
		}
		goto error;
	}
	else
	{
		AreaInfo_s sInfo;

		do_get_area_info( psProc->pr_nHeapArea, &sInfo );
		pOldBrk = ( void * )( ( ( uint32_t )sInfo.pAddress ) + sInfo.nSize );

		UNLOCK( g_hAreaTableSema );
		nError = resize_area( psProc->pr_nHeapArea, sInfo.nSize + nDelta, true );
		LOCK( g_hAreaTableSema );

		if ( nError < 0 )
		{
			pOldBrk = ( void * )-1;
		}
	}
      error:
	UNLOCK( g_hAreaTableSema );
	return ( pOldBrk );
}

/**
  * clone_from_inactive_ctx() and update_inactive_ctx() are used to access
  * an inactive address space / memory context (the associated process is not
  * currently running, page directory is not loaded into CR3).
  * clone_from_inactive_ctx() will create in kernel space a clone of (a part
  * of) the specified area. If you have written to the cloned area, call
  * update_inactive_ctx() to make sure that pages allocated by the
  * copy-on-write mechanism are transfered to the originating memory context.
  * \internal
  * \ingroup Areas
  * \param psArea a pointer to the area to clone.
  * \param nOffset
  * \param nLength specifies, together with nOffset, which part of the area is
  *        to be cloned. Both nOffset and nLength are expected to be page
  *        aligned.
  * \author Jan Hauffa (hauffa@in.tum.de)
  */
area_id clone_from_inactive_ctx( MemArea_s *psArea, uintptr_t nOffset,
                                 size_t nLength )
{
	MemArea_s *psNewArea;
	area_id hArea;
	uintptr_t nSrcAddr, nDstAddr;

	if ( ( psArea->a_nEnd - psArea->a_nStart + 1 ) < ( nOffset + nLength ) )
		return -EINVAL;
	if ( psArea->a_nProtection & AREA_KERNEL )
		return -EINVAL;
	if ( psArea->a_psContext->mc_psOwner == CURRENT_PROC )
		return -EINVAL;

	/* TODO: is this even possible? */
	if ( atomic_read( &psArea->a_nIOPages ) > 0 )
	{
		printk( "clone_from_inactive_ctx(): area has unfinished IO ops\n" );
		return -EINVAL;
	}

	lock_area( psArea, LOCK_AREA_READ );

	hArea = create_area( psArea->a_zName, NULL, nLength, nLength,
		(psArea->a_nProtection | AREA_KERNEL) & ~AREA_ADDR_SPEC_MASK,
		psArea->a_nLockMode );
	if ( hArea <= 0 )
	{
		printk( "clone_from_inactive_ctx() failed to alloc area\n" );
		unlock_area( psArea, LOCK_AREA_READ );
		return -ENOMEM;
	}
	printk( "clone_from_inactive_ctx: created area %d\n", hArea );

	LOCK( g_hAreaTableSema );
	psNewArea = get_area_from_handle( hArea );
	kassertw( psNewArea != NULL );

	psNewArea->a_psOps = psArea->a_psOps;

	for ( nDstAddr = psNewArea->a_nStart, nSrcAddr = psArea->a_nStart + nOffset;
	      ( nDstAddr - 1 ) != psNewArea->a_nEnd;
	      nDstAddr += PAGE_SIZE, nSrcAddr += PAGE_SIZE )
	{
		pgd_t *pSrcPgd = pgd_offset( psArea->a_psContext, nSrcAddr );
		pte_t *pSrcPte = pte_offset( pSrcPgd, nSrcAddr );
		pgd_t *pDstPgd = pgd_offset( g_psKernelSeg, nDstAddr );
		pte_t *pDstPte = pte_offset( pDstPgd, nDstAddr );

		kassertw( PGD_PAGE( *pDstPgd ) != 0 );
		*pDstPte = *pSrcPte;

		if ( PTE_ISPRESENT( *pSrcPte ) )
			atomic_inc( &g_psFirstPage[PAGE_NR( PTE_PAGE( *pSrcPte ) )].p_nCount );
		else if ( PTE_PAGE( *pSrcPte ) != 0 )
			dup_swap_page( PTE_PAGE( *pSrcPte ) );
	}

	/* If the source area is mapped to a file, the new one should be, too. */
	if ( psArea->a_psFile )
	{
		psNewArea->a_psFile = psArea->a_psFile;
		psNewArea->a_nFileOffset = psArea->a_nFileOffset + nOffset;
		psNewArea->a_nFileLength = psArea->a_nFileLength - nOffset;
		if ( psNewArea->a_nFileLength > nLength )
			psNewArea->a_nFileLength = nLength;
		psNewArea->a_psNextShared = psArea->a_psNextShared;
		psNewArea->a_psNextShared->a_psPrevShared = psNewArea;
		psArea->a_psNextShared = psNewArea;
		psNewArea->a_psPrevShared = psArea;
		atomic_inc( &psNewArea->a_psFile->f_nRefCount );
	}

	put_area( psNewArea );
	UNLOCK( g_hAreaTableSema );
	unlock_area( psArea, LOCK_AREA_READ );

    /* TODO: invalidate only the pages of the newly created area */
	flush_tlb_global();

	return hArea;
}

/**
  * \sa clone_from_inactive_ctx()
  * \internal
  * \ingroup Areas
  * \author Jan Hauffa (hauffa@in.tum.de)
  */
status_t update_inactive_ctx( MemArea_s *psOriginalArea, area_id hCloneArea,
                              uintptr_t nOffset )
{
	uintptr_t nSrcAddr, nDstAddr;
	MemArea_s *psCloneArea;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	psCloneArea = get_area_from_handle( hCloneArea );

	if ( psCloneArea == NULL )
	{
		printk( "update_inactive_ctx: get_area_from_handle failed\n" );
		goto error;
	}
	if ( ( psOriginalArea->a_nEnd - psOriginalArea->a_nStart + 1 ) <
	     ( psCloneArea->a_nEnd - psCloneArea->a_nStart + nOffset + 1 ) )
		goto error;
	
	lock_area( psCloneArea, LOCK_AREA_READ );
	lock_area( psOriginalArea, LOCK_AREA_WRITE );

	for ( nSrcAddr = psCloneArea->a_nStart, nDstAddr = psOriginalArea->a_nStart + nOffset;
	      ( nSrcAddr - 1 ) != psCloneArea->a_nEnd;
	      nSrcAddr += PAGE_SIZE, nDstAddr += PAGE_SIZE )
	{
		pgd_t *pSrcPgd = pgd_offset( g_psKernelSeg, nSrcAddr );
		pte_t *pSrcPte = pte_offset( pSrcPgd, nSrcAddr );
		pgd_t *pDstPgd = pgd_offset( psOriginalArea->a_psContext, nDstAddr );
		pte_t *pDstPte = pte_offset( pDstPgd, nDstAddr );

		kassertw( PGD_PAGE( *pDstPgd ) != 0 );

		if ( PTE_VALUE( *pDstPte ) != PTE_VALUE( *pSrcPte ) )
		{
			*pDstPte = *pSrcPte;

			if ( PTE_ISPRESENT( *pSrcPte ) )
				atomic_inc( &g_psFirstPage[PAGE_NR( PTE_PAGE( *pSrcPte ) )].p_nCount );
			else if ( PTE_PAGE( *pSrcPte ) != 0 )
				dup_swap_page( PTE_PAGE( *pSrcPte ) );
		}
	}

	unlock_area( psOriginalArea, LOCK_AREA_WRITE );
	unlock_area( psCloneArea, LOCK_AREA_READ );
	UNLOCK( g_hAreaTableSema );

    /* TODO: invalidate only the modified pages of the updated area */
	flush_tlb_global();
	return 0;

error:
	UNLOCK( g_hAreaTableSema );
	return -EINVAL;	
}


/**
 * Creates the memory context for the kernel. This context holds all kernel
 * areas below AREA_FIRST_USER_ADDRESS.
 * \internal
 * \ingroup Areas
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void init_kernel_mem_context()
{
	g_psKernelSeg = ( MemContext_s * )get_free_page( MEMF_CLEAR );

	g_psKernelSeg->mc_pPageDir = ( pgd_t * ) get_free_page( MEMF_CLEAR );

	for ( count_t i = 0; i < ( ( ( g_sSysBase.ex_nTotalPageCount * PAGE_SIZE ) + PGDIR_SIZE - 1 ) / PGDIR_SIZE ); ++i )
	{
		PGD_VALUE( g_psKernelSeg->mc_pPageDir[i] ) =
			MK_PGDIR( ( uintptr_t )( get_free_page( MEMF_CLEAR ) ) );
	}

	for ( count_t i = 0; i < g_sSysBase.ex_nTotalPageCount; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, i * PAGE_SIZE );
		pte_t *pPte = pte_offset( pPgd, i * PAGE_SIZE );

		PTE_VALUE( *pPte ) = ( i * PAGE_SIZE ) | PTE_PRESENT | PTE_WRITE;
	}
	g_psKernelSeg->mc_psNext = g_psKernelSeg;

}

/**
 * Print all areas in psCtx to the debug console.
 * \internal
 * \ingroup Areas
 * \param psCtx - pointer to a MemContext_s structure.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static void db_list_proc_areas( MemContext_s *psCtx )
{
	MemArea_s *psArea;
	
	size_t nMappedToFileSize = 0;
	size_t nSharedSize = 0;
	size_t nRemappedSize = 0;
	size_t nOtherSize = 0;

	for ( count_t i = 0; i < psCtx->mc_nAreaCount; ++i )
	{
		psArea = psCtx->mc_apsAreas[i];

		if ( NULL == psArea )
		{
			dbprintf( DBP_DEBUGGER, "Error: db_list_proc_areas() index %d of %d has a NULL pointer!\n", i, psCtx->mc_nAreaCount );
			break;
		}
		if ( i > 0 && psArea->a_psPrev != psCtx->mc_apsAreas[i - 1] )
		{
			dbprintf( DBP_DEBUGGER, "Error: db_list_proc_areas() area at index %d (%p) has invalid prev pointer %p\n", i, psArea, psArea->a_psPrev );
		}
		if ( i < psCtx->mc_nAreaCount - 1 && psArea->a_psNext != psCtx->mc_apsAreas[i + 1] )
		{
			dbprintf( DBP_DEBUGGER, "Error: db_list_proc_areas() area at index %d (%p) has invalid next pointer %p\n", i, psArea, psArea->a_psNext );
		}
		dbprintf( DBP_DEBUGGER, "Area %04d %s (%p) -> %08x-%08x %i bytes %p %02d\n", i, psArea->a_zName, psArea, psArea->a_nStart, psArea->a_nEnd, psArea->a_nEnd - psArea->a_nStart + 1, psArea->a_psFile, atomic_read( &psArea->a_nRefCount ) );
		if( psArea->a_psFile != 0 )
			nMappedToFileSize += psArea->a_nEnd - psArea->a_nStart + 1;
		else if( psArea->a_nProtection & AREA_REMAPPED )
			nRemappedSize += psArea->a_nEnd - psArea->a_nStart + 1;
		else if( psArea->a_nProtection & AREA_SHARED )
			nSharedSize += psArea->a_nEnd - psArea->a_nStart + 1;
		else
			nOtherSize += psArea->a_nEnd - psArea->a_nStart + 1;
	}
	dbprintf( DBP_DEBUGGER, "------------------------------\n" );
	dbprintf( DBP_DEBUGGER, "Memory mapped size: %i bytes \n", nMappedToFileSize );
	dbprintf( DBP_DEBUGGER, "Remapped size: %i bytes \n", nRemappedSize );
	dbprintf( DBP_DEBUGGER, "Shared size: %i bytes\n", nSharedSize );
	dbprintf( DBP_DEBUGGER, "Other size: %i bytes\n", nOtherSize );
}

/**
 * Prints all areas for the specified process to the debug console.
 * \internal
 * \ingroup Areas
 * \param argc the number of arguments in the debug command.
 * \param argv an array of pointers to the arguments in the debug command.
 * \author Kurt Skauen (kurt@atheos.cx)
 */
static void db_list_areas( int argc, char **argv )
{
	if ( argc != 2 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s proc_id\n", argv[0] );
	}
	else
	{
		proc_id hProc = atol( argv[1] );
		Process_s *psProc = get_proc_by_handle( hProc );

		if ( NULL == psProc )
		{
			dbprintf( DBP_DEBUGGER, "Process %04d does not exist\n", hProc );
			return;
		}
		dbprintf( DBP_DEBUGGER, "Listing areas of %s (%d)\n", psProc->tc_zName, hProc );

		db_list_proc_areas( psProc->tc_psMemSeg );
	}
}

/**
 * Initializes the area manager.
 * \internal
 * \ingroup Areas
 * \author Kurt Skauen (kurt@atheos.cx)
 */
void init_areas( void )
{
	register_debug_cmd( "ls_area", "list all memory areas created by a given process.", db_list_areas );
	MArray_Init( &g_sAreas );
	g_hAreaTableSema = create_semaphore( "area_table", 1, SEM_RECURSIVE );

	do_create_area( g_psKernelSeg, 0, 1024 * 1024, 1024 * 1024, false, AREA_KERNEL | AREA_READ | AREA_WRITE | AREA_EXEC, AREA_FULL_LOCK );
	do_create_area( g_psKernelSeg, 1024 * 1024, g_sSysBase.ex_nTotalPageCount * PAGE_SIZE - 1024 * 1024, g_sSysBase.ex_nTotalPageCount * PAGE_SIZE - 1024 * 1024, false, AREA_KERNEL | AREA_READ | AREA_WRITE | AREA_EXEC, AREA_FULL_LOCK );
	flush_tlb();
}





