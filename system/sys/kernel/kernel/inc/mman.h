
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
#include <atheos/typedefs.h>
#include <atheos/filesystem.h>

#include "array.h"

extern MultiArray_s g_sAreas;

#define AREA_MUTEX_COUNT	100000	/* Maximum simultanous read-only area table locks */



typedef struct MemChunk MemChunk_s;

struct MemChunk
{
	MemChunk_s *mc_Next;
	uint32 mc_Bytes;
};

typedef struct MemHeader
{
	MemChunk_s *mh_First;
	void *mh_Lower;
	void *mh_Upper;
	uint32 mh_Free;
} MemHeader_s;




typedef struct _MemContext MemContext_s;
typedef struct Page Page_s;
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

#define	PTRS_PER_PTE	1024
#define	GFP_CLEAR	0x0001

// Bit patterns for the p_nFlags member of Page_s
#define PF_BUSY 0x0001

struct Page
{
	Page_s *p_psNext;
	Page_s *p_psPrev;
	int p_nAge;
	atomic_t p_nCount;
	uint32 p_nFlags;
	int p_nPageNum;
	WaitQueue_s *p_psIOThreads;	// Threads waiting for this page to be loaded.
};


int32 get_free_page( int nFlags );
uint32 get_free_pages( int nPageCount, int nFlags );
void free_pages( uint32 nPages, int nCount );

int clone_page_pte( pte_t * pDst, pte_t * pSrc, bool bCow );


Page_s *get_page_desc( int nPageNum );

void list_areas( MemContext_s *psSeg );

int shrink_caches( int nBytesNeeded );

void init_swapper();
void dup_swap_page( int nPage );
void free_swap_page( int nPage );
int swap_in( pte_t * pPte );


#ifdef __cplusplus
}
#endif

#endif /*       _EXEC_MMAN_H_ */
