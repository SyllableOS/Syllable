
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
 *
 * Revision History :
 *		16/8/00		Initial version		Joel Smith
 *							<joels@mobyfoo.org>
 *		20/8/00		Modified to match the	Kurt Skauen
 *				AtheOS coding style	<kurt@atheos.cx>
 */

#include <atheos/types.h>
#include <posix/errno.h>
#include <atheos/atomic.h>
#include <atheos/kernel.h>
#include <atheos/dma.h>

/* Macros for port I/O. */
#define dma_out( reg, val )     isa_writeb( (reg), (val) )
#define dma_in( reg )           isa_readb( (reg) )

typedef struct
{
	int dc_nLock;
	uint dc_nAddress;
	uint dc_nCount;
	uint dc_nPage;
} DMAChannel_s;

static DMAChannel_s g_asDMAChannels[DMA_NUM_CHANNELS] = {
	/* 8 bit channels */
	{0, 0x00, 0x01, 0x87},
	{0, 0x02, 0x03, 0x83},
	{0, 0x04, 0x05, 0x81},
	{0, 0x06, 0x07, 0x82},
	/* 16 bit channels */
	{1, 0xc0, 0xc2, 0x00},	/* cascade */
	{0, 0xc4, 0xc6, 0x8b},
	{0, 0xc8, 0xca, 0x89},
	{0, 0xcc, 0xce, 0x8a}
};

status_t request_dma( uint nDMAChannel )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	if ( atomic_swap( &g_asDMAChannels[nDMAChannel].dc_nLock, 1 ) != 0 )
	{
		return ( -EBUSY );
	}
	return ( EOK );
}

status_t free_dma( uint nDMAChannel )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	if ( atomic_swap( &g_asDMAChannels[nDMAChannel].dc_nLock, 0 ) == 0 )
	{
		return ( -EINVAL );
	}
	return ( EOK );
}

status_t enable_dma_channel( uint nDMAChannel )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	dma_out( ( nDMAChannel < 4 ) ? DMA_1_MASK : DMA_2_MASK, nDMAChannel & 3 );

	return ( EOK );
}

status_t disable_dma_channel( uint nDMAChannel )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	dma_out( ( nDMAChannel < 4 ) ? DMA_1_MASK : DMA_2_MASK, ( nDMAChannel & 3 ) | 4 );

	return ( EOK );
}

status_t clear_dma_ff( uint nDMAChannel )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	dma_out( ( nDMAChannel < 4 ) ? DMA_1_FF : DMA_2_FF, 0 );

	return ( EOK );
}

status_t set_dma_mode( uint nDMAChannel, uint8 nMode )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	dma_out( ( nDMAChannel < 4 ) ? DMA_1_MODE : DMA_2_MODE, ( nDMAChannel & 3 ) | nMode );

	return ( EOK );
}

status_t set_dma_page( uint nDMAChannel, uint8 nPage )
{
	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	dma_out( g_asDMAChannels[nDMAChannel].dc_nPage, ( nDMAChannel < 4 ) ? nPage : nPage & 0xFE );

	return ( EOK );
}

status_t set_dma_address( uint nDMAChannel, void *pAddress )
{
	register uint nAddr = ( uint )pAddress;

	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	if ( nDMAChannel < 4 )
	{
		dma_out( g_asDMAChannels[nDMAChannel].dc_nAddress, nAddr & 0xFF );
		dma_out( g_asDMAChannels[nDMAChannel].dc_nAddress, ( nAddr >> 8 ) & 0xFF );
	}
	else
	{
		dma_out( g_asDMAChannels[nDMAChannel].dc_nAddress, ( nAddr >> 1 ) & 0xFF );
		dma_out( g_asDMAChannels[nDMAChannel].dc_nAddress, ( nAddr >> 9 ) & 0xFF );
	}

	return ( EOK );
}

status_t set_dma_count( uint nDMAChannel, uint16 nCount )
{
	register uint16 i = nCount - 1;

	if ( nDMAChannel >= DMA_NUM_CHANNELS )
	{
		return ( -EINVAL );
	}
	if ( nDMAChannel < 4 )
	{
		dma_out( g_asDMAChannels[nDMAChannel].dc_nCount, i & 0xFF );
		dma_out( g_asDMAChannels[nDMAChannel].dc_nCount, ( i >> 8 ) & 0xFF );
	}
	else
	{
		dma_out( g_asDMAChannels[nDMAChannel].dc_nCount, ( i >> 1 ) & 0xFF );
		dma_out( g_asDMAChannels[nDMAChannel].dc_nCount, ( i >> 9 ) & 0xFF );
	}

	return ( EOK );
}
