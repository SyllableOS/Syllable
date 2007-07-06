/*
 ** Radeon graphics driver for Syllable application server
 *  Copyright (C) 2004 Michael Krueger <invenies@web.de>
 *  Copyright (C) 2003 Arno Klenke <arno_klenke@yahoo.com>
 *  Copyright (C) 1998-2001 Kurt Skauen <kurt@atheos.cx>
 *
 ** This program is free software; you can redistribute it and/or
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
 *
 ** In this driver, code from these sources (GPL or GPL-compatible) was used:
 *
 *> Linux 2.6.2-rc3 Radeon framebuffer driver v0.2.0
 *  Original copyright notice:
 *
 *	drivers/video/radeonfb.c
 *	framebuffer driver for ATI Radeon chipset video boards
 *
 *	Copyright 2003	Ben. Herrenschmidt <benh@kernel.crashing.org>
 *	Copyright 2000	Ani Joshi <ajoshi@kernel.crashing.org>
 *
 *	i2c bits from Luca Tettamanti <kronos@kronoz.cjb.net>
 *	
 *	Special thanks to ATI DevRel team for their hardware donations.
 *
 *	...Insert GPL boilerplate here...
 *
 *	Significant portions of this driver apdated from XFree86 Radeon
 *	driver which has the following copyright notice:
 *
 *	Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                     VA Linux Systems Inc., Fremont, California.
 *
 *	All Rights Reserved.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation on the rights to use, copy, modify, merge,
 *	publish, distribute, sublicense, and/or sell copies of the Software,
 *	and to permit persons to whom the Software is furnished to do so,
 *	subject to the following conditions:
 *
 *	The above copyright notice and this permission notice (including the
 *	next paragraph) shall be included in all copies or substantial
 *	portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * 	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 *	THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 *
 *	XFree86 driver authors:
 *
 *	   Kevin E. Martin <martin@xfree86.org>
 *	   Rickard E. Faith <faith@valinux.com>
 *	   Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 *> Video overlay code from BeOS Radeon Driver v3.2.8
 * Original coyright notice:
 *
 * Copyright (c) 2002, Thomas Kurschel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *> On some bits, the Xfree86 4.3.0.1 driver source was consulted
 * (for copyright notices see above)
 */

#include "radeon.h"

