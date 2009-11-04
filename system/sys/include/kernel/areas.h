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

#ifndef __F_KERNEL_AREA_H__
#define __F_KERNEL_AREA_H__

#include <kernel/types.h>
#include <syllable/areas.h>

/* Flags given to kmalloc() */
#if 0
#define MEMF_REAL			0x00000002		/* memory located below 1M */
#define MEMF_USER			0x00000004		/* memory allocated for user-space */
#define MEMF_BUFFER			0x00000008		/* memory for disk-cache buffers */
#endif
#define MEMF_KERNEL			0x00000010		/* Kernel memory, must be supervisor */
#define MEMF_OKTOFAIL		0x00000020		/* Do not block forever when running out of memory */
#define MEMF_OKTOFAILHACK   MEMF_OKTOFAIL

#define MEMF_PRI_MASK	    0x000000ff
#define MEMF_NOBLOCK		0x00000100		/* make kmalloc fail rather than wait for pages to be swapped out */
#define MEMF_CLEAR			0x00010000		/* kmalloc: return a zero filled area */
#define MEMF_LOCKED			0x10000000		/* memory is non-pageable */

/* Convert to/from addresses/pages */
#define atop(x)		(x >> PAGE_SHIFT)
#define ptoa(x)		(x << PAGE_SHIFT)

#define PAGE_NR(addr)		(((unsigned long)(addr)) >> PAGE_SHIFT)

#define	AREA_REMAPPED	0x0020
#define	AREA_SHARED		0x0040
#define	AREA_GROWSDOWN	0x0080

area_id create_area( const char* pzName, void** ppAddress, size_t nSize, size_t nMaxSize, flags_t nProtection, flags_t nLockMode );
area_id sys_create_area( const char* pzName, void** ppAddress, size_t nSize, flags_t nProtection, flags_t nLockMode );

status_t alloc_area_list( flags_t nProtection, flags_t nLockMode, uintptr_t nAddress, count_t nCount, const char *const *apzNames, size_t* panOffsets, size_t* panSizes, area_id* panAreas );

status_t resize_area( area_id hArea, size_t nNewSize, bool bAtomic );

void*	 get_area_address( area_id nArea );
area_id	 get_next_area( area_id hArea );
status_t get_area_physical_address( area_id hArea, uintptr_t* pnAddress );

status_t verify_mem_area( const void* pAddress, size_t nSize, bool bWriteAccess );

status_t memcpy_to_user( void* pDst, const void* pSrc, size_t nSize );
status_t memcpy_from_user( void* pDst, const void* pSrc, size_t nSize );
status_t strncpy_from_user( char* pzDst, const char* pzSrc, size_t nLen );
status_t strcpy_to_user( char* pzDst, const char* pzSrc );
status_t strlen_from_user( const char* pzstring );
status_t strndup_from_user( const char* pzSrc, size_t nMaxLen, char** ppzDst );

#endif	/* __F_KERNEL_AREA_H__ */
