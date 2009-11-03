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

#ifndef _SYLLABLE_AGPGART_SIS_H_
#define _SYLLABLE_AGPGART_SIS_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <kernel/agpgart.h>

/*
 * Config offsets for SiS AGP chipsets.
 */
#define AGP_SIS_ATTBASE		0x90
#define AGP_SIS_WINCTRL		0x94
#define AGP_SIS_TLBCTRL		0x97
#define AGP_SIS_TLBFLUSH	0x98

typedef struct
{
	uint32			nInitAp;	/* aperture size at startup */
	AGP_Gatt_s		*psGatt;
} AGP_Bridge_Sis_s;


#ifdef __cplusplus
}
#endif
#endif /* _SYLLABLE_AGPGART_SIS_H_ */

