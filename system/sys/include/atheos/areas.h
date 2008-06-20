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
//#define MEMF_REAL   	    0x00000002 ///< memory located below 1M
//#define MEMF_USER	    0x00000004 ///< memory allocated for user-space
//#define MEMF_BUFFER	    0x00000008 ///< memory for disk-cache buffers
#define	MEMF_KERNEL	    0x00000010 ///< Kernel memory, must be supervisor
#define MEMF_OKTOFAIL		0x00000020 ///< Do not block forever when running out of memory
#define MEMF_OKTOFAILHACK   0x00000020 ///< For compatibility

//#define MEMF_PRI_MASK	    0x000000ff
#define	MEMF_NOBLOCK	    0x00000100 ///< make kmalloc fail rather than wait
				       ///< for pages to be swapped out
#define MEMF_CLEAR   	    0x00010000 ///< kmalloc: return a zero filled area
#define	MEMF_LOCKED	    0x10000000 ///< memory is non-pageable


/** Intel x86 has 4K pages */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

/* Convert to/from addresses/pages */
#define atop(x)		(x >> PAGE_SHIFT)
#define ptoa(x)		(x << PAGE_SHIFT)

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)
#define PAGE_NR(addr)		(((unsigned long)(addr)) >> PAGE_SHIFT)

//typedef unsigned long iaddr_t;	// changed to C99 type uintptr_t


typedef enum {
    LOCK_AREA_READ,
    LOCK_AREA_WRITE
} area_lock_mode;

typedef struct
{
    char    zName[OS_NAME_LENGTH];
    area_id hAreaID;
    size_t  nSize;
    int	    nLock;
    uint32_t  nProtection;
    proc_id hProcess;
    uint32_t  nAllocSize;
    void*   pAddress;
} AreaInfo_s;

  // Locking
#define AREA_NO_LOCK	0
#define AREA_LAZY_LOCK	1
#define AREA_FULL_LOCK	2
#define AREA_CONTIGUOUS	3

  // Protection
#define	AREA_NONE		0x00000000
#define	AREA_READ		0x00000001
#define	AREA_WRITE		0x00000002
#define	AREA_EXEC		0x00000004
#define	AREA_FULL_ACCESS	(AREA_READ | AREA_WRITE | AREA_EXEC)
#define AREA_KERNEL		0x00000008
#define AREA_WRCOMB		0x00000010

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

//  Moved to <atheos/tunables.h>
//#define	AREA_FIRST_KERNEL_ADDRESS	0x00100000
//#define	AREA_LAST_KERNEL_ADDRESS	0x7fffffff
//#define	AREA_FIRST_USER_ADDRESS		0x80000000
//#define	AREA_LAST_USER_ADDRESS		0xffffffff

/*
 * Functions that can be used outside the area-handling code iteslf
 */

#ifdef __KERNEL__
area_id	create_area( const char* pzName, void** ppAddress, size_t nSize,
		     size_t nMaxSize, flags_t nProtection, flags_t nLockMode );
area_id	sys_create_area( const char* pzName, void** ppAddress, size_t nSize,
			 flags_t nProtection, flags_t nLockMode );
#else
area_id	create_area( const char* pzName, void** ppAddress, size_t nSize,
		     flags_t nProtection, flags_t nLockMode );
#endif

__SYSCALL( status_t, delete_area( area_id hArea ) );
__SYSCALL( status_t, remap_area( area_id nArea, void* pPhysAddress ) );
__SYSCALL( area_id,  clone_area( const char* pzName, void** ppAddress, flags_t nProtection, flags_t nLockMode, area_id hSrcArea ) );
__SYSCALL( status_t, get_area_info( area_id hArea, AreaInfo_s* psInfo ) );

status_t alloc_area_list( flags_t nProtection, flags_t nLockMode,
			  uintptr_t nAddress, count_t nCount,
			  const char *const *apzNames, size_t* panOffsets,
			  size_t* panSizes, area_id* panAreas );
status_t resize_area( area_id hArea, size_t nNewSize, bool bAtomic );

void*	 get_area_address( area_id nArea );
area_id  get_next_area( area_id hArea );
status_t get_area_physical_address( area_id hArea, uintptr_t* pnAddress );

status_t verify_mem_area( const void* pAddress, size_t nSize, bool bWriteAccess );

status_t memcpy_to_user( void* pDst, const void* pSrc, size_t nSize );
status_t memcpy_from_user( void* pDst, const void* pSrc, size_t nSize );
status_t strncpy_from_user( char* pzDst, const char* pzSrc, size_t nLen );
status_t strcpy_to_user( char* pzDst, const char* pzSrc );
status_t strlen_from_user( const char* pzstring );
status_t strndup_from_user( const char* pzSrc, size_t nMaxLen, char** ppzDst );



#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_AREA_H__ */
