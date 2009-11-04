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

#ifndef __F_ATHEOS_AREA_H__
#define __F_ATHEOS_AREA_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* Locking */
#define AREA_NO_LOCK	0
#define AREA_LAZY_LOCK	1
#define AREA_FULL_LOCK	2
#define AREA_CONTIGUOUS	3

typedef enum {
    LOCK_AREA_READ,
    LOCK_AREA_WRITE
} area_lock_mode;

/* Protection */
#define	AREA_NONE			0x00000000
#define	AREA_READ			0x00000001
#define	AREA_WRITE			0x00000002
#define	AREA_EXEC			0x00000004
#define	AREA_FULL_ACCESS	(AREA_READ | AREA_WRITE | AREA_EXEC)
#define AREA_KERNEL			0x00000008
#define AREA_WRCOMB			0x00000010

/* Creation/cloning allocation policy */
#define AREA_ANY_ADDRESS	0x00000000
#define AREA_EXACT_ADDRESS	0x00000100
#define AREA_BASE_ADDRESS	0x00000200
#define AREA_CLONE_ADDRESS	0x00000300
#define AREA_ADDR_SPEC_MASK	0x00000f00
#define AREA_TOP_DOWN		0x00001000

/* Intel x86 has 4K pages */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/* Areas API */
#ifndef __KERNEL__
area_id	create_area( const char* pzName, void** ppAddress, size_t nSize, flags_t nProtection, flags_t nLockMode );
#endif
status_t delete_area( area_id hArea );
status_t remap_area( area_id nArea, void* pPhysAddress );
area_id  clone_area( const char* pzName, void** ppAddress, flags_t nProtection, flags_t nLockMode, area_id hSrcArea );
status_t get_area_info( area_id hArea, AreaInfo_s* psInfo );

#ifdef __cplusplus
}
#endif

#endif	/* __F_ATHEOS_AREA_H__ */
