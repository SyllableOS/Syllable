/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  Code from the Linux ide driver is
 *  Copyright (C) 1994, 1995, 1996  scott snyder  <snyder@fnald0.fnal.gov>
 *  Copyright (C) 1996-1998  Erik Andersen <andersee@debian.org>
 *  Copyright (C) 1998-2000  Jens Axboe <axboe@suse.de>
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

#ifndef __ATA_DMA_H_
#define __ATA_DMA_H_

#include "ata.h"

/* Busmaster registers */
#define ATA_DMA_CONTROL 0x0
#define		DMA_CONTROL_STOP			0x00
#define		DMA_CONTROL_START			0x01
#define		DMA_CONTROL_READ			0x08

#define ATA_DMA_STATUS	0x2
#define		DMA_STATUS_ACTIVE			0x01
#define 	DMA_STATUS_ERROR			0x02
#define		DMA_STATUS_IRQ				0x04
#define 	DMA_STATUS_DRIVE0_ENABLED	0x20
#define		DMA_STATUS_DRIVE1_ENABLED	0x40

#define ATA_DMA_TABLE 0x4

int ata_dma_check( int drive );
bool ata_dma_table_prepare( int drive, void *buffer, uint32 len );
void ata_dma_read( int drive );
void ata_dma_write( int drive );
void ata_dma_start( int drive );
int ata_dma_stop( int drive );


/* I/O */
#define ata_dma_inb( reg )		inb( g_nControllers[controller].dma_base + reg )
#define ata_dma_inl( reg )		inl( g_nControllers[controller].dma_base + reg )

#define ata_dma_outb( reg, value )	outb( value, g_nControllers[controller].dma_base + reg )
#define ata_dma_outl( reg, value )	outl( value, g_nControllers[controller].dma_base + reg )

#endif

