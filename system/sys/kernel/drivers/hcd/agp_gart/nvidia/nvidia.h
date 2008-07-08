/*-
 * Dee Sharpe 2008 - demetrioussharpe@netscape.net
 *
 * Copyright (c) 2000 Doug Rabson
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
 *
 *	$FreeBSD: src/sys/pci/agpreg.h,v 1.19 2007/07/13 16:28:12 anholt Exp $
 */

#ifndef _SYLLABLE_AGPGART_NVIDIA_H_
#define _SYLLABLE_AGPGART_NVIDIA_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <atheos/pci.h>
#include <atheos/agpgart.h>

/*
 * NVIDIA nForce/nForce2 registers
 */
#define	AGP_NVIDIA_0_APBASE		0x10
#define	AGP_NVIDIA_0_APSIZE		0x80
#define	AGP_NVIDIA_1_WBC		0xf0
#define	AGP_NVIDIA_2_GARTCTRL		0xd0
#define	AGP_NVIDIA_2_APBASE		0xd8
#define	AGP_NVIDIA_2_APLIMIT		0xdc
#define	AGP_NVIDIA_2_ATTBASE(i)		(0xe0 + (i) * 4)
#define	AGP_NVIDIA_3_APBASE		0x50
#define	AGP_NVIDIA_3_APLIMIT		0x54

/*
 * NVIDIA nForce3 registers
 */
#define AGP_AMD64_NVIDIA_0_APBASE	0x10
#define AGP_AMD64_NVIDIA_1_APBASE1	0x50
#define AGP_AMD64_NVIDIA_1_APLIMIT1	0x54
#define AGP_AMD64_NVIDIA_1_APSIZE	0xa8
#define AGP_AMD64_NVIDIA_1_APBASE2	0xd8
#define AGP_AMD64_NVIDIA_1_APLIMIT2	0xdc

#define	SYSCFG				0xC0010010
#define	IORR_BASE0			0xC0010016
#define	IORR_MASK0			0xC0010017
#define	AMD_K7_NUM_IORR		2

typedef struct
{
	uint32			nInitAp;		/* aperture size at startup */
	AGP_Gatt_s		*psGatt;

	PCI_Entry_s		*psADev;		/* AGP Controller */
	PCI_Entry_s		*psMc1Dev;		/* Memory Controller 1 */
	PCI_Entry_s		*psMc2Dev;		/* Memory Controller 2 */
	PCI_Entry_s		*psBDev;		/* Bridge */

	uint32			nWbcMask;
	int				nNumDirs;
	int				nNumActiveEntries;
	int				nPgOffset;
} AGP_Bridge_nVidia_s;

#ifdef __cplusplus
}
#endif
#endif /* _SYLLABLE_AGPGART_NVIDIA_H_ */



