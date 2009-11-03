/*
 *  The Syllable kernel
 *  AGP busmanager
 *
 *  Contains some implementation ideas from Free|Net|OpenBSD
 *
 *  Copyright (C) 2008 Dee Sharpe
 *
 *	Two licenses are covered in this code.  All portions originating from
 *	BSD sources are covered by the BSD LICENSE.  Everything else is covered
 *	by the GPL v2 LICENSE.  Both licenses are included for your viewing pleasure.
 *	in no order of importance.
 *//*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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
 *//*
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __F_KERNEL_AGPGART_H_
#define __F_KERNEL_AGPGART_H_

#include <kernel/pci.h>
#include <kernel/device.h>
#include <kernel/areas.h>
#include <kernel/semaphore.h>
#include <kernel/dlist.h>
#include <kernel/kdebug.h>

#ifdef __ENABLE_DEBUG__
#define DEBUG_LIMIT	KERN_DEBUG
#endif

typedef enum
{
	AGP_ACQUIRE_FREE = 0,
	AGP_ACQUIRE_USER,
	AGP_ACQUIRE_KERNEL
} AGP_Acquire_State_e;

typedef struct
{
	uint16 nMajor;
	uint16 nMinor;
} AGP_Version_s;

typedef DLIST_HEAD(, _AGP_Memory_s) AGP_Memory_List_s;
typedef struct _AGP_Memory_s
{
	DLIST_ENTRY(_AGP_Memory_s)	psLink;	/* Memory doublely linked list */
	uint32		nId;			/* Memory block ID */
	uint32		nType;			/* Chipset specific memory type */
	uint32		nSize;			/* Total size of this memory block */
	uint32		nBound;			/* Is this memory bound? */
	uint32		nOffset;		/* Offset within GATT */
	uintptr_t	nVirtual;		/* Virtual address of memory */
	uint32		nPhysical;		/* Physical address of memory (bogus hack for i810)*/
	uint32		nAreaCount;		/* Number of areas contained in this memory block */
	char		**apzNames;		/* Array of area names */
	size_t		*panOffsets;	/* Array of offsets from beginning of nAddress */
	size_t		*panSizes;		/* Array of sizes for each area */
	area_id		*panAreas;		/* Array of area IDs for each area */
} AGP_Memory_s;

typedef struct
{
	uint32		nSize;			/* size in bytes */
	uint32		nPhysical;		/* bogus hack for i810 */
	uint32		nOffset;		/* page offset if bound */
	uint32		nBound;			/* non-zero if bound */
} AGP_Memory_Info_s;

typedef struct
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
} AGP_Device_s;

typedef struct
{
	PCI_Info_s sInfo;
	char zName[255];
} AGP_Node_s;

typedef struct
{
	int (*get_aperture)(void);
	int (*set_aperture)(uint32);
	int (*bind_page)(uint32, uint32);
	int (*unbind_page)(uint32);
	void (*flush_tlb)(void);
	int (*enable)(uint32 mode);
	AGP_Memory_s *(*alloc_memory)(int, uint32);
	int (*free_memory)(AGP_Memory_s *);
	int (*bind_memory)(AGP_Memory_s *, uint32);
	int (*unbind_memory)(AGP_Memory_s *);
} AGP_Methods_s;

#define AGP_GET_APERTURE(br)			((br)->psMethods->get_aperture())
#define AGP_SET_APERTURE(br, ap)		((br)->psMethods->set_aperture(ap))
#define AGP_BIND_PAGE(br, off, phy)		((br)->psMethods->bind_page(off, phy))
#define AGP_UNBIND_PAGE(br, off)		((br)->psMethods->unbind_page(off))
#define AGP_FLUSH_TLB(br)				((br)->psMethods->flush_tlb())
#define AGP_ENABLE(br, m)				((br)->psMethods->enable(m))
#define AGP_ALLOC_MEMORY(br, t, s)		((br)->psMethods->alloc_memory(t, s))
#define AGP_FREE_MEMORY(br, m)			((br)->psMethods->free_memory(m))
#define AGP_BIND_MEMORY(br, m, off)		((br)->psMethods->bind_memory(m, off))
#define AGP_UNBIND_MEMORY(br, m)		((br)->psMethods->unbind_memory(m))

typedef struct
{
	PCI_Entry_s 	*psDev;		/* Bridge's pci entry */
	AGP_Node_s		*psNode;	/* Bridge's info node */
	int				nDeviceID;	/* Bridge's device ID */
	AGP_Version_s	sVersion;	/* AGP version supported */
	AGP_Acquire_State_e	eState;	/* Has this bridge been acquired? */
	AGP_Memory_List_s	sMemory;	/* Allocated memory list */
	AGP_Methods_s	*psMethods;	/* Methods of I/O for bridge */
	void			*pChipCtx;	/* Northbridge AGP context */ 
	uint32			nMaxMem;	/* Upper limit of allocatable memory */
	uint32			nAllocated;	/* Amount of allocated memory */
	int				nOpened;	/* Has this device been opened? */
	sem_id			nLock;		/* Mutex for GATT access */
	uint32			nNextId;	/* Next available ID for memory block */
	uint32			*pAperture;	/* AGP aperture memory */
	uint32			nAGPReg;	/* Capability Register */
	uintptr_t		nApBase;	/* Aperture base address register */
	uintptr_t		nApSize;	/* Aperture size */
	uint32			nApAreaId;	/* Area ID for aperture */
	uint32			nAttached;	/* 1 - bridge is attached to agp busmanager, 0 - otherwise */
} AGP_Bridge_s;

typedef struct
{
	uint32		nEntries;		/* Number of entries */
	uint32		*pVirtual;		/* GATT table of addresses */
	uint32		nPhysical;		/* Physical address of GATT */
	area_id		nId;			/* Area ID for this GATT */
} AGP_Gatt_s;


/* Prototypes for generic methods */
int	generic_enable(uint32);
AGP_Memory_s *generic_alloc_memory(int, size_t);
int	generic_free_memory(AGP_Memory_s *);
int	generic_bind_memory(AGP_Memory_s *, uint32);
int	generic_unbind_memory(AGP_Memory_s *);

#endif /* __F_KERNEL_AGPGART_H_ */

