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

#ifndef __ATHEOS_AREA_H__
#define __ATHEOS_AREA_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

  /* Flags given to kmalloc() */
#define MEMF_REAL   		0x00000002 /* memory located below 1M						*/
#define	MEMF_USER		0x00000004 /* memory allocated for user-space areas				*/
#define	MEMF_BUFFER		0x00000008 /* memory for disk-cache buffers					*/
#define	MEMF_KERNEL		0x00000010 /* Kernel memory, must be supervisor					*/
#define MEMF_OKTOFAILHACK	0x00000020 /* Ugly hack to let new fail-safe code avoid block forever when out of mem */
#define	MEMF_PRI_MASK		0x000000ff
#define	MEMF_NOBLOCK		0x00000100 /* make kmalloc fail rather than wait for pages to be swapped out	*/
#define MEMF_CLEAR   		0x00010000 /* AllocMem: NULL out area before return				*/
#define	MEMF_LOCKED		0x10000000 /* don't allow the memory to be swapped away				*/


#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))


#define PGDIR_SHIFT	22
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))


/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)
#define PAGE_NR(addr)		(((unsigned long)(addr)) >> PAGE_SHIFT)


typedef unsigned long iaddr_t;


typedef enum {
    LOCK_AREA_READ,
    LOCK_AREA_WRITE
} area_lock_mode;

typedef struct
{
    char    zName[64];
    area_id hAreaID;
    size_t  nSize;
    int	    nLock;
    uint32  nProtection;
    proc_id hProcess;
    uint32  nAllocSize;
    void*   pAddress;
} AreaInfo_s;

  // Locking
#define AREA_NO_LOCK	0
#define AREA_LAZY_LOCK	1
#define AREA_FULL_LOCK	2
#define AREA_CONTIGUOUS	3

  // Protection
#define	AREA_READ		0x00000001
#define	AREA_WRITE		0x00000002
#define	AREA_EXEC		0x00000004
#define	AREA_FULL_ACCESS	(AREA_READ | AREA_WRITE | AREA_EXEC)
#define AREA_KERNEL		0x00000008
#define AREA_UNMAP_PHYS		0x00000010

  // Creation/cloning allocation policy
#define AREA_ANY_ADDRESS	0x00000000
#define AREA_EXACT_ADDRESS	0x00000100
#define AREA_BASE_ADDRESS	0x00000200
#define AREA_CLONE_ADDRESS	0x00000300
#define AREA_ADDR_SPEC_MASK	0x00000f00
#define AREA_TOP_DOWN		0x00001000


#ifdef	__KERNEL__
#define	AREA_REMAPPED	0x0020
#define	AREA_SHARED	0x0040
#define	AREA_GROWSDOWN	0x0080
#endif


#define	AREA_FIRST_KERNEL_ADDRESS	0x00100000
#define	AREA_LAST_KERNEL_ADDRESS	0x7fffffff
#define	AREA_FIRST_USER_ADDRESS		0x80000000
#define AREA_LAST_USER_ADDRESS		0xffffffff

/*
 * Functions that can be used outside the area-handling code iteslf
 */

#ifdef __KERNEL__
area_id	create_area( const char* pzName, void** ppAddress, size_t nSize, size_t nMaxSize, uint32 nProtection, int nLock );
#else
area_id	create_area( const char* pzName, void** ppAddress, size_t nSize, uint32 nProtection, int nLock );
#endif

int alloc_area_list( uint32 nProtection, uint32 nLockMode, iaddr_t nAddress, uint32 nCount,
		     const char* const * apzNames, uint32* panOffsets, uint32* panSizes, area_id* panAreas );
status_t resize_area( area_id hArea, uint32 nNewSize, bool bAtomic );
status_t remap_area( area_id nArea, void* pPhysAddress );

status_t  delete_area( area_id hArea );
area_id	  clone_area( const char* pzName, void** ppAddress, uint32 nProtection, int nLockMode, area_id hSrcArea );

status_t get_area_info( area_id hArea, AreaInfo_s* psInfo );
void*	 get_area_address( area_id nArea );
area_id  get_next_area( const area_id hArea );

int lock_mem_area( const void* pAddress, uint32 nSize, bool bWriteAccess );
int unlock_mem_area( const void* pAddress, uint32 nSize );

int memcpy_to_user( void* pDst, const void* pSrc, int nSize );
int memcpy_from_user( void* pDst, const void* pSrc, int nSize );
int strncpy_from_user( char* pzDst, const char* pzSrc, int nLen );
int strcpy_to_user( char* pzDst, const char* pzSrc );
int strlen_from_user( const char* pzstring );
int strndup_from_user( const char* pzSrc, int nMaxLen, char** ppzDst );



#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_AREA_H__ */
