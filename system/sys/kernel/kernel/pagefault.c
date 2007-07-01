
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

#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/areas.h"
#include "inc/swap.h"

static const uint32_t WAIT_LENGTH = 10000;


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static uint32 find_clean_page( MemArea_s *psArea, pte_t * pDstPte, uintptr_t nAddress, off_t nOffset, bool bWriteAccess )
{
	MemArea_s *psSrcArea;

	for ( psSrcArea = psArea->a_psNextShared; psSrcArea != psArea; psSrcArea = psSrcArea->a_psNextShared )
	{
		uint64 nSrcAddress = psSrcArea->a_nStart + nOffset - psSrcArea->a_nFileOffset;
		pgd_t *pPgd;
		pte_t *pSrcPte;
		pte_t sOrigPte;

		if ( psSrcArea->a_psFile == NULL )
		{
			printk( "Panic: find_clean_page() found area in share-list not associated with a file!\n" );
			continue;
		}
		kassertw( psSrcArea->a_psFile->f_psInode->i_nInode == psArea->a_psFile->f_psInode->i_nInode );

		if ( psArea->a_nFileOffset != psSrcArea->a_nFileOffset || psArea->a_nFileLength != psSrcArea->a_nFileLength )
		{
			continue;
		}

		// Verify that the page resist within the intersection of the two areas.
		if ( nSrcAddress < psSrcArea->a_nStart || nSrcAddress > ( psSrcArea->a_nStart + psSrcArea->a_nFileLength - 1 ) )
		{
			continue;
		}

		// Make sure noting is mapped in or out while we investigate the area

		lock_area( psSrcArea, LOCK_AREA_READ );

		pPgd = pgd_offset( psSrcArea->a_psContext, nSrcAddress );
		pSrcPte = pte_offset( pPgd, nSrcAddress );


		// Make sure it don't get dirty between our test and the COW
		sOrigPte = *pSrcPte;
		PTE_VALUE( *pSrcPte ) &= ~PTE_WRITE;
		flush_tlb_global();

		if ( ( PTE_VALUE( *pSrcPte ) & PTE_PRESENT ) == 0 || ( PTE_VALUE( *pSrcPte ) & PTE_DIRTY ) )
		{
			PTE_VALUE( *pSrcPte ) |= PTE_VALUE( sOrigPte ) & PTE_WRITE;
			flush_tlb_global();
			unlock_area( psSrcArea, LOCK_AREA_READ );
			continue;
		}

		// If we are about to write to the page we copy it now, else we make a COW sharing
		// and delay the copy til the page is actually written to.

		if ( bWriteAccess )
		{
			uintptr_t nPageAddr = PTE_PAGE( *pSrcPte );
			uintptr_t nNewPage;

			if ( nPageAddr == ( uintptr_t )g_sSysBase.ex_pNullPage )
			{
				printk( "Panic: file-mapped area reference the global null-page for address %08x (%08x)!\n", ( uintptr_t )nSrcAddress, nAddress );
				unlock_area( psSrcArea, LOCK_AREA_READ );
				return ( 0 );
			}

			nNewPage = get_free_page( 0 );
			if ( nNewPage == 0 )
			{
				unlock_area( psSrcArea, LOCK_AREA_READ );
				return ( -ENOMEM );
			}
			memcpy( ( void * )nNewPage, ( void * )nPageAddr, PAGE_SIZE );

			register_swap_page( nNewPage );

			set_pte_address( pDstPte, nNewPage );
			PTE_VALUE( *pDstPte ) |= PTE_WRITE | PTE_ACCESSED | PTE_DIRTY | PTE_PRESENT | PTE_USER;

			PTE_VALUE( *pSrcPte ) |= PTE_VALUE( sOrigPte ) & PTE_WRITE;
		}
		else
		{
			clone_page_pte( pDstPte, pSrcPte, true );
		}
		flush_tlb_global();
		unlock_area( psSrcArea, LOCK_AREA_READ );

		return ( PTE_VALUE( *pSrcPte ) & PAGE_MASK );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:	handle page faults in mem-mapped files
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 memmap_no_page( MemArea_s *psArea, uintptr_t nAddress, bool bWriteAccess )
{
	Process_s *psProc = CURRENT_PROC;
	pgd_t *pPgd = pgd_offset( psProc->tc_psMemSeg, nAddress );
	pte_t *pPte = pte_offset( pPgd, nAddress );
	uintptr_t nNewPage;
	off_t nFileOffset;
	int nSize;
	size_t nBytesRead;
	Page_s *psPage;

	kassertw( psArea->a_psFile != NULL );

	nAddress &= PAGE_MASK;

	nFileOffset = ( ( off_t )nAddress ) - ( ( off_t )psArea->a_nStart ) + psArea->a_nFileOffset;
	nSize = min( PAGE_SIZE, psArea->a_nFileLength - ( nFileOffset - psArea->a_nFileOffset ) );

	nNewPage = find_clean_page( psArea, pPte, nAddress, nFileOffset, bWriteAccess );

	if ( nNewPage != 0 )
	{
		unlock_area( psArea, LOCK_AREA_WRITE );
		return ( 0 );
	}

	nNewPage = get_free_page( MEMF_CLEAR );

	if ( nNewPage == 0 )
	{
		return ( -ENOMEM );
	}

	psPage = &g_psFirstPage[nNewPage >> PAGE_SHIFT];
	psPage->p_nFlags |= PF_BUSY;

	set_pte_address( pPte, nNewPage );
	PTE_VALUE( *pPte ) |= PTE_WRITE | PTE_USER;

	register_swap_page( nNewPage );

	if ( bWriteAccess )
	{
		PTE_VALUE( *pPte ) |= PTE_ACCESSED | PTE_DIRTY;
	}

	if ( nSize <= 0 )
	{
		psPage->p_nFlags &= ~PF_BUSY;
		PTE_VALUE( *pPte ) |= PTE_PRESENT;

		flush_tlb_global();
		
		// Wake up threads bumping into the page while we were loading it.
		if ( psPage->p_psIOThreads != NULL )
		{
			wake_up_queue( true, psPage->p_psIOThreads, 0, true );
		}
		if ( psArea->a_psIOThreads != NULL )
		{
			wake_up_queue( true, psArea->a_psIOThreads, 0, true );
		}	
		return ( 0 );
	}

	atomic_inc( &psArea->a_nIOPages );


	UNLOCK( g_hAreaTableSema );
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	nBytesRead = read_pos_p( psArea->a_psFile, nFileOffset, ( void * )nNewPage, nSize );
	unlock_area( psArea, LOCK_AREA_WRITE );
	LOCK( g_hAreaTableSema );

	psPage->p_nFlags &= ~PF_BUSY;
	PTE_VALUE( *pPte ) |= PTE_PRESENT;

	flush_tlb_global();

	atomic_dec( &psArea->a_nIOPages );

	// Wake up threads bumping into the page while we was loading it.
	if ( psPage->p_psIOThreads != NULL )
	{
		wake_up_queue( true, psPage->p_psIOThreads, 0, true );
	}
	if ( psArea->a_psIOThreads != NULL )
	{
		wake_up_queue( true, psArea->a_psIOThreads, 0, true );
	}

	if ( nBytesRead != nSize )
	{
		printk( "Error : memmap_no_page() failed to read page from disk %Ld - %d (%d)\n", nFileOffset, nSize, nBytesRead );
//    do_exit( SIGBUS );
		return ( -EIO );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int handle_copy_on_write( MemArea_s *psArea, pte_t * pPte, uintptr_t nVirtualAddress )
{
	int nPageAddr = PTE_PAGE( *pPte );
	Page_s *psPage = &g_psFirstPage[nPageAddr >> PAGE_SHIFT];
	uintptr_t nNewPage;

	// COW of the global NULL page is very common, so we handle it specially.
	if ( nPageAddr == ( uintptr_t )g_sSysBase.ex_pNullPage )
	{
		Page_s *psNewPage;

		nNewPage = get_free_page( MEMF_CLEAR );
		psNewPage = &g_psFirstPage[nNewPage >> PAGE_SHIFT];

		if ( nNewPage == 0 )
		{
			return ( -ENOMEM );
		}

		set_pte_address( pPte, nNewPage );
		PTE_VALUE( *pPte ) |= PTE_WRITE;

		if ( psArea->a_nProtection & AREA_SHARED )
		{
			MemArea_s *psTmp;
			uint32 nOffset = nVirtualAddress - psArea->a_nStart;

			for ( psTmp = psArea->a_psNextShared; psTmp != psArea; psTmp = psTmp->a_psNextShared )
			{
				uintptr_t nAddr = psTmp->a_nStart + nOffset;
				pgd_t *pClPgd = pgd_offset( psTmp->a_psContext, nAddr );
				pte_t *pClPte = pte_offset( pClPgd, nAddr );

				kassertw( PTE_ISWRITE( *pClPte ) == false );
				*pClPte = *pPte;
				atomic_inc( &psNewPage->p_nCount );
			}
		}

		flush_tlb_global();
		register_swap_page( nNewPage );
		return ( 0 );
	}

	if ( atomic_read( &psPage->p_nCount ) < 0 )
	{
		printk( "PANIC: handle_copy_on_write()  Page at %08x got ref count of %d!\n", nPageAddr, atomic_read( &psPage->p_nCount ) );
		atomic_set( &psPage->p_nCount, 100 );
	}

	kassertw( ( psArea->a_nProtection & AREA_SHARED ) == 0 );

//  nFlags = cli();
//  lock_pagelist();
	if ( atomic_read( &psPage->p_nCount ) == 1 )
	{
		PTE_VALUE( *pPte ) |= PTE_WRITE;
//    unlock_pagelist();
//    put_cpu_flags( nFlags );
		return ( 0 );
	}
//  unlock_pagelist();
//  put_cpu_flags( nFlags );

	nNewPage = get_free_page( 0 );

	if ( nNewPage == 0 )
	{
		return ( -ENOMEM );
	}

	register_swap_page( nNewPage );
	memcpy( ( void * )nNewPage, ( void * )nPageAddr, PAGE_SIZE );

	set_pte_address( pPte, nNewPage );
	PTE_VALUE( *pPte ) |= PTE_WRITE;

	atomic_dec( &psPage->p_nCount );

//  flush_tlb_page( nVirtualAddress );
	flush_tlb_global();
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static MemArea_s *lookup_area( MemContext_s *psCtx, uintptr_t nAddress )
{
	MemArea_s *psArea;
	int nIndex;

	if ( nAddress < AREA_FIRST_USER_ADDRESS )
	{
		psCtx = g_psKernelSeg;
	}

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );
	for ( ;; )
	{
		nIndex = find_area( psCtx, nAddress );
		if ( nIndex >= 0 )
		{
			psArea = psCtx->mc_apsAreas[nIndex];
			atomic_inc( &psArea->a_nRefCount );
			break;
		}
		else
		{
			psArea = NULL;
			break;
		}
	}
	return ( psArea );
}

static int handle_not_present( MemArea_s *psArea, pte_t * pPte, uintptr_t nAddress, bool bWriteAccess )
{
	int nError;

	if ( PTE_ISPRESENT( *pPte ) )
	{
		printk( "handle_page_fault() not-present page at %08x loaded by another thread\n", nAddress );
		return ( 0 );
	}
	if ( PTE_PAGE( *pPte ) != 0 )
	{
		int nPage;
		Page_s* psPage;
		
		if( PTE_PAGE( *pPte ) & 0x80000000 )
		{
			/* Try to swap in the page */
			nError =  swap_in( pPte );
			if( nError == 0 )
			{
				flush_tlb_global();
				return( 0 );
			} else {
				return( -ENOMEM );
			}
		}
		
		nPage = PTE_PAGE( *pPte );
		psPage = &g_psFirstPage[nPage >> PAGE_SHIFT];

		if ( psPage->p_nFlags & PF_BUSY )
		{
			Thread_s *psThread = CURRENT_THREAD;
			WaitQueue_s sWaitNode;
			int nFlg;

			printk( "Wait for page to be loaded by another thread\n" );
			sWaitNode.wq_hThread = psThread->tr_hThreadID;

			nFlg = cli();	// Make sure we are not pre-empted until we are added to the waitlist
			sched_lock();
			add_to_waitlist( false, &psPage->p_psIOThreads, &sWaitNode );
			psThread->tr_nState = TS_WAIT;
			sched_unlock();
			UNLOCK( g_hAreaTableSema );			
			put_cpu_flags( nFlg );
			Schedule();
			LOCK( g_hAreaTableSema );
			remove_from_waitlist( true, &psPage->p_psIOThreads, &sWaitNode );
		}
		return ( -EAGAIN );

/*	
	panic( "Swapping mechanism is currently broken, and disabled."
	       "Nevertheless someone pretends to have swapped out a page at %08lx\n", nAddress );
	return( -EFAULT );
	UNLOCK( g_hAreaTableSema );
	nError = swap_in( pPte );
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
      
	if ( nError >= 0 ) {
//	flush_tlb_page( nFaultAddr );
	    flush_tlb_global();
	    goto fixed;
	}
	if ( nError == -EAGAIN ) {
//		unlock_semaphore_ex( psArea->a_hMutex, AREA_MUTEX_COUNT );
	    unlock_area( psArea, LOCK_AREA_WRITE );
	    put_area( psArea );
	    UNLOCK( g_hAreaTableSema );
//	snooze( WAIT_LENGTH );
	    shrink_caches();
	    goto again;
	}
	printk( "Panic: failed to swap in page %ld\n", PTE_PAGE( *pPte ) );
	return( -EFAULT );
//	goto bad_pf;
*/
	}
	if ( psArea->a_psFile == NULL )
	{
		printk( "Error: Not present page in area '%s' and the area is not mapped to a file\n", psArea->a_zName );
		return ( -EFAULT );
	}
	nError = memmap_no_page( psArea, nAddress, bWriteAccess );
	return ( nError );
}

static int handle_write_prot( MemArea_s *psArea, pte_t * pPte, uintptr_t nAddress )
{
	if ( PTE_ISWRITE( *pPte ) )
	{
//      printk( "handle_page_fault() page %08lx was COW'ed by another thread\n" );
		return ( 0 );
	}
	return ( handle_copy_on_write( psArea, pPte, nAddress ) );
}

/** 
 * \par Description:
 *	Make the page accessible. This means allocating a new page and
 *	either perform a copy on write or load the page from disk.
 * \par Note:
 *	The given address is NULL based and must be between psArea->a_nStart
 *	and psArea->a_nEnd
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int load_area_page( MemArea_s *psArea, uintptr_t nAddress, bool bWriteAccess )
{
	MemContext_s *psCtx = psArea->a_psContext;
	pgd_t *pPgd;
	pte_t *pPte;
	int nError;


	pPgd = pgd_offset( psCtx, nAddress );
	pPte = pte_offset( pPgd, nAddress );

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	lock_area( psArea, LOCK_AREA_WRITE );

	if ( PTE_ISPRESENT( *pPte ) )
	{
		nError = handle_write_prot( psArea, pPte, nAddress );
	}
	else
	{
		nError = handle_not_present( psArea, pPte, nAddress, bWriteAccess );
	}
	unlock_area( psArea, LOCK_AREA_WRITE );
	return ( nError );
}

void handle_page_fault( SysCallRegs_s * psRegs, int nErrorCode )
{
	MemContext_s *psCtx;
	pgd_t *pPgd;
	pte_t *pPte;
	MemArea_s *psArea = NULL;
	uintptr_t nFaultAddr;
	bool bWriteAccess = ( nErrorCode & PFE_WRITE ) != 0;
	int nError;

	if ( psRegs->eflags & EFLG_VM )
	{
		if ( ( psRegs->cs & 0xffff ) == 0xffff && ( psRegs->eip & 0xffff ) == 0xffff )
		{
			handle_v86_pagefault( ( Virtual86Regs_s * ) psRegs, 0 );
			return;
		}
	}

	// Get the fault address
      __asm__( "movl %%cr2,%0":"=r"( nFaultAddr ) );

#if 0
    if ( (nErrorCode & PFE_USER) == 0 && g_bKernelInitialized ) {
	printk( "Warning: pagefault from kernel at %08lx (EIP=%08lx) (%s:%s:%s)\n", nFaultAddr, psRegs->eip,
		(nErrorCode & PFE_PROTVIOL) ? "PROTV" : "NOTP",
		(nErrorCode & PFE_WRITE)    ? "WRITE" : "READ",
		(nErrorCode & PFE_USER)     ? "USER"  : "SUPER" );
    }
#endif    

      again:
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	if ( nFaultAddr < PAGE_SIZE )
	{
		goto bad_pf;
	}

	if ( nFaultAddr < AREA_FIRST_USER_ADDRESS )
	{
		psCtx = g_psKernelSeg;
	}
	else
	{
		psCtx = CURRENT_PROC->tc_psMemSeg;
	}

	atomic_inc( &g_sSysBase.ex_nPageFaultCount );

	pPgd = pgd_offset( psCtx, nFaultAddr );
	pPte = pte_offset( pPgd, nFaultAddr );

	if ( nFaultAddr < AREA_FIRST_USER_ADDRESS )
	{
		kassertw( pPte == pte_offset( pgd_offset( psCtx, nFaultAddr ), nFaultAddr ) );
	}

	if ( nErrorCode & PFE_PROTVIOL )
	{			// Page is present but not accessible by us
		// If it is a write access it might be a COW if not it is an access violation
		if ( bWriteAccess == false )
		{
			printk( "*** PROTECTION VIOLATION WHILE ACCESSING ADDRESS %08x ***\n", nFaultAddr );
			goto bad_pf;
		}
	}

	psArea = lookup_area( psCtx, nFaultAddr );

	if ( psArea == NULL )
	{
		printk( "*** NO AREA FOR ADDRESS %08x ***\n", nFaultAddr );
		goto bad_pf;
	}
	if ( psArea->a_nEnd < nFaultAddr )
	{
		printk( "*** NO AREA FOR ADDRESS %08x ***\n", nFaultAddr );
		goto bad_pf;
	}
	if ( ( nErrorCode & PFE_USER ) && ( psArea->a_nProtection & AREA_KERNEL ) )
	{
		printk( "*** KERNEL AREA FOR ADDRESS %08x ACCESSED FROM USER-SPACE\n", nFaultAddr );
		goto bad_pf;
	}
	if ( psArea->a_bDeleted )
	{
		printk( "*** DELETED AREA %s ACCESSED AT ADDRESS %08x\n", psArea->a_zName, nFaultAddr );
		goto bad_pf;
	}

	kassertw( psArea->a_psContext == psCtx );
	nError = load_area_page( psArea, nFaultAddr, bWriteAccess );

	put_area( psArea );
	psArea = NULL;
	UNLOCK( g_hAreaTableSema );
	if ( nError == -EAGAIN )
	{
		goto again;
	}
	if ( nError == -ENOMEM )
	{
		if ( shrink_caches( PAGE_SIZE ) <= 0 )
		{
			printk( "Error: handle_page_fault() run out of memory!\n" );
			kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
			LOCK( g_hAreaTableSema );
			goto bad_pf;
		}
		else
		{
			goto again;
		}
	}
	if ( nError < 0 )
	{
		kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
		LOCK( g_hAreaTableSema );
		goto bad_pf;
	}
	return;

      bad_pf:
	if ( psArea != NULL )
	{
		unlock_area( psArea, LOCK_AREA_WRITE );
		put_area( psArea );
	}

	printk( "Invalid pagefault at %08x (%s:%s:%s)\n", nFaultAddr, ( nErrorCode & PFE_PROTVIOL ) ? "PROTV" : "NOTP", ( nErrorCode & PFE_WRITE ) ? "WRITE" : "READ", ( nErrorCode & PFE_USER ) ? "USER" : "SUPER" );

	UNLOCK( g_hAreaTableSema );
	print_registers( psRegs );
	printk( "\n" );
	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );

	printk( "Areas :\n" );
	list_areas( CURRENT_PROC->tc_psMemSeg );

	UNLOCK( g_hAreaTableSema );

	if ( nErrorCode & PFE_USER )
	{
		sys_kill( CURRENT_THREAD->tr_hThreadID, SIGSEGV );
	}
	else
	{
		do_exit( SIGSEGV );
	}
}
