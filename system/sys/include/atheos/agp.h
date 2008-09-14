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

#ifndef _SYLLABLE_AGP_H_
#define _SYLLABLE_AGP_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/agpgart.h>

/*
 * The AGP gatt uses 4k pages irrespective of the host page size.
 */
#define AGP_PAGE_SIZE		4096
#define AGP_PAGE_SHIFT		12

/*
 * Macros to manipulate AGP mode words.
 *
 * SBA = Sideband Address Port
 * FW  = Fast Writes
 */
#define AGP_MODE_GET_RQ(x)			(((x) & 0xff000000U) >> 24)
#define AGP_MODE_GET_ARQSZ(x)		(((x) & 0x0000e000U) >> 13)
#define AGP_MODE_GET_CAL(x)			(((x) & 0x00001c00U) >> 10)
#define AGP_MODE_GET_SBA(x)			(((x) & 0x00000200U) >> 9)
#define AGP_MODE_GET_AGP(x)			(((x) & 0x00000100U) >> 8)
#define AGP_MODE_GET_GART_64(x)		(((x) & 0x00000080U) >> 7)
#define AGP_MODE_GET_OVER_4G(x)		(((x) & 0x00000020U) >> 5)
#define AGP_MODE_GET_FW(x)			(((x) & 0x00000010U) >> 4)
#define AGP_MODE_GET_MODE_3(x)		(((x) & 0x00000008U) >> 3)
#define AGP_MODE_GET_RATE(x)		((x) & 0x00000007U)
#define AGP_MODE_SET_RQ(x,v)		(((x) & ~0xff000000U) | ((v) << 24))
#define AGP_MODE_SET_ARQSZ(x,v)		(((x) & ~0x0000e000U) | ((v) << 13))
#define AGP_MODE_SET_CAL(x,v)		(((x) & ~0x00001c00U) | ((v) << 10))
#define AGP_MODE_SET_SBA(x,v)		(((x) & ~0x00000200U) | ((v) << 9))
#define AGP_MODE_SET_AGP(x,v)		(((x) & ~0x00000100U) | ((v) << 8))
#define AGP_MODE_SET_GART_64(x,v)	(((x) & ~0x00000080U) | ((v) << 7))
#define AGP_MODE_SET_OVER_4G(x,v)	(((x) & ~0x00000020U) | ((v) << 5))
#define AGP_MODE_SET_FW(x,v)		(((x) & ~0x00000010U) | ((v) << 4))
#define AGP_MODE_SET_MODE_3(x,v)	(((x) & ~0x00000008U) | ((v) << 3))
#define AGP_MODE_SET_RATE(x,v)		(((x) & ~0x00000007U) | (v))

#define AGP_MODE_V2_RATE_1x			0x00000001
#define AGP_MODE_V2_RATE_2x			0x00000002
#define AGP_MODE_V2_RATE_4x			0x00000004
#define AGP_MODE_V3_RATE_4x			0x00000001
#define AGP_MODE_V3_RATE_8x			0x00000002
#define AGP_MODE_V3_RATE_RSVD		0x00000004

/* XXX: Compat */
#define AGP_MODE_GET_4G(x)		AGP_MODE_GET_OVER_4G(x)
#define AGP_MODE_SET_4G(x)		AGP_MODE_SET_OVER_4G(x)
#define AGP_MODE_RATE_1x		AGP_MODE_V2_RATE_1x
#define AGP_MODE_RATE_2x		AGP_MODE_V2_RATE_2x
#define AGP_MODE_RATE_4x		AGP_MODE_V2_RATE_4x

/* These 2 are for the capabilities register's result of PCI_AGP_VERSION */
#define AGP_VERSION_MAJOR(x)	(((x) >> 20) & 0xf)
#define AGP_VERSION_MINOR(x)	(((x) >> 16) & 0xf)

#define AGPIOC_BASE			'A'
#define AGPIOC_INFO			_IOR (AGPIOC_BASE, 0, AGP_Info_s)
#define AGPIOC_ACQUIRE		_IO  (AGPIOC_BASE, 1)
#define AGPIOC_RELEASE		_IO  (AGPIOC_BASE, 2)
#define AGPIOC_SETUP		_IOW (AGPIOC_BASE, 3, AGP_Setup_s)

