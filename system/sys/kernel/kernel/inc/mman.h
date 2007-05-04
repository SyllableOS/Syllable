
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

#ifndef	_EXEC_MMAN_H_
#define	_EXEC_MMAN_H_

#ifdef __cplusplus
extern "C"
{
#endif
#if 0
}				/* Make emacs auto-indent work. */
#endif

//#include <atheos/swap.h>
#include <atheos/multiboot.h>
#include "typedefs.h"

typedef struct MemChunk MemChunk_s;

struct MemChunk
{
	MemChunk_s *mc_psNext;
	bool mc_bUsed;
	uint32 mc_nAddress;
	uint32 mc_nSize;
};

typedef struct MemHeader
{
	MemChunk_s *mh_psFirst;
	uint32 mh_nTotalSize;
} MemHeader_s;

typedef struct MemAreaOps MemAreaOps_s;

typedef struct
{
	uint32 _pte;
} pte_t;
typedef struct
{
	uint32 _pgd;
} pgd_t;


extern Page_s *g_psFirstPage;
extern Page_s *g_psFirstFreePage;
extern sem_id g_hAreaTableSema;


#define	PTE_VALUE( pte )	((pte)._pte)
#define	PGD_VALUE( pgd )	((pgd)._pgd)

#define	PTE_PAGE( pte )	(PTE_VALUE(pte) & PAGE_MASK)

/*#define	MK_PTE( uint32 page, uint32 prot )	((pte_t)((page) | MKPROT(prot) ))*/
#define	MK_PGDIR( addr )		((addr) | PTE_PRESENT | PTE_WRITE | PTE_USER )
#define	PGD_TABLE( pgd ) 	((pte_t*)((PGD_VALUE( pgd )) & PAGE_MASK))
#define	PGD_PAGE( pgd ) 	((PGD_VALUE( pgd )) & PAGE_MASK)

#define	PTE_ISPRESENT( pte )	(PTE_VALUE(pte) & PTE_PRESENT)
#define	PTE_ISWRITE( pte )	(PTE_VALUE(pte) & PTE_WRITE)
#define	PTE_ISUSER( pte )	(PTE_VALUE(pte) & PTE_USER)
#define	PTE_ISACCESSED( pte )	(PTE_VALUE(pte) & PTE_ACCESSED)
#define	PTE_ISDIRTY( pte )	(PTE_VALUE(pte) & PTE_DIRTY)

static const size_t PTRS_PER_PTE = 1024;

// Bit patterns for the p_nFlags member of Page_s
static const uint32_t PF_BUSY	= 0x0001;

struct _Page
{
	Page_s *p_psNext;
	Page_s *p_psPrev;
	int p_nAge;
	atomic_t p_nCount;
	uint32_t p_nFlags;
	int p_nPageNum;
	WaitQueue_s *p_psIOThreads;	// Threads waiting for this page to be loaded.
};


void init_pages( uint32 nFirstUsablePage );
int32 get_free_page( int nFlags );
uint32 get_free_pages( int nPageCount, int nFlags );
void free_pages( uint32 nPages, int nCount );
void do_free_pages( uint32 nPages, int nCount );	// doesn't call flush_tlb_global()

Page_s *get_page_desc( int nPageNum );

int shrink_caches( int nBytesNeeded );

void init_memory_pools( char* pRealMemBase, MultiBootHeader_s* psHeader );

#ifdef __cplusplus
}
#endif

#endif /*       _EXEC_MMAN_H_ */