#define CHIP_DEF(id, family, flags)					\
	{ 0x1002, id, (flags) | (CHIP_FAMILY_##family) }

static struct pci_table radeon_pci_table[] = {
#ifdef RADEON_ENABLE_MOBILITY
	/* Mobility M6 */
	CHIP_DEF(PCI_CHIP_RADEON_LY, 	RV100,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RADEON_LZ,	RV100,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif
	/* Radeon VE/7000 */
	CHIP_DEF(PCI_CHIP_RV100_QY, 	RV100,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV100_QZ, 	RV100,	CHIP_HAS_CRTC2),
#ifdef RADEON_ENABLE_MOBILITY
	/* Radeon IGP320M (U1) */
	CHIP_DEF(PCI_CHIP_RS100_4336,	RS100,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* Radeon IGP320 (A3) */
	CHIP_DEF(PCI_CHIP_RS100_4136,	RS100,	CHIP_HAS_CRTC2 | CHIP_IS_IGP), 
	/* IGP330M/340M/350M (U2) */
	CHIP_DEF(PCI_CHIP_RS200_4337,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* IGP330/340/350 (A4) */
	CHIP_DEF(PCI_CHIP_RS200_4137,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	/* Mobility 7000 IGP */
	CHIP_DEF(PCI_CHIP_RS250_4437,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* 7000 IGP (A4+) */
	CHIP_DEF(PCI_CHIP_RS250_4237,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
#endif
	/* 8500 AIW */
	CHIP_DEF(PCI_CHIP_R200_BB,	R200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R200_BC,	R200,	CHIP_HAS_CRTC2),
	/* 8700/8800 */
	CHIP_DEF(PCI_CHIP_R200_QH,	R200,	CHIP_HAS_CRTC2),
	/* 8500 */
	CHIP_DEF(PCI_CHIP_R200_QI,	R200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R200_QJ,	R200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R200_QL,	R200,	CHIP_HAS_CRTC2),
	/* 9100 */
	CHIP_DEF(PCI_CHIP_R200_QM,	R200,	CHIP_HAS_CRTC2),
#ifdef RADEON_ENABLE_MOBILITY
	/* Mobility M7 */
	CHIP_DEF(PCI_CHIP_RADEON_LW,	RV200,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RADEON_LX,	RV200,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif
	/* 7500 */
	CHIP_DEF(PCI_CHIP_RV200_QW,	RV200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV200_QX,	RV200,	CHIP_HAS_CRTC2),
#ifdef RADEON_ENABLE_MOBILITY
	/* Mobility M9 */
	CHIP_DEF(PCI_CHIP_RV250_Ld,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Le,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Lf,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Lg,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif
	/* 9000/Pro */
	CHIP_DEF(PCI_CHIP_RV250_If,	RV250,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV250_Ig,	RV250,	CHIP_HAS_CRTC2),
#ifdef RADEON_ENABLE_MOBILITY
	/* Mobility 9100 IGP (U3) */
	CHIP_DEF(PCI_CHIP_RS300_5835,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* 9100 IGP (A5) */
	CHIP_DEF(PCI_CHIP_RS300_5834,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	/* Mobility 9200 (M9+) */
	CHIP_DEF(PCI_CHIP_RV280_5C61,	RV280,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV280_5C63,	RV280,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif
	/* 9200 */
	CHIP_DEF(PCI_CHIP_RV280_5960,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5961,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5962,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5963,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5964,	RV280,	CHIP_HAS_CRTC2),
#ifdef RADEON_ENABLE_R300
	/* 9500 */
	CHIP_DEF(PCI_CHIP_R300_AD,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_AE,	R300,	CHIP_HAS_CRTC2),
	/* 9600TX / FireGL Z1 */
	CHIP_DEF(PCI_CHIP_R300_AF,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_AG,	R300,	CHIP_HAS_CRTC2),
	/* 9700/9500/Pro/FireGL X1 */
	CHIP_DEF(PCI_CHIP_R300_ND,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NE,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NF,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NG,	R300,	CHIP_HAS_CRTC2),
	/* Mobility M10/M11 */
#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_RV350_NP,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NQ,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NR,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NS,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NT,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NV,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif
	/* 9600/FireGL T2 */
	CHIP_DEF(PCI_CHIP_RV350_AP,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AQ,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AR,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AS,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AT,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AV,	RV350,	CHIP_HAS_CRTC2),
	/* 9800/Pro/FileGL X2 */
	CHIP_DEF(PCI_CHIP_R350_AH,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AI,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AJ,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AK,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NH,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NI,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R360_NJ,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NK,	R350,	CHIP_HAS_CRTC2),
#endif /* RADEON_ENABLE_R300 */

#ifdef RADEON_ENABLE_R400
#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_RV380_3150,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV380_3154,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV370_5460,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV370_5464,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif /* RADEON_ENABLE_MOBILITY */
	CHIP_DEF(PCI_CHIP_RV380_3E50,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV380_3E54,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B60,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B62,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B64,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B65,	RV380,	CHIP_HAS_CRTC2),

#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_RS400_5A42,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RC410_5A62,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RS480_5955,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RS482_5975,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif /* RADEON_ENABLE_MOBILITY */
	CHIP_DEF(PCI_CHIP_RS400_5A41,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RC410_5A61,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RS480_5954,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RS482_5974,	RV380,	CHIP_HAS_CRTC2),

#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_RV410_564A,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV410_564B,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV410_5652,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV410_5653,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif /* RADEON_ENABLE_MOBILITY */
	CHIP_DEF(PCI_CHIP_RV410_5E48,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV410_5E4B,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV410_5E4A,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV410_5E4D,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV410_5E4C,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV410_5E4F,	R420,	CHIP_HAS_CRTC2),

#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_R420_JN,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif /* RADEON_ENABLE_MOBILITY */
	CHIP_DEF(PCI_CHIP_R420_JH,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JI,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JJ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JK,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JL,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JM,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JP,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_4A4F,	R420,	CHIP_HAS_CRTC2),

	CHIP_DEF(PCI_CHIP_R423_UH,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UI,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UJ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UK,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UQ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UR,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UT,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_5D57,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_5550,	R420,	CHIP_HAS_CRTC2),

#ifdef RADEON_ENABLE_MOBILITY
	CHIP_DEF(PCI_CHIP_R430_5D49,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_R430_5D4A,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_R430_5D48,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
#endif /* RADEON_ENABLE_MOBILITY */
	CHIP_DEF(PCI_CHIP_R430_554F,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R430_554D,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R430_554E,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R430_554C,	R420,	CHIP_HAS_CRTC2),

	CHIP_DEF(PCI_CHIP_R480_5D4C,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R480_5D50,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R480_5D4E,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R480_5D4F,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R480_5D52,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R480_5D4D,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R481_4B4B,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R481_4B4A,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R481_4B49,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R481_4B4C,	R420,	CHIP_HAS_CRTC2),
#endif /* RADEON_ENABLE_R400 */

	/* Original Radeon/7200 */
	CHIP_DEF(PCI_CHIP_RADEON_QD,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QE,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QF,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QG,	RADEON,	0),
	{ 0xffff, 0xffff, 0},
};

/*
 * globals
 */

//static char *mode_option;
static char *monitor_layout;
//static int noaccel = 0;
//static int nomodeset = 0;
static int ignore_edid = 1;
//static int mirror = 0;
//static int panel_yres = 0;
//static int force_dfp = 0;
//static int force_measure_pll = 0;

int radeonfb_noaccel = 0;

// *** mode table ***
// based upon fb.modes
// Add your custom video mode here!

VideoMode DefaultVideoModes [] = {
	 // 640x480 & 60Hz
	{ 640, 480, 60, 39722, 48, 16, 33, 10, 96, 2, 0},
	// 640x480 & 75Hz
	{ 640, 480, 75, 33747, 125, 18, 18, 5, 60, 5, 0},
	// 640x480 & 85Hz
	{ 640, 480, 85, 27777, 80, 56, 25, 1, 56, 3, 0},
	// 640x480 & 100Hz
	{ 640, 480, 100, 22272, 48, 32, 17, 22, 128, 12,0},
	// 800x600 & 60Hz
	{ 800, 600, 60, 25000, 88, 40, 23, 1, 128, 4, V_PHSYNC|V_PVSYNC},
	// 800x600 & 75Hz
	{ 800, 600, 75, 20203, 160, 16, 21, 1, 80, 3, V_PHSYNC|V_PVSYNC},
	// 800x600 & 85Hz
	{ 800, 600, 85, 16460, 160, 64, 36, 16, 64, 5, 0},
	// 800x600 & 100Hz
	{ 800, 600, 100, 14815, 160, 32, 20, 4, 80, 6, V_PHSYNC|V_PVSYNC},
	// 1024x768 & 60Hz
	{ 1024, 768, 60, 15385, 160, 24, 29, 3, 136, 6, 0},
	// 1024x768 & 75Hz
	{ 1024, 768, 75, 12699, 176, 16, 28, 1, 96, 3, V_PHSYNC|V_PVSYNC},
	// 1024x768 & 85Hz
	{ 1024, 768, 85, 10111, 192, 32, 34, 14, 160, 6, 0},
	// 1024x768 & 100Hz
	{ 1024, 768, 100, 9091, 280, 0, 16, 0, 88, 8, 0},
	// 1152x864 & 60Hz
	{ 1152, 864, 60, 12500, 128, 64, 41, 6, 112, 5, V_PHSYNC|V_PVSYNC},
	// 1152x864 & 75Hz
	{ 1152, 864, 75, 9091, 144, 24, 85, 45, 144, 8, V_PHSYNC|V_PVSYNC},
	// 1280x1024 & 60Hz
	{ 1280, 1024, 60, 9620, 248, 48, 38, 1, 112, 3, V_PHSYNC|V_PVSYNC},
	// 1280x1024 & 75Hz
	{ 1280, 1024, 75, 7408, 248, 16, 38, 1, 144, 3, V_PHSYNC|V_PVSYNC},
	// 1400x1050 & 60Hz (notebook LCD)
	{ 1400, 1050, 60, 9259, 128, 40, 12, 0, 112, 3, V_PHSYNC|V_PVSYNC},
	// 1400x1050 & 75Hz (notebook LCD)
	{ 1400, 1050, 75, 9271, 120, 56, 13, 0, 112, 3, V_PHSYNC|V_PVSYNC},
	// 1600x1200 & 60Hz
	{ 1600, 1200, 60, 6172, 304, 64, 46, 1, 192, 3, V_PHSYNC|V_PVSYNC},
	// 1600x1200 & 75Hz
	{ 1600, 1200, 75, 4938, 304, 64, 46, 1, 192, 3, V_PHSYNC|V_PVSYNC},

	// mode list terminator
	{   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


/* these common regs are cleared before mode setting so they do not
 * interfere with anything
 */
reg_val common_regs[] = {
	{ OVR_CLR, 0 },	
	{ OVR_WID_LEFT_RIGHT, 0 },
	{ OVR_WID_TOP_BOTTOM, 0 },
	{ OV0_SCALE_CNTL, 0 },
	{ SUBPIC_CNTL, 0 },
	{ VIPH_CONTROL, 0 },
	{ I2C_CNTL_1, 0 },
	{ GEN_INT_CNTL, 0 },
	{ CAP0_TRIG_CNTL, 0 },
	{ CAP1_TRIG_CNTL, 0 }
};

reg_val common_regs_m6[] = {
	{ OVR_CLR,      0 },
	{ OVR_WID_LEFT_RIGHT,   0 },
	{ OVR_WID_TOP_BOTTOM,   0 },
	{ OV0_SCALE_CNTL,   0 },
	{ SUBPIC_CNTL,      0 },
	{ GEN_INT_CNTL,     0 },
	{ CAP0_TRIG_CNTL,   0 } 
};

//=============================================================================
// NAME: ATIRadeon::ProbeHardware
// DESC: probe specified PCI card
// NOTE: returns 1 if a Radeon card was found
// SEE ALSO:
//-----------------------------------------------------------------------------
uint32 ATIRadeon::ProbeHardware ( int nFd, PCI_Info_s * PCIInfo ) {
	int nCard = 0;
	 // check all descriptors
	while( radeon_pci_table[nCard].pci_id != 0xffff ) {
		if( PCIInfo->nVendorID == radeon_pci_table[nCard].vendor_id
				&& PCIInfo->nDeviceID == radeon_pci_table[nCard].pci_id ) {
			m_sRadeonInfo = radeon_pci_table[nCard];
			return 1;
		}
		nCard++;
	}
	return( 0 );
}

//-----------------------------------------------------------------------------
// NAME: ATIRadeon::InitHardware
// DESC: sets up framebuffer & MMIO
// NOTE: returns true if successful
// SEE ALSO:
//-----------------------------------------------------------------------------
bool ATIRadeon::InitHardware( int nFd ) {
	uint32 tmp;
	uint32 orig_fb_base_phys;

	rinfo.pdev = m_cPCIInfo;
	
	strcpy(rinfo.name, "ATi Radeon XX\0");
	rinfo.name[11] = m_sRadeonInfo.pci_id >> 8;
	rinfo.name[12] = m_sRadeonInfo.pci_id & 0xFF;
	rinfo.family = m_sRadeonInfo.flags & CHIP_FAMILY_MASK;
	rinfo.chipset = rinfo.pdev.nDeviceID;
	rinfo.has_CRTC2 = (m_sRadeonInfo.flags & CHIP_HAS_CRTC2) != 0;
	rinfo.is_mobility = (m_sRadeonInfo.flags & CHIP_IS_MOBILITY) != 0;
	rinfo.is_IGP = (m_sRadeonInfo.flags & CHIP_IS_IGP) != 0;

	dbprintf("Radeon :: %s detected.\n", rinfo.name);

	/* read configuration file */
	GetSettings();

	/* sort out what's enabled */
	if( rinfo.family >= CHIP_FAMILY_R300 && !m_bCfgEnableR300 )
		return false;
	else if( ( rinfo.is_mobility || rinfo.is_IGP ) && !m_bCfgEnableMobility )
		return false;
		
	/* Set base addrs */
	orig_fb_base_phys = (m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK);
	rinfo.fb_base_phys = orig_fb_base_phys;
	m_nRegBasePhys = (m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK);
	rinfo.mmio_base_phys = (m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK);

	// allocate register area
	m_pRegisterBase = NULL;
	m_hRegisterArea = create_area ("radeon_register", (void **)&m_pRegisterBase,
		get_pci_memory_size(nFd,&m_cPCIInfo, 2), AREA_FULL_ACCESS, AREA_NO_LOCK);
	if( m_hRegisterArea < 0 ) {
		dbprintf ("Radeon::failed to create register area (%d)\n", m_hRegisterArea);
		return false;
	}

	if( remap_area (m_hRegisterArea, (void *)((m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK))) < 0 ) {
		dbprintf("Radeon :: failed to create register area (%d)\n", m_hRegisterArea);
		return false;
	}
	rinfo.mmio_base = m_pRegisterBase;

	/*
	 * Enable memory-space accesses using config-space
	 * command register.
	 */
	tmp=pci_gfx_read_config(nFd, m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2);
	if( !(tmp & PCI_COMMAND_MEMORY) ) {
		dbprintf("Radeon :: Enabling memory-space access\n");
		tmp |= PCI_COMMAND_MEMORY;
		pci_gfx_write_config(nFd, m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, PCI_COMMAND, 2, tmp);
	}

	dbprintf("Radeon :: Using MMIO at %x\n", (uint)(rinfo.mmio_base_phys));

#if 0
	FixUpMemoryMappings();
#endif

	/* framebuffer size */
	if(rinfo.is_IGP)
	{
		uint32 tom;
		/* force MC_FB_LOCATION to NB_TOM */
		tom = INREG( NB_TOM );
		rinfo.fb_local_base = tom << 16;
		rinfo.video_ram   = ((tom >> 16) - (tom & 0xffff) + 1) << 16;
	} 
	else
	{
		if (rinfo.family >= CHIP_FAMILY_R300) {
			rinfo.fb_local_base = 0;
			rinfo.video_ram   = INREG( CONFIG_MEMSIZE );
		} else {
			rinfo.fb_local_base = INREG( CONFIG_APER_0_BASE ); 
			rinfo.video_ram   = INREG( CONFIG_APER_SIZE );
		}
	}
     
	OUTREG( MC_FB_LOCATION, (rinfo.fb_local_base>>16) |
				  ((rinfo.fb_local_base + rinfo.video_ram - 1) & 0xffff0000) ); 

     OUTREG( DISPLAY_BASE_ADDR, rinfo.fb_local_base );
     OUTREG( DISP_MERGE_CNTL, 0xffff0000 );
     if (rinfo.has_CRTC2) {
          OUTREG( CRTC2_DISPLAY_BASE_ADDR, rinfo.fb_local_base );
          OUTREG( DISP2_MERGE_CNTL, 0xffff0000 );
     }

     OUTREG( FCP_CNTL, FCP0_SRC_GND );
     OUTREG( CAP0_TRIG_CNTL, 0 );
     OUTREG( VID_BUFFER_CONTROL, 0x00010001 );
     OUTREG( DISP_TEST_DEBUG_CNTL, 0 );

	/* ram type */
	tmp = INREG(MEM_SDRAM_MODE_REG);
	switch ((MEM_CFG_TYPE & tmp) >> 30) {
       	case 0:
       		/* SDR SGRAM (2:1) */
       		strcpy(rinfo.ram_type, "SDR SGRAM");
       		rinfo.ram.ml = 4;
       		rinfo.ram.mb = 4;
       		rinfo.ram.trcd = 1;
       		rinfo.ram.trp = 2;
       		rinfo.ram.twr = 1;
       		rinfo.ram.cl = 2;
       		rinfo.ram.loop_latency = 16;
       		rinfo.ram.rloop = 16;
       		break;
       	case 1:
       		/* DDR SGRAM */
       		strcpy(rinfo.ram_type, "DDR SGRAM");
       		rinfo.ram.ml = 4;
       		rinfo.ram.mb = 4;
       		rinfo.ram.trcd = 3;
       		rinfo.ram.trp = 3;
       		rinfo.ram.twr = 2;
       		rinfo.ram.cl = 3;
       		rinfo.ram.tr2w = 1;
       		rinfo.ram.loop_latency = 16;
       		rinfo.ram.rloop = 16;
		break;
       	default:
       		/* 64-bit SDR SGRAM */
       		strcpy(rinfo.ram_type, "SDR SGRAM 64");
       		rinfo.ram.ml = 4;
       		rinfo.ram.mb = 8;
       		rinfo.ram.trcd = 3;
       		rinfo.ram.trp = 3;
       		rinfo.ram.twr = 1;
       		rinfo.ram.cl = 3;
       		rinfo.ram.tr2w = 1;
       		rinfo.ram.loop_latency = 17;
       		rinfo.ram.rloop = 17;
		break;
	}

	if(rinfo.family == CHIP_FAMILY_R300 ||
		rinfo.family == CHIP_FAMILY_R350 ||
		rinfo.family == CHIP_FAMILY_RV350)
	{
		/* force DDR SGRAM */
       strcpy(rinfo.ram_type, "DDR SGRAM");
       rinfo.ram.ml = 4;
       rinfo.ram.mb = 4;
       rinfo.ram.trcd = 3;
       rinfo.ram.trp = 3;
       rinfo.ram.twr = 2;
       rinfo.ram.cl = 3;
       rinfo.ram.tr2w = 1;
       rinfo.ram.loop_latency = 16;
       rinfo.ram.rloop = 16;
	}

	/*
	 * Hack to get around some busted production M6's
	 * reporting no ram
	 */
	if (rinfo.video_ram == 0) {
		switch (rinfo.pdev.nDeviceID) {	
			case PCI_CHIP_RADEON_LY:
			case PCI_CHIP_RADEON_LZ:
				rinfo.video_ram = 8192 * 1024;
				break;
	       	default:
	       		break;
		}
	}

	dbprintf("Radeon :: probed %s %ldk videoram\n", (rinfo.ram_type), (rinfo.video_ram/1024));

	rinfo.video_ram -= 0x40000; /* Reserve space for BIOS ROM workaround -MK */

	// allocate framebuffer area
	m_hFramebufferArea = create_area ("radeon_fb", (void **)&m_pFramebufferBase,
		rinfo.video_ram, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	if( m_hFramebufferArea < 0 ) {
		dbprintf ("Radeon :: failed to create framebuffer area of %d bytes (%d)\n", rinfo.video_ram, m_hFramebufferArea);
		return false;
	}

	if( remap_area (m_hFramebufferArea, (void *)((rinfo.fb_base_phys))) < 0 ) {
		dbprintf("Radeon :: failed to remap framebuffer area at physical addr 0x%x (%d)\n", rinfo.fb_base_phys, m_hFramebufferArea);
		return false;
	}
	rinfo.fb_base = m_pFramebufferBase;

	dbprintf("Radeon :: Using framebuffer at %x\n", (uint)(rinfo.fb_base_phys));

	/*
	 * Check for errata
	 */
	rinfo.errata = 0;
	if (rinfo.family == CHIP_FAMILY_R300 &&
	    (INREG(CONFIG_CNTL) & CFG_ATI_REV_ID_MASK)
	    == CFG_ATI_REV_A11)
		rinfo.errata |= CHIP_ERRATA_R300_CG;

	if (rinfo.family == CHIP_FAMILY_RV200 ||
	    rinfo.family == CHIP_FAMILY_RS200)
		rinfo.errata |= CHIP_ERRATA_PLL_DUMMYREADS;

	if (rinfo.family == CHIP_FAMILY_RV100 ||
	    rinfo.family == CHIP_FAMILY_RS100 ||
	    rinfo.family == CHIP_FAMILY_RS200)
		rinfo.errata |= CHIP_ERRATA_PLL_DELAY;


	if( !m_bCfgDisableBIOSUsage ) {
		/*
		 * Map the BIOS ROM if any and retrieve PLL parameters from
		 * the BIOS.
		 */
		MapROM(nFd, &m_cPCIInfo);

		/*
		 * On x86, the primary display on laptop may have its BIOS
		 * ROM elsewhere, try to locate it at the legacy memory hole.
		 * We probably need to make sure this is the primary display,
		 * but that is difficult without some arch support.
		 */
		if (rinfo.bios_seg == NULL)
			FindMemVBios();
			
	}

	/* Get informations about the board's PLL */
	GetPLLInfo();

	/* Probe screen types */
	ProbeScreens(monitor_layout, ignore_edid);

	/* Create screen mode list */
	CreateModeList();

	/* save current mode regs before we switch into the new one
	 * so we can restore this upon exit
	 */
	SaveState (&rinfo.init_state);


	/* Enable PM on mobility chips */
	if (rinfo.is_mobility) {
		/* Find PM registers in config space */
		/* Enable dynamic PM of chip clocks */
		PMEnableDynamicMode();
		RTRACE("Radeon :: Power Management enabled for Mobility chipsets\n");
	}

	/* We don't need BIOS anymore */
	if(rinfo.bios_seg)
		UnmapROM(nFd, &rinfo.pdev);

	if(rinfo.is_mobility && m_bCfgEnableMirroring)
		mirror = 1;

	m_bVideoOverlayUsed = false;
	m_bOVToRecreate = false;

	m_bFirstRepaint = false;

	m_bRetrievedInfos = true;
	
	return true;
}

//----------------------------------------------------------------------------
// NAME: ATIRadeon::ATIRadeon
// DESC: constructor for ATIRadeon, probes and initializes for Radeon cards
// NOTE: m_bIsInitialized returns if initialization has been successful
// SEE ALSO:
//----------------------------------------------------------------------------

ATIRadeon::ATIRadeon( int nFd ) : m_cLock ("radeon_hardware_lock")
{
	m_bIsInitialized = false;
	m_bRetrievedInfos = false;

	memset( &rinfo, 0, sizeof( rinfo ) );

	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Radeon :: Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}

	// scan all PCI devices (we need to scan again)
	for( int i = 0 ; get_pci_info( &m_cPCIInfo, i ) == 0 ; i++ ) {
		if( ProbeHardware (nFd, &m_cPCIInfo) ) { m_bIsInitialized = true; break; }
	}

	if( m_bIsInitialized == false ) {
		dbprintf("Radeon :: No supported cards found\n");
		return;
	}

	// OK. card found. now initialize it.
	if( !InitHardware ( nFd ) ) {
		m_bIsInitialized = false;
		return;
	}
}

//----------------------------------------------------------------------------
// NAME: ATIRadeon::~ATIRadeon
// DESC: destructor of Radeon driver
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ATIRadeon::~ATIRadeon()
{
	if(m_hFramebufferArea >= 0)
		delete_area(m_hFramebufferArea);
	if(m_hRegisterArea >= 0)
		delete_area(m_hRegisterArea);
	if(m_hROMArea >= 0)
		delete_area(m_hROMArea);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

area_id ATIRadeon::Open( void ) {
	return( m_hFramebufferArea );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ATIRadeon::Close( void )
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ATIRadeon::GetScreenModeCount()
{
	return( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ATIRadeon::GetScreenModeDesc( int nIndex, screen_mode* psMode )
{
    if ( nIndex >= 0 && nIndex < int(m_cModeList.size()) ) {
        *psMode = m_cModeList[nIndex];
        return( true );
    } else {
        return( false );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ATIRadeon::SetScreenMode( screen_mode sMode )
{
	while(!m_bRetrievedInfos)
		snooze(1);
	
	if( sMode.m_eColorSpace == CS_CMAP8 || sMode.m_eColorSpace == CS_RGB24 ) {
		dbprintf ("Radeon :: 8BPP or 24BPP modes are not supported!\n");
		return( -1 );
	}

	/* find display mode */
	VideoMode * mode = DefaultVideoModes;
	VideoMode * bestVMode = NULL;
	while( mode->Width > 0 ) {
		if( mode->Width == sMode.m_nWidth &&
		mode->Height == sMode.m_nHeight &&
		mode->Refresh <= int (sMode.m_vRefreshRate) ) {
			// we have found mode with given size and compatible refresh
			if( bestVMode == NULL )
				bestVMode = mode;
			else if( (int (sMode.m_vRefreshRate)-mode->Refresh) <
			(int (sMode.m_vRefreshRate)-bestVMode->Refresh) )
				bestVMode = mode;
		}
		mode++;
	}

	if( !bestVMode ) {
		dbprintf ("Radeon :: Unable to find video mode %dx%d %dhz\n", sMode.m_nWidth, sMode.m_nHeight, int (sMode.m_vRefreshRate));
		return( -1 );
	}

	mode = bestVMode;

	struct radeon_regs newmode;
	int hTotal, vTotal, hSyncStart, hSyncEnd,
	    hSyncPol, vSyncStart, vSyncEnd, vSyncPol, cSync;
	uint8 hsync_adj_tab[] = {0, 0x12, 9, 9, 6, 5};
	uint8 hsync_fudge_fp[] = {2, 2, 0, 0, 5, 5};
	uint32 sync, h_sync_pol, v_sync_pol, dotClock, pixClock;
	int i, freq;
	int format = 0;
	int nopllcalc = 0;
	int hsync_start, hsync_fudge, bytpp, hsync_wid, vsync_wid;
	int primary_mon = PRIMARY_MONITOR(rinfo);
	int depth = BitsPerPixel( sMode.m_eColorSpace );
	int use_rmx = 0;	

	/* We always want engine to be idle on a mode switch, even
	 * if we won't actually change the mode
	 */
	EngineIdle();

	hSyncStart = mode->Width + mode->right_margin;
	hSyncEnd = hSyncStart + mode->hsync_len;
	hTotal = hSyncEnd + mode->left_margin;

	vSyncStart = mode->Height + mode->lower_margin;
	vSyncEnd = vSyncStart + mode->vsync_len;
	vTotal = vSyncEnd + mode->upper_margin;
	pixClock = mode->Clock;

	sync = mode->Flags;
	h_sync_pol = sync & V_PHSYNC ? 0 : 1;
	v_sync_pol = sync & V_PVSYNC ? 0 : 1;

	if (primary_mon == MT_DFP || primary_mon == MT_LCD) {
		if (rinfo.panel_info.xres < mode->Width)
			mode->Width = rinfo.panel_info.xres;
		if (rinfo.panel_info.yres < mode->Height)
			mode->Height = rinfo.panel_info.yres;

		hTotal = mode->Width + rinfo.panel_info.hblank;
		hSyncStart = mode->Width + rinfo.panel_info.hOver_plus;
		hSyncEnd = hSyncStart + rinfo.panel_info.hSync_width;

		vTotal = mode->Height + rinfo.panel_info.vblank;
		vSyncStart = mode->Height + rinfo.panel_info.vOver_plus;
		vSyncEnd = vSyncStart + rinfo.panel_info.vSync_width;

		/* AK: This was enabled in the radeon framebuffer driver but disabled 
		 in the syllable driver */
		h_sync_pol = !rinfo.panel_info.hAct_high;
		v_sync_pol = !rinfo.panel_info.vAct_high;

		pixClock = 100000000 / rinfo.panel_info.clock;

		/* Disabled because it causes problems on Mobility chips -MK */
		/* AK: Enabled again */
		if (rinfo.panel_info.use_bios_dividers) {
			nopllcalc = 1;
			newmode.ppll_div_3 = rinfo.panel_info.fbk_divider |
				(rinfo.panel_info.post_divider << 16);
			newmode.ppll_ref_div = rinfo.pll.ref_div;
		}
	}
	dotClock = 1000000000 / pixClock;
	rinfo.dotClock = dotClock;
	freq = dotClock / 10; /* x100 */

	RTRACE("hStart = %d, hEnd = %d, hTotal = %d\n",
		hSyncStart, hSyncEnd, hTotal);
	RTRACE("vStart = %d, vEnd = %d, vTotal = %d\n",
		vSyncStart, vSyncEnd, vTotal);

	hsync_wid = (hSyncEnd - hSyncStart) / 8;
	vsync_wid = vSyncEnd - vSyncStart;
	if (hsync_wid == 0)
		hsync_wid = 1;
	else if (hsync_wid > 0x3f)	/* max */
		hsync_wid = 0x3f;

	if (vsync_wid == 0)
		vsync_wid = 1;
	else if (vsync_wid > 0x1f)	/* max */
		vsync_wid = 0x1f;

	hSyncPol = mode->Flags & V_PHSYNC ? 0 : 1;
	vSyncPol = mode->Flags & V_PVSYNC ? 0 : 1;
	cSync = 0;

	format = GetDstBpp(depth);
	bytpp = BitsPerPixel( sMode.m_eColorSpace ) >> 3;

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD))
		hsync_fudge = hsync_fudge_fp[format-1];
	else
		hsync_fudge = hsync_adj_tab[format-1];

	hsync_start = hSyncStart - 8 + hsync_fudge;

	newmode.crtc_gen_cntl = CRTC_EXT_DISP_EN | CRTC_EN |
				(format << 8);

	/* Clear auto-center etc... */
	newmode.crtc_more_cntl = rinfo.init_state.crtc_more_cntl;
	newmode.crtc_more_cntl &= 0xfffffff0;
	
	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD)) {
		newmode.crtc_ext_cntl = VGA_ATI_LINEAR | XCRT_CNT_EN;
		if (mirror)
			newmode.crtc_ext_cntl |= CRTC_CRT_ON;

		newmode.crtc_gen_cntl &= ~(CRTC_DBL_SCAN_EN |
					   CRTC_INTERLACE_EN);
	} else {
		newmode.crtc_ext_cntl = VGA_ATI_LINEAR | XCRT_CNT_EN |
					CRTC_CRT_ON;
	}

	newmode.dac_cntl = /* INREG(DAC_CNTL) | */ DAC_MASK_ALL | DAC_VGA_ADR_EN |
			   DAC_8BIT_EN;

	newmode.crtc_h_total_disp = ((((hTotal / 8) - 1) & 0x3ff) |
				     (((mode->Width / 8) - 1) << 16));

	newmode.crtc_h_sync_strt_wid = ((hsync_start & 0x1fff) |
					(hsync_wid << 16) | (h_sync_pol << 23));

	newmode.crtc_v_total_disp = ((vTotal - 1) & 0xffff) |
				    ((mode->Height - 1) << 16);

	newmode.crtc_v_sync_strt_wid = (((vSyncStart - 1) & 0xfff) |
					 (vsync_wid << 16) | (v_sync_pol  << 23));

	/* We first calculate the engine pitch */
	rinfo.pitch = ((mode->Width * BytesPerPixel( sMode.m_eColorSpace ) + 0x3f)
 			& ~(0x3f)) >> 6;

	/* Then, re-multiply it to get the CRTC pitch */
	newmode.crtc_pitch = (rinfo.pitch << 3) / BytesPerPixel( sMode.m_eColorSpace );

	newmode.crtc_pitch |= (newmode.crtc_pitch << 16);

	/*
	 * It looks like recent chips have a problem with SURFACE_CNTL,
	 * setting SURF_TRANSLATION_DIS completely disables the
	 * swapper as well, so we leave it unset now.
	 */
	newmode.surface_cntl = 0;

	/* Clear surface registers */
	for (i=0; i<8; i++) {
		newmode.surf_lower_bound[i] = 0;
		newmode.surf_upper_bound[i] = 0x1f;
		newmode.surf_info[i] = 0;
	}

	RTRACE("h_total_disp = 0x%x\t   hsync_strt_wid = 0x%x\n",
		(uint)newmode.crtc_h_total_disp, (uint)newmode.crtc_h_sync_strt_wid);
	RTRACE("v_total_disp = 0x%x\t   vsync_strt_wid = 0x%x\n",
		(uint)newmode.crtc_v_total_disp, (uint)newmode.crtc_v_sync_strt_wid);

	rinfo.bpp = BitsPerPixel( sMode.m_eColorSpace );
	rinfo.depth = depth;

	RTRACE("pixclock = %lu\n", (unsigned long)pixClock);
	RTRACE("freq = %lu\n", (unsigned long)freq);

	/* We use PPLL_DIV_3 */
	newmode.clk_cntl_index = 0x300;
	

	if(!nopllcalc)
		CalcPLLRegs(&newmode, freq);

	newmode.vclk_ecp_cntl = rinfo.init_state.vclk_ecp_cntl;

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD)) {
		unsigned int hRatio, vRatio;

		if (mode->Width > rinfo.panel_info.xres)
			mode->Width = rinfo.panel_info.xres;
		if (mode->Height > rinfo.panel_info.yres)
			mode->Height = rinfo.panel_info.yres;

		newmode.fp_horz_stretch = (((rinfo.panel_info.xres / 8) - 1)
					   << HORZ_PANEL_SHIFT);
		newmode.fp_vert_stretch = ((rinfo.panel_info.yres - 1)
					   << VERT_PANEL_SHIFT);

		if (mode->Width != rinfo.panel_info.xres) {
			hRatio = round_div(mode->Width * HORZ_STRETCH_RATIO_MAX,
					   rinfo.panel_info.xres);
			newmode.fp_horz_stretch = (((((unsigned long)hRatio) & HORZ_STRETCH_RATIO_MASK)) |
						   (newmode.fp_horz_stretch &
						    (HORZ_PANEL_SIZE | HORZ_FP_LOOP_STRETCH |
						     HORZ_AUTO_RATIO_INC)));
			newmode.fp_horz_stretch |= (HORZ_STRETCH_BLEND |
						    HORZ_STRETCH_ENABLE);
			use_rmx = 1;
		}
		newmode.fp_horz_stretch &= ~HORZ_AUTO_RATIO;

		if (mode->Height != rinfo.panel_info.yres) {
			vRatio = round_div(mode->Height * VERT_STRETCH_RATIO_MAX,
					   rinfo.panel_info.yres);
			newmode.fp_vert_stretch = (((((unsigned long)vRatio) & VERT_STRETCH_RATIO_MASK)) |
						   (newmode.fp_vert_stretch &
						   (VERT_PANEL_SIZE | VERT_STRETCH_RESERVED)));
			newmode.fp_vert_stretch |= (VERT_STRETCH_BLEND |
						    VERT_STRETCH_ENABLE);
			use_rmx = 1;
		}
		newmode.fp_vert_stretch &= ~VERT_AUTO_RATIO_EN;

		newmode.fp_gen_cntl = (rinfo.init_state.fp_gen_cntl & (uint32)
				       ~(FP_SEL_CRTC2 |
					 FP_RMX_HVSYNC_CONTROL_EN |
					 FP_DFP_SYNC_SEL |
					 FP_CRT_SYNC_SEL |
					 FP_CRTC_LOCK_8DOT |
					 FP_USE_SHADOW_EN |
					 FP_CRTC_USE_SHADOW_VEND |
					 FP_CRT_SYNC_ALT));

		newmode.fp_gen_cntl |= (FP_CRTC_DONT_SHADOW_VPAR |
					FP_CRTC_DONT_SHADOW_HEND |
					FP_PANEL_FORMAT);
					
		if (IS_R300_VARIANT(&rinfo) ||
		    (rinfo.family == CHIP_FAMILY_R200)) {
			newmode.fp_gen_cntl &= ~R200_FP_SOURCE_SEL_MASK;
			if (use_rmx)
				newmode.fp_gen_cntl |= R200_FP_SOURCE_SEL_RMX;
			else
				newmode.fp_gen_cntl |= R200_FP_SOURCE_SEL_CRTC1;
		} else
			newmode.fp_gen_cntl |= FP_SEL_CRTC1;


		newmode.lvds_gen_cntl = rinfo.init_state.lvds_gen_cntl;
		newmode.lvds_pll_cntl = rinfo.init_state.lvds_pll_cntl;
		newmode.tmds_crc = rinfo.init_state.tmds_crc;
		newmode.tmds_transmitter_cntl = rinfo.init_state.tmds_transmitter_cntl;

		if (primary_mon == MT_LCD) {
			newmode.lvds_gen_cntl |= (LVDS_ON | LVDS_BLON);
			newmode.fp_gen_cntl &= ~(FP_FPON | FP_TMDS_EN);
		} else {
			/* DFP */
			newmode.fp_gen_cntl |= (FP_FPON | FP_TMDS_EN);
			newmode.tmds_transmitter_cntl &= ~(TMDS_PLLRST);
			/* TMDS_PLL_EN bit is reversed on RV (and mobility) chips */
			if (IS_R300_VARIANT(&rinfo) ||
			    (rinfo.family == CHIP_FAMILY_R200) || !rinfo.has_CRTC2)
				newmode.tmds_transmitter_cntl &= ~TMDS_PLL_EN;
			else
				newmode.tmds_transmitter_cntl |= TMDS_PLL_EN;
			newmode.crtc_ext_cntl &= ~CRTC_CRT_ON;
		}

		newmode.fp_crtc_h_total_disp = (((rinfo.panel_info.hblank / 8) & 0x3ff) |
				(((mode->Width / 8) - 1) << 16));
		newmode.fp_crtc_v_total_disp = (rinfo.panel_info.vblank & 0xffff) |
				((mode->Height - 1) << 16);
		newmode.fp_h_sync_strt_wid = ((rinfo.panel_info.hOver_plus & 0x1fff) |
				(hsync_wid << 16) | (h_sync_pol << 23));
		newmode.fp_v_sync_strt_wid = ((rinfo.panel_info.vOver_plus & 0xfff) |
				(vsync_wid << 16) | (v_sync_pol  << 23));
	}

	/* do it! */
	if (!rinfo.asleep) {
		WriteMode (&newmode);
		InitPalette();
		/* (re)initialize the engine */
		EngineInit ();

		/* blank video memory for first mode set */
		if(!m_bFirstRepaint)
		{
			snooze(1000);

			/* paint background black for first mode set */
			m_cLock.Lock();

			FifoWait(4);  
  			OUTREG(DP_GUI_MASTER_CNTL,  
				rinfo.dp_gui_master_cntl
       	   	      | GMC_BRUSH_SOLID_COLOR
       	   	      | ROP3_P);
			OUTREG(DP_BRUSH_FRGD_CLR, 0x000000);
			OUTREG(DP_WRITE_MSK, 0xffffffff);
			OUTREG(DP_CNTL, (DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM));

			FifoWait(2);  
			OUTREG(DST_Y_X, 0);
			OUTREG(DST_WIDTH_HEIGHT, ((sMode.m_nWidth) << 16) | sMode.m_nHeight );
	
			EngineIdle();
	
			m_cLock.Unlock();
			
			m_bFirstRepaint = true;
		}
	}
	
	m_sCurrentMode.m_nWidth = sMode.m_nWidth;
	m_sCurrentMode.m_nHeight = sMode.m_nHeight;
	m_sCurrentMode.m_eColorSpace = sMode.m_eColorSpace;
	m_sCurrentMode.m_vRefreshRate = bestVMode->Refresh;
	m_sCurrentMode.m_nBytesPerLine = ( ( sMode.m_nWidth + 63 ) & ~63 ) * BytesPerPixel( sMode.m_eColorSpace );

	return 0;
}

screen_mode ATIRadeon::GetCurrentScreenMode()
{
	return( m_sCurrentMode );
}

//----------------------------------------------------------------------------
// NAME: ATIRadeon::IsInitialized
// DESC: returns the value of m_bIsInitialized
// NOTE:
// SEE ALSO: ATIRadeon::ATIRadeon
//----------------------------------------------------------------------------

bool ATIRadeon::IsInitialized() {
	return( m_bIsInitialized );
}

//----------------------------------------------------------------------------
// NAME: init_gfx_driver
// DESC: C extern entry point for loading the driver
// NOTE:
// SEE ALSO: ATIRadeon::ATIRadeon
//----------------------------------------------------------------------------

extern "C"
DisplayDriver* init_gfx_driver( int nFd ) {

	try {
		ATIRadeon* pcDriver = new ATIRadeon( nFd );
		if (pcDriver->IsInitialized()) {
			return pcDriver;
		} else {
			delete pcDriver;
			return NULL ;
		}
	} catch (std::exception&  cExc) {
		dbprintf( "Radeon :: Got exception\n" );
		return NULL ;
	}
}

