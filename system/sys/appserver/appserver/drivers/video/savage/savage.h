/*
 *  S3 Savage driver for Syllable
 *
 *  Based on the original SavageIX/MX driver for Syllable by Hillary Cheng
 *  and the X.org Savage driver.
 *
 *  X.org driver copyright Tim Roberts (timr@probo.com),
 *  Ani Joshi (ajoshi@unixbox.com) & S. Marineau
 *
 *  Copyright 2005 Kristian Van Der Vliet
 *  Copyright 2003 Hilary Cheng (hilarycheng@yahoo.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef SYLLABLE_SAVAGE_H__
#define SYLLABLE_SAVAGE_H__

/* PCI device ID's for supported chipsets */
/* XXXKV: We really, really, really need to update pci_vendors.h! */
#define PCI_DEVICE_ID_SAVAGE3D			0x8A20
#define PCI_DEVICE_ID_SAVAGE3D_MV		0x8A21
#define PCI_DEVICE_ID_SAVAGE4			0x8A22
#define PCI_DEVICE_ID_PROSAVAGE_PM		0x8A25
#define PCI_DEVICE_ID_PROSAVAGE_KM		0x8A26
#define PCI_DEVICE_ID_SAVAGE_MX_MV		0x8C10
#define PCI_DEVICE_ID_SAVAGE_MX			0x8C11
#define PCI_DEVICE_ID_SAVAGE_IX_MV		0x8C12
#define PCI_DEVICE_ID_SAVAGE_IX			0x8C13
#define PCI_DEVICE_ID_SUPSAV_MX128		0x8C22
#define PCI_DEVICE_ID_SUPSAV_MX64		0x8C24
#define PCI_DEVICE_ID_SUPSAV_MX64C		0x8C26
#define PCI_DEVICE_ID_SUPSAV_IX128SDR	0x8C2A
#define PCI_DEVICE_ID_SUPSAV_IX128DDR	0x8C2B
#define PCI_DEVICE_ID_SUPSAV_IX64SDR	0x8C2C
#define PCI_DEVICE_ID_SUPSAV_IX64DDR	0x8C2D
#define PCI_DEVICE_ID_SUPSAV_IXCSDR		0x8C2E
#define PCI_DEVICE_ID_SUPSAV_IXCDDR		0x8C2F
#define PCI_DEVICE_ID_S3TWISTER_P		0x8D01
#define PCI_DEVICE_ID_S3TWISTER_K		0x8D02
#define PCI_DEVICE_ID_PROSAVAGE_DDR		0x8D03
#define PCI_DEVICE_ID_PROSAVAGE_DDRK	0x8D04
#define PCI_DEVICE_ID_SAVAGE2000		0x9102

/* IO methods */
#define VGAOUT8(a, b)	*((vuint8*)(psCard->VgaMem + a)) = b
#define VGAIN8(a)		*((vuint8*)(psCard->VgaMem + a))
#define VGAOUT16(a, b)	*((vuint16*)(psCard->VgaMem + a)) = b
#define VGAIN16(a)		*((vuint16*)(psCard->VgaMem + a))

#define OUTREG(a, b)	*((vuint32*)(psCard->MapBase + a)) = b
#define INREG(a)		*((vuint32*)(psCard->MapBase + a))
#define OUTREG32(a, b)	*((vuint32*)(psCard->MapBase + a)) = b
#define INREG32(a)		*((vuint32*)(psCard->MapBase + a))
#define OUTREG16(a, b)	*((vuint16*)(psCard->MapBase + a)) = b
#define INREG16(a)		*((vuint16*)(psCard->MapBase + a))
#define OUTREG8(a, b)	*((vuint8*)(psCard->MapBase + a)) = b
#define INREG8(a)		*((vuint8*)(psCard->MapBase + a))

/* Additional registers not provided by savage_regs.h */
#define VGA_MISC_OUT_W			0x3c2
#define VGA_MISC_OUT_R			0x3cc
#define VGA_IN_STAT_1_OFFSET	0x0a

typedef enum {
    MT_NONE,
    MT_CRT,
    MT_LCD,
    MT_DFP,
    MT_TV
} SavageMonitorType;

/*  Tiling defines */
#define TILE_SIZE_BYTE          2048   /* 0x800, 2K */
#define TILE_SIZE_BYTE_2000     4096

#define TILEHEIGHT_16BPP        16
#define TILEHEIGHT_32BPP        16
#define TILEHEIGHT              16      /* all 16 and 32bpp tiles are 16 lines high */
#define TILEHEIGHT_2000         32      /* 32 lines on savage 2000 */

#define TILEWIDTH_BYTES         128     /* 2048/TILEHEIGHT (** not for use w/8bpp tiling) */
#define TILEWIDTH8BPP_BYTES     64      /* 2048/TILEHEIGHT_8BPP */
#define TILEWIDTH_16BPP         64      /* TILEWIDTH_BYTES/2-BYTES-PER-PIXEL */
#define TILEWIDTH_32BPP         32      /* TILEWIDTH_BYTES/4-BYTES-PER-PIXEL */

#endif