#if 0
#define AGPIOC_RESERVE		_IOW (AGPIOC_BASE, 4, AGP_Region_s)
#define AGPIOC_PROTECT		_IOW (AGPIOC_BASE, 5, AGP_Region_s)
#endif

#define AGPIOC_ALLOCATE		_IOWR(AGPIOC_BASE, 6, AGP_Allocate_s)
#define AGPIOC_DEALLOCATE	_IOW (AGPIOC_BASE, 7, int)
#define AGPIOC_BIND			_IOW (AGPIOC_BASE, 8, AGP_Bind_s)
#define AGPIOC_UNBIND		_IOW (AGPIOC_BASE, 9, AGP_Unbind_s)
#define AGPIOC_MMAP			_IOR (AGPIOC_BASE, 10, AGP_Mmap_s)

typedef struct
{
	AGP_Version_s sVersion;		/* Version of the driver */
	uint32 nVendor;				/* Bridge vendor */
	uint32 nDevice;				/* Bridge device */
	uint32 nMode;				/* Mode info of bridge */
	uintptr_t nApBase;			/* Base of aperture */
	uintptr_t nApSize;			/* Size of aperture */
	size_t nPgTotal;			/* Max pages (swap + system) */
	size_t nPgSystem;			/* Max pages (system) */
	size_t nPgUsed;				/* Current pages used */
} AGP_Info_s;

typedef struct
{
	uint32 nMode;				/* Mode info of bridge */
} AGP_Setup_s;

#if 0
/*
 * The "prot" down below needs still a "sleep" flag somehow ...
 */
typedef struct
{
	uint32	nPgStart;		/* Starting page to populate */
	size_t	nPgCount;		/* Number of pages */
	int 	nProt;			/* Prot flags for mmap */
} AGP_Segment_s;

typedef struct
{
	proc_id		nProcId;		/* Process ID */
	thread_id	nThID;			/* Thread ID of process */
	size_t 		nSegCount;		/* number of segments */
	AGP_Segment_s *psSegList;
} AGP_Region_s;
#endif

typedef struct
{
	int		nKey;			/* tag of allocation            */
	size_t	nPgCount;		/* number of pages              */
	uint32	nType;			/* 0 == normal, other devspec   */
   	uint32	nPhysical;  	/* device specific (some devices  
				 			 * need a phys address of the
				 			 * actual page behind the gatt
				 			 * table)                       */
} AGP_Allocate_s;

typedef struct
{
	int		nKey;			/* tag of allocation            */
	uint32	nPgStart;		/* starting page to populate    */
} AGP_Bind_s;

typedef struct
{
	int		nKey;			/* tag of allocation            */
	uint32	nPriority;		/* priority for paging out      */
} AGP_Unbind_s;

typedef struct
{
	uint32	nOffset;		/* Offset of area within aperature */
	size_t	nSize;			/* Size of area mapped */
	uintptr_t *pAperture;	/* Gart aperture */
} AGP_Mmap_s;

#define AGP_BUS_NAME "agp"
#define AGP_BUS_VERSION 1
#define	MAX_AGP_DEVICES	255

typedef struct
{
	/* functions directly used by video drivers */
	status_t	(*attach_bridge)(AGP_Bridge_s *);
	status_t	(*remove_bridge)(void);
	void		(*flush_cache)(void);
	AGP_Gatt_s	*(*alloc_gatt)(void);
	void		(*free_gatt)(AGP_Gatt_s *);
	status_t	(*map_aperture)(int);
	PCI_Entry_s	*(*find_display)(void);
	
	/* functions directly usable by kenel video drivers & also callable from userspace via IOCTL */
	AGP_Acquire_State_e (*get_state)(void);
	void		(*get_info)(AGP_Info_s *psInfo);
	int			(*acquire)(void);
	int			(*release)(void);
	int			(*enable)(uint32 nMode);
	AGP_Memory_s *(*alloc_memory)(AGP_Allocate_s *psAlloc);
	void		(*free_memory)(int nId);
	int			(*bind_memory)(AGP_Bind_s *psBind);
	int			(*unbind_memory)(AGP_Unbind_s *psUnbind);
} AGP_bus_s;

void memory_info(void *pHandle, AGP_Memory_Info_s *psMemInfo);

#ifdef __cplusplus
}
#endif
#endif /* _SYLLABLE_AGP_H_ */

