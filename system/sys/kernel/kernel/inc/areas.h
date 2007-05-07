
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

#ifndef __AREAS_H__
#define __AREAS_H__

#include <atheos/types.h>
#include <atheos/semaphore.h>

#include "../vfs/vfs.h"
#include "mman.h"
#include "array.h"
#include "intel.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif


extern MultiArray_s g_sAreas;

static const size_t AREA_MUTEX_COUNT = 100000;	/* Maximum simultanous read-only area table locks */


struct _MemContext
{
	sem_id mc_hSema;
	pgd_t *mc_pPageDir;
	MemContext_s *mc_psNext;
	Process_s *mc_psOwner;

	MemArea_s **mc_apsAreas;
	int mc_nAreaCount;
	int mc_nMaxAreaCount;
	bool mc_bBusy;		/* true while fork() is cloning the segment */
};

struct _MemArea
{
	char a_zName[OS_NAME_LENGTH];
	MemArea_s *a_psNext;
	MemArea_s *a_psPrev;
	MemArea_s *a_psNextShared;
	MemArea_s *a_psPrevShared;
	uintptr_t a_nStart;
	uintptr_t a_nEnd;
	uintptr_t a_nMaxEnd;
	File_s *a_psFile;
	off_t a_nFileOffset;
	size_t a_nFileLength;
	MemContext_s *a_psContext;
	MemAreaOps_s *a_psOps;

	area_id a_nAreaID;
	uint32_t a_nProtection;
	int a_nLockMode;
	atomic_t a_nRefCount;
	WaitQueue_s *a_psIOThreads;	/* Threads waiting for all IO to . */
	atomic_t a_nIOPages;		/* Number of pages currently being loaded/swapped in or out. */
	bool a_bDeleted;	/* True while waiting for pending IO on a deleted area */
};


static inline int lock_area( MemArea_s *psArea, area_lock_mode eMode )
{

/*    
    if ( eMode == LOCK_AREA_READ ) {
	return( lock_semaphore_ex( psArea->a_hMutex1, 1, SEM_NOSIG, INFINITE_TIMEOUT ) );
    } else {
	return( lock_semaphore_ex( psArea->a_hMutex1, AREA_MUTEX_COUNT, SEM_NOSIG, INFINITE_TIMEOUT ) );
    }
    */
	return ( 0 );
}

static inline int unlock_area( MemArea_s *psArea, area_lock_mode eMode )
{

/*    
    if ( eMode == LOCK_AREA_READ ) {
	return( unlock_semaphore_ex( psArea->a_hMutex1, 1 ) );
    } else {
	return( unlock_semaphore_ex( psArea->a_hMutex1, AREA_MUTEX_COUNT ) );
    }
    */
	return ( 0 );
}

struct MemAreaOps
{
	void ( *open ) ( MemArea_s *area );
	void ( *close ) ( MemArea_s *area );
	void ( *unmap ) ( MemArea_s *area, uint32_t, size_t );
	void ( *protect ) ( MemArea_s *area, uint32_t, size_t, unsigned int newprot );
	int ( *sync ) ( MemArea_s *area, uint32_t, size_t, unsigned int flags );
	void ( *advise ) ( MemArea_s *area, uint32_t, size_t, unsigned int advise );
	uint32_t ( *nopage ) ( MemArea_s *area, uintptr_t address, bool bWriteAccess );
	uint32_t ( *wppage ) ( MemArea_s *area, uintptr_t address, uint32 page );
	int ( *swapout ) ( MemArea_s *, uint32_t, pte_t * );
	  pte_t( *swapin ) ( MemArea_s *, uintptr_t nAddress, uint32 nPage );
};


/*
 * Private functions for the memory managment system
 */

void init_areas( void );

void init_kernel_mem_context( void );
MemContext_s *clone_mem_context( MemContext_s *psOrig );
void empty_mem_context( MemContext_s *psCtx );
void delete_mem_context( MemContext_s *psCtx );

area_id clone_from_inactive_ctx( MemArea_s *psArea, uintptr_t nOffset, size_t nLength );
status_t update_inactive_ctx( MemArea_s *psOriginalArea, area_id hCloneArea, uintptr_t nOffset );


int load_area_page( MemArea_s *psArea, uintptr_t nAddress, bool bWriteAccess );
int handle_copy_on_write( MemArea_s *psArea, pte_t * pPte, uintptr_t nVirtualAddress );
uint32_t memmap_no_page( MemArea_s *psArea, uintptr_t nAddress, bool bWriteAccess );
int insert_area( MemContext_s *psCtx, MemArea_s *psArea );
void remove_area( MemContext_s *psCtx, MemArea_s *psArea );
inline int find_area( MemContext_s *psCtx, uintptr_t nAddress );

uint32_t find_unmapped_area( MemContext_s *psCtx, int nAllocMode, uint32_t nSize, uintptr_t nAddress );

int unmap_area( MemContext_s *psSegment, MemArea_s *psArea );

inline MemArea_s *get_area( MemContext_s *psCtx, uintptr_t nAddress );
MemArea_s *get_area_from_handle( area_id hArea );
MemArea_s *verify_area( const void *pAddr, uint32_t nSize, bool bWrite );
int put_area( MemArea_s *psArea );

inline int clone_page_pte( pte_t * pDst, pte_t * pSrc, bool bCow );
void list_areas( MemContext_s *psSeg );


int map_area_to_file( area_id hArea, File_s *psFile, uint32_t nProt, off_t nOffset, size_t nLength );


extern inline pte_t *pte_offset( pgd_t * dir, uintptr_t address )
{
	return ( ( pte_t * ) PGD_PAGE( *dir ) + ( ( address >> PAGE_SHIFT ) & ( PTRS_PER_PTE - 1 ) ) );
}

extern inline pgd_t *pgd_offset( MemContext_s *mm, uintptr_t address )
{
	return ( mm->mc_pPageDir + ( address >> PGDIR_SHIFT ) );
}

extern inline void set_pte_address( pte_t * psPte, uintptr_t nAddress )
{
	PTE_VALUE( *psPte ) = ( PTE_VALUE( *psPte ) & ~PAGE_MASK ) | nAddress;
}


extern MemContext_s *g_psKernelSeg;

#ifdef __cplusplus
}
#endif

#endif /* __AREAS_H__ */
