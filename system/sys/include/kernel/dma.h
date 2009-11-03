/*
 *  DMA Support for the AtheOS kernel
 *  Copyright (C) 2000  Joel Smith
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
                 
#ifndef __F_KERNEL_DMA_H__
#define __F_KERNEL_DMA_H__

#include <kernel/types.h>

#define DMA_NUM_CHANNELS	0x08

#define DMA_1_STATUS		0x08
#define DMA_1_COMMAND		0x08
#define DMA_1_REQUEST		0x09
#define DMA_1_MASK		0x0A
#define DMA_1_MODE		0x0B
#define DMA_1_FF		0x0C

#define DMA_2_STATUS		0xD0
#define DMA_2_COMMAND		0xD0
#define DMA_2_REQUEST		0xD2
#define DMA_2_MASK		0xD4
#define DMA_2_MODE		0xD6
#define DMA_2_FF		0xD8

#define DMA_MODE_READ   	0x44
#define DMA_MODE_WRITE  	0x48

extern status_t request_dma( uint nDMAChannel );
extern status_t free_dma( uint nDMAChannel );

extern status_t enable_dma_channel( uint nDMAChannel );
extern status_t disable_dma_channel( uint nDMAChannel );

extern status_t clear_dma_ff( uint nDMAChannel );

extern status_t set_dma_mode( uint nDMAChannel, uint8 nMode );
extern status_t set_dma_page( uint nDMAChannel, uint8 nPage );
extern status_t set_dma_address( uint nDMAChannel, void* pAddress );
extern status_t set_dma_count( uint nDMAChannel, uint16 nCount );

#endif	/* __F_KERNEL_DMA_H__ */
