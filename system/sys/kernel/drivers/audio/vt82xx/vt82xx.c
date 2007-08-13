/*
 *	VIA audio driver
 *	Copyright (C) 2006 Arno Klenke
 * 
 *	based on:
 *   ALSA driver for VIA VT82xx (South Bridge)
 *
 *   VT82C686A/B/C, VT8233A/C, VT8235
 *
 *	Copyright (c) 2000 Jaroslav Kysela <perex@suse.cz>
 *	                   Tjeerd.Mulder <Tjeerd.Mulder@fujitsu-siemens.com>
 *                    2002 Takashi Iwai <tiwai@suse.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <posix/signal.h>
#include <macros.h>

#include "ac97audio.h"
#include "vt82xx.h"

static PCI_bus_s *g_psBus;

struct audio_device
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
};

struct audio_device g_sDevices[] = {
	{0x1106, 0x3059, "VIA", "VT82XX"}
};


/* AC97 functions */
status_t via_ac97_wait( void* pData, int nCodec )
{
	VIAAudioDriver_s* psDriver = ( VIAAudioDriver_s* )pData;
	int nTimeout = 0;
	uint32 nVal;
	while( nTimeout++ < 1000 )
	{
		nVal = inl( psDriver->nBaseAddr + VIA_AC97_CTRL );
		if( !( nVal & VIA_CR80_BUSY ) )
			return( 0 );
		snooze( 1000 );
	}
	return( -1 );
}

status_t via_ac97_write( void* pData, int nCodec, uint8 nReg, uint16 nVal )
{
	VIAAudioDriver_s* psDriver = ( VIAAudioDriver_s* )pData;
	
	uint32 nData = ( nReg << 16 ) + nVal;
	outl( nData, psDriver->nBaseAddr + VIA_AC97_CTRL );
	
	snooze( 20 );
	return( via_ac97_wait( pData, nCodec ) );
}

uint16 via_ac97_read( void* pData, int nCodec, uint8 nReg )
{
	VIAAudioDriver_s* psDriver = ( VIAAudioDriver_s* )pData;
	int nTimeout = 0;
	uint32 nData = ( (uint32)nReg << 16 ) | VIA_CR80_READ |  VIA_CR80_VALID;
	outl( nData, psDriver->nBaseAddr + VIA_AC97_CTRL );
	
	snooze( 20 );
	
	while( nTimeout++ < 1000 )
	{
		nData = inl( psDriver->nBaseAddr + VIA_AC97_CTRL );
		if( ( nData & ( VIA_CR80_BUSY | VIA_CR80_VALID ) ) == VIA_CR80_VALID )
			goto out;
		snooze( 1000 );
	}
	printk( "AC97Read error!\n" );
	return( 0xffff );
	
out:	
	snooze( 25 );
	nData = inl( psDriver->nBaseAddr + VIA_AC97_CTRL );
	
	if( ( ( nData & 0x007F0000) >> 16 ) == nReg )
	{
		return( nData & 0x0000FFFF );
	}
	
	printk( "AC97Read error!\n" );
	return( 0xffff );
}


status_t via_ac97_reset( VIAAudioDriver_s* psDriver )
{
	uint8 nVal;
	/* Initialize Codec */
	nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_STAT, 1 );
	if( !( nVal & VIA_ACLINK_C00_READY ) )
	{
		printk( "Initialize codec #0\n" );
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_CTRL, 1,
									VIA_CR41_AC97_ENABLE | VIA_CR41_AC97_RESET | VIA_CR41_AC97_WAKEUP );
		snooze( 100 );
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_CTRL, 1,
									VIA_CR41_AC97_RESET | VIA_CR41_AC97_WAKEUP );
		
		snooze( 100 );
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_CTRL, 1,
									VIA_CR41_AC97_ENABLE | VIA_CR41_PCM_ENABLE | VIA_CR41_VRA | VIA_CR41_AC97_RESET );
		snooze( 100 );
	}
	
	nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_CTRL, 1 );
	if( ( nVal & VIA_CR41_RUN_MASK ) != VIA_CR41_RUN_MASK ) {
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_CTRL, 1,
									VIA_CR41_RUN_MASK );
		snooze( 100 );
	}
	
	/* Wait for codec ready */
	int nTries = 0;
	do {
		nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_ACLINK_STAT, 1 );
		if( nVal & VIA_ACLINK_C00_READY )
			break;
		snooze( 1000 );
		nTries++;
	} while( nTries < 1000 );
	
	if( nTries == 1000 ) {
		printk( "Could not reset codec #0\n" );
		return( -EIO );
	}
	
	/* Reset codec */
	via_ac97_wait( psDriver, 0 );
	via_ac97_write( psDriver, 0, AC97_RESET, 0x0000 );
	via_ac97_read( psDriver, 0, 0 );
	
	printk( "AC97 Reset finished\n" );
	
	return( 0 );
}


/* Streams */

static status_t via_check_format( VIAAudioDriver_s* psDriver, VIAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	/* Resolution */
	if( nResolution != 16 )
		return( -1 );
	
	/* Check if we support the rate */
	if( ( !ac97_supports_vra( &psDriver->sAC97, 0 ) || psStream->bIsSPDIF ) && nSampleRate != 48000  )
	{
		return( -1 );
	}
	
	/* Check if we support the number of channels */
	if( ( nChannels > ac97_get_max_channels( &psDriver->sAC97 ) ) || ( ( nChannels != 2 )
		&& ( nChannels != 4 ) && ( nChannels != 6 ) ) )
	{
		return( -1 );
	}
	
	if( psStream->bIsRecord && nChannels > 2 )
	{
		return( -1 );
	}
	
	return( 0 );
}

static status_t via_set_format( VIAAudioDriver_s* psDriver, VIAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	LOCK( psDriver->hLock );
	
	/* Save data */
	psStream->nChannels = nChannels;
	psStream->nSampleRate = nSampleRate;
	
	
	
	/* TODO: Support multiple codecs */
	
	
	/* Set rates (ignore errors) */
	if( psStream->bIsRecord )
	{
		printk( "Set record format\n" );
		ac97_set_rate( &psDriver->sAC97, 0, AC97_PCM_LR_ADC_RATE, nSampleRate );
	}
	else
	{
		ac97_set_rate( &psDriver->sAC97, 0, AC97_PCM_FRONT_DAC_RATE, nSampleRate );
		if( psStream->bIsSPDIF )
		{
			ac97_set_rate( &psDriver->sAC97, 0, AC97_SPDIF_CONTROL, nSampleRate );
		}
		if( nChannels > 2 )
		{
			ac97_set_rate( &psDriver->sAC97, 0, AC97_PCM_SURR_DAC_RATE, nSampleRate );
			ac97_set_rate( &psDriver->sAC97, 0, AC97_PCM_LFE_DAC_RATE, nSampleRate );		
		}
	}
	
	
	/* Reset engine */
	if( inb( psStream->nIoBase + VIA_PCM_STATUS ) & VIA_SGD_ACTIVE )
		outb( VIA_SGD_TERMINATE, psStream->nIoBase + VIA_PCM_CONTROL );
	snooze( 50 );
	outb( 0, psStream->nIoBase + VIA_PCM_CONTROL );
	outb( VIA_SGD_FLAG | VIA_SGD_EOL, psStream->nIoBase + VIA_PCM_STATUS );
	outb( 0, psStream->nIoBase + VIA_PCM_TYPE );
	
	/* Set format */
	uint32 nPcmFmt = 0;
	uint32 nPcmStop = 0;
	if( psStream->bIsRecord )
	{
		nPcmStop = VIA_PCM_STOP_16BIT;
		if( nChannels == 2 )
			nPcmStop |= VIA_PCM_STOP_STEREO;
	}
	else
	{
		nPcmFmt = VIA_PCM_FMT_VT_16BIT | ( nChannels << 4 );
		nPcmStop = 0;
		switch( nChannels )
		{
			case 1:
				nPcmStop = (1<<0) | (1<<4);
				break;
			case 2:
				nPcmStop = (1<<0) | (2<<4);
				break;
			case 4:
				nPcmStop = (1<<0) | (2<<4) | (3<<8) | (4<<12);
				break;
			case 6:
				nPcmStop = (1<<0) | (2<<4) | (3<<8) | (4<<12) | (5<<16) | (6<<20);
				break;
			default:
				UNLOCK( psDriver->hLock );
				return( -EINVAL );
		}
	}
	
	nPcmStop |= VIA_PCM_STOP_NEVER;
	if( psStream->bIsRecord )
		outb( VIA_PCM_CAPTURE_FIFO_ENABLE, psStream->nIoBase + VIA_PCM_CAPTURE_FIFO );
	else
		outb( nPcmFmt, psStream->nIoBase + VIA_PCM_TYPE );
	outl( nPcmStop, psStream->nIoBase + VIA_PCM_STOP );
	psStream->nPcmStop = nPcmStop;
	psStream->nPcmFmt = nPcmFmt;
	
	
	/* Load SGD table */
	via_ac97_wait( psDriver, 0 );
	outl( psStream->nSgdPhysAddr, psStream->nIoBase + VIA_PCM_TABLE_ADDR );
	snooze( 20 );
	via_ac97_wait( psDriver, 0 );
	
	UNLOCK( psDriver->hLock );
	
	return( 0 );
}

static status_t via_create_buffers( VIAAudioDriver_s* psDriver, VIAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	/* Set fragments */
	psStream->nFragSize = VIA_MAX_FRAG_SIZE / 2;
	psStream->nFragNumber = VIA_MAX_FRAGS;
	
	/* Align fragment size */
	psStream->nFragSize /= 2 * nChannels;
	psStream->nFragSize *= 2 * nChannels;
	
	int nShift = 0;
	while( psStream->nFragSize )
	{
		psStream->nFragSize >>= 1;
		nShift++;
	}
	psStream->nFragSize = 1 << nShift;
	
	printk( "Frag size %i Frag number %i\n", psStream->nFragSize, psStream->nFragNumber );
	
	/* Delete areas */
	if( psStream->hBufArea > -1 )
		delete_area( psStream->hBufArea );
	if( psStream->hSgdArea > -1 )
		delete_area( psStream->hSgdArea );
	
	/* Create sgd table */
	uint8* pAddress = NULL;
	psStream->hSgdArea = create_area( "via_dma", (void**)&pAddress, PAGE_SIZE, PAGE_SIZE, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	psStream->pasSgTable = (volatile struct via_sgd_table*)pAddress;
	memset( pAddress, 0, sizeof( struct via_sgd_table ) * psStream->nFragNumber );
	
	/* Calculate number of pages */
	psStream->nPageNumber = ( psStream->nFragSize * psStream->nFragNumber ) / PAGE_SIZE +
		( ( ( psStream->nFragSize * psStream->nFragNumber ) % PAGE_SIZE ) ? 1 : 0 );
		
	printk( "Need %i pages\n", (int)psStream->nPageNumber );
	
	/* Create audio buffer */
	psStream->pBuffer = NULL;
	psStream->hBufArea = create_area( "via_buf", (void**)&psStream->pBuffer, PAGE_SIZE * psStream->nPageNumber, PAGE_SIZE * psStream->nPageNumber, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	memset( psStream->pBuffer, 0,  PAGE_SIZE * psStream->nPageNumber );
	uint32 nPhysAddr;
	get_area_physical_address( psStream->hBufArea, &nPhysAddr );
	printk("Buf: Address 0x%x -> 0x%x\n", (uint)psStream->pBuffer, (uint)nPhysAddr );
		
	/* Fill sgd table */
	uint i;
	for( i = 0; i < psStream->nFragNumber; i++ )
	{
		psStream->pasSgTable[i].count = ( psStream->nFragSize | VIA_FLAG );
		psStream->pasSgTable[i].addr = nPhysAddr;
		printk( "SG: %i -> 0x%x\n", i, (uint)nPhysAddr );
		nPhysAddr += psStream->nFragSize;
	}
	psStream->pasSgTable[psStream->nFragNumber - 1].count = ( psStream->nFragSize | VIA_EOL );
	
	/* Get SGD address */
	get_area_physical_address( psStream->hSgdArea, &psStream->nSgdPhysAddr );
	printk("SG: Address 0x%x -> 0x%x\n", (uint)pAddress, (uint)psStream->nSgdPhysAddr );
	
	generic_stream_clear( &psStream->sGeneric );
	
	return( 0 );
}

uint32 via_get_frag_size( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	return( psStream->nFragSize );
}

uint32 via_get_frag_number( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	return( psStream->nFragNumber );
}

uint8* via_get_buffer( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	return( psStream->pBuffer );
}

uint32 via_get_current_position( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	return( inl( psStream->nIoBase + VIA_PCM_BLOCK_COUNT) & 0xffffff );
}

status_t via_start( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	if( !psStream->bIsActive )
	{
		psStream->bIsActive = true;
		outb ( VIA_SGD_START | VIA_SGD_FLAG | VIA_SGD_EOL 
		| VIA_SGD_STOP | VIA_SGD_AUTOSTART, psStream->nIoBase + VIA_PCM_CONTROL );
	}
	return( 0 );
}

void via_stop( void* pDriverData )
{
	VIAStream_s* psStream = pDriverData;
	
	
	if( inb( psStream->nIoBase + VIA_PCM_STATUS ) & VIA_SGD_ACTIVE )
		outb( VIA_SGD_TERMINATE, psStream->nIoBase + VIA_PCM_CONTROL );
	psStream->bIsActive = false;
}


static void via_clear( VIAAudioDriver_s* psDriver, VIAStream_s* psStream )
{
	LOCK( psDriver->hLock );
	
	via_stop( psStream );
	
	
	/* Reload the state */
	outb( 0, psStream->nIoBase + VIA_PCM_CONTROL );
	outb( VIA_SGD_FLAG | VIA_SGD_EOL, psStream->nIoBase + VIA_PCM_STATUS );
	snooze(20);
	outb( 0, psStream->nIoBase + VIA_PCM_TYPE );
	if( psStream->bIsRecord )
		outb( VIA_PCM_CAPTURE_FIFO_ENABLE, psStream->nIoBase + VIA_PCM_CAPTURE_FIFO );
	else
		outb( psStream->nPcmFmt, psStream->nIoBase + VIA_PCM_TYPE );
	outl( psStream->nPcmStop, psStream->nIoBase + VIA_PCM_STOP );
	snooze(20);
	via_ac97_wait( psDriver, 0 );
	outl( psStream->nSgdPhysAddr, psStream->nIoBase + VIA_PCM_TABLE_ADDR );
	snooze( 20 );
	via_ac97_wait( psDriver, 0 );
	UNLOCK( psDriver->hLock );
	
	generic_stream_clear( &psStream->sGeneric );
}



status_t via_open( void *pNode, uint32 nFlags, void **pCookie )
{
	VIAStream_s* psStream = pNode;
	
	/* Check what open mode is requested */
	if( nFlags & O_RDWR )
	{
		/* This means that the application wants to read/write from the device
		and not just use the ioctl commands for the mixer */
		if( psStream->bIsLocked )
		{
			printk( "Error: via_open() stream already used\n" );
			return( -EBUSY );
		}
		psStream->bIsLocked = true;
		psStream->hBufArea = -1;
		psStream->hSgdArea = -1;
		generic_stream_init( &psStream->sGeneric, psStream->bIsRecord );
		*((int*)pCookie) = 1;
		printk( "via_open() for writing\n" );
	}
	else
	{
		*((int*)pCookie) = 0;
		printk( "via_open() for reading\n" );
	}
	
	return( 0 );		
}

status_t via_close( void *pNode, void *pCookie )
{
	VIAStream_s* psStream = pNode;
	
	
	if( (int)pCookie == 1 )
	{
		/* Stop playback */
		via_stop( psStream );
		
		
		outb( 0, psStream->nIoBase + VIA_PCM_CONTROL );
		outb( VIA_SGD_FLAG | VIA_SGD_EOL, psStream->nIoBase + VIA_PCM_STATUS );
		snooze(20);
		outb( 0, psStream->nIoBase + VIA_PCM_TYPE );

		/* Delete areas */
		if( psStream->hBufArea > -1 )
			delete_area( psStream->hBufArea );
		if( psStream->hSgdArea > -1 )
			delete_area( psStream->hSgdArea );
			
		psStream->hBufArea = psStream->hSgdArea = -1;
		
			
		generic_stream_free( &psStream->sGeneric );
		psStream->bIsLocked = false;
		printk( "via_close() for writing\n");
	} else
		printk( "via_close() for reading\n");
		
		
	return( 0 );
}

int	via_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	VIAStream_s* psStream = pNode;
	if( !psStream->bIsRecord || !psStream->bIsLocked )
		return( -EINVAL );
	return( generic_stream_read( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

int	via_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	VIAStream_s* psStream = pNode;
	if( psStream->bIsRecord || !psStream->bIsLocked )
		return( -EINVAL );
	return( generic_stream_write( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

status_t via_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	VIAStream_s* psStream = pNode;
	VIAAudioDriver_s* psDriver = psStream->psDriver;
	switch( nCommand )
	{
		case IOCTL_GET_USERSPACE_DRIVER:
			memcpy_to_user( pArgs, "ac97_audio.so", strlen( "ac97_audio.so" ) );
		break;
		case AC97_GET_CARD_INFO:
		{
			AC97CardInfo_s sInfo;
			sInfo.nDeviceID = psDriver->nDeviceID;
			sInfo.nDeviceHandle = psDriver->sPCI.nHandle;
			strcpy( sInfo.zName, psStream->zName );
			sInfo.bRecord = psStream->bIsRecord;
			sInfo.bSPDIF = psStream->bIsSPDIF;
			if( psStream->bIsRecord )
				sInfo.nMaxChannels = 2;
			else
				sInfo.nMaxChannels = 6;
			sInfo.nMaxSampleRate = 48000;
			sInfo.nMaxResolution = 16;
			memcpy_to_user( pArgs, &sInfo, sizeof( sInfo ) );
		}
		break;
		case AC97_SET_FORMAT:
		{
			AC97Format_s sFormat;
			int nError;
			memcpy_from_user( &sFormat, pArgs, sizeof( sFormat ) );
			
			nError = via_check_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			nError = via_create_buffers( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			return( via_set_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution ) );
		};
		break;
		case AC97_GET_BUFFER_SIZE:
		{
			//printk("GetBufferSize!\n");
			uint32 nBufferSize = psStream->nFragSize * psStream->nFragNumber;
			memcpy_to_user( pArgs, &nBufferSize, sizeof( uint32 ) );
		}
		break;
		case AC97_GET_DELAY:
		{
			//printk( "Get Delay\n");
			uint32 nValue = generic_stream_get_delay( &psStream->sGeneric );
			memcpy_to_user( pArgs, &nValue, sizeof( uint32 ) );
		}
		break;
		case AC97_CLEAR:
		{
			via_clear( psDriver, psStream );
		}
		break;
		default:
			return( ac97_ioctl( &psDriver->sAC97, nCommand, pArgs, bFromKernel ) );
	}
	return( 0 );
}


DeviceOperations_s g_sOperations = {
	via_open,
	via_close,
	via_ioctl,
	via_read,
	via_write
};


static void via_init_stream( VIAAudioDriver_s* psDriver, int nChannel, char* pzName, int nReg, uint32 nIRQMask, bool bSPDIF, bool bRecord )
{
	/* Initialize stream structure */
	psDriver->sStream[nChannel].psDriver = psDriver;
	psDriver->sStream[nChannel].nBaseAddr = psDriver->nBaseAddr;
	psDriver->sStream[nChannel].nIoBase = psDriver->nBaseAddr + nReg;
	psDriver->sStream[nChannel].hSgdArea = -1;
	psDriver->sStream[nChannel].nIRQMask = nIRQMask;
	psDriver->sStream[nChannel].bIsSPDIF = bSPDIF;
	psDriver->sStream[nChannel].bIsRecord = bRecord;
	psDriver->sStream[nChannel].nChannels = 2;
	psDriver->sStream[nChannel].nSampleRate = 48000;
	sprintf( psDriver->sStream[nChannel].zName, "%s %s", psDriver->zName, pzName );
	
	/* Create generic output interface */
	GenericStream_s* psGeneric = &psDriver->sStream[nChannel].sGeneric;
	psGeneric->pfGetFragSize = via_get_frag_size;
	psGeneric->pfGetFragNumber = via_get_frag_number;
	psGeneric->pfGetBuffer = via_get_buffer;
	psGeneric->pfGetCurrentPosition = via_get_current_position;
	psGeneric->pfStart = via_start;
	psGeneric->pfStop = via_stop;
	psGeneric->pDriverData = &psDriver->sStream[nChannel];
	
	/* Create node */
	char zNodePath[PATH_MAX];
	sprintf( zNodePath, "audio/via_%i_%s", psDriver->sPCI.nHandle, pzName );

	if( create_device_node( psDriver->nDeviceID,  psDriver->sPCI.nHandle, zNodePath, &g_sOperations, &psDriver->sStream[nChannel] ) < 0 )
	{
		printk( "Error: Failed to create device node %s\n", zNodePath );
	}
	
	psDriver->nNumStreams++;
	
	printk( "Add stream %s @ 0x%x\n", pzName, (uint)psDriver->sStream[nChannel].nIoBase );
}

/* Interrupts */
void via_stream_interrupt( VIAAudioDriver_s* psDriver, VIAStream_s* psStream )
{
	generic_stream_fragment_processed( &psStream->sGeneric );
}

int via_interrupt( int irq, void *dev_id, SysCallRegs_s *regs )
{
	VIAAudioDriver_s* psDriver = dev_id;
	
	/* Check streams */
	int i = 0;
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		VIAStream_s* psStream = &psDriver->sStream[i];
		uint8 nStatus = inb( psStream->nIoBase ) & ( VIA_SGD_FLAG | VIA_SGD_EOL | VIA_SGD_STOPPED );
	
		if( !nStatus )
			continue;
			
		outb( nStatus, psStream->nIoBase );
			
		via_stream_interrupt( psDriver, psStream );
	}
	return( 0 );
}

void via_init_spdif( VIAAudioDriver_s* psDriver )
{
	
	uint8 nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_SPDIF_CTRL, 1 );
	nVal = ( nVal & VIA_SPDIF_SLOT_MASK );
	nVal |= VIA_SPDIF_DX3;
	g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, VIA_SPDIF_CTRL, 1, nVal );
		
}
	

static status_t via_init( int nDeviceID, PCI_Info_s sPCI, char* zName )
{
	printk( "via_init()\n" );
	VIAAudioDriver_s* psDriver;
	
	psDriver = kmalloc( sizeof( VIAAudioDriver_s ), MEMF_KERNEL | MEMF_CLEAR );
	if( psDriver == NULL )
	{
		printk( "Out of memory\n" );
		return( -ENOMEM );
	}
	
	psDriver->nDeviceID = nDeviceID;
	psDriver->sPCI = sPCI;
	strcpy( psDriver->zName, zName );
	
	printk( "VendorID: 0x%x DeviceID: 0x%x\n", (uint)sPCI.nVendorID, (uint)sPCI.nDeviceID );
	
	/* Create structure */
	
	
	/* I/O */
	psDriver->nBaseAddr = sPCI.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
	printk( "IO @ 0x%x\n", (uint)psDriver->nBaseAddr );

	
	
	/* Reset AC97 Codec */
	if( via_ac97_reset( psDriver ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}
		
	/* Calculate number of codecs */
	int nCodecs = 1;
	

	/* Initialize AC97 Codec */
	psDriver->sAC97.pfWait = via_ac97_wait;
	psDriver->sAC97.pfRead = via_ac97_read;
	psDriver->sAC97.pfWrite = via_ac97_write;
	psDriver->sAC97.pDriverData = psDriver;
	
	char zAC97ID[255];
	sprintf( zAC97ID, "via_%i", sPCI.nHandle );
	
	if( ac97_initialize( &psDriver->sAC97, zAC97ID, nCodecs ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}

	/* Init SPDIF */
	via_init_spdif( psDriver );
	
	/* Init streams */
	via_init_stream( psDriver, 0, "pcm", VIA_BASE0_PCM_OUT_CHAN, 0, false, false );
	via_init_stream( psDriver, 1, "input", 0x60, 0, false, true );
	
	if ( claim_device( nDeviceID, sPCI.nHandle, zName, DEVICE_AUDIO ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}
	
	set_device_data( sPCI.nHandle, psDriver );
		
	/* Request irq */
	psDriver->nIRQHandle = request_irq( sPCI.u.h0.nInterruptLine, via_interrupt, NULL, SA_SHIRQ, "via_audio", psDriver );	
	
	/* Create lock */
	psDriver->hLock = create_semaphore( "via_lock", 1, 0 );
	
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
	
	int nNumDevices = sizeof( g_sDevices ) / sizeof( struct audio_device );
	int nDeviceNum;
	PCI_Info_s sInfo;
	int nPCINum;

	bool bDevFound = false;

	/* Get PCI busmanager */
	g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if ( g_psBus == NULL )
		return ( -ENODEV );

	/* Look for the device */
	for ( nPCINum = 0; g_psBus->get_pci_info( &sInfo, nPCINum ) == 0; ++nPCINum )
	{
		for ( nDeviceNum = 0; nDeviceNum < nNumDevices; ++nDeviceNum )
		{
			/* Compare vendor and device id */
			if ( sInfo.nVendorID == g_sDevices[nDeviceNum].nVendorID && sInfo.nDeviceID == g_sDevices[nDeviceNum].nDeviceID )
			{
				char zTemp[255];
				sprintf( zTemp, "%s %s", g_sDevices[nDeviceNum].zVendorName, g_sDevices[nDeviceNum].zDeviceName );
				/* Init card */
				if( via_init( nDeviceID, sInfo, zTemp ) == 0 )
					bDevFound = true;
			}
		}
	}

	if ( !bDevFound )
	{
		disable_device( nDeviceID );
		return ( -ENODEV );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	return ( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_suspend( int nDeviceID, int nDeviceHandle, void* pData )
{
	VIAAudioDriver_s* psDriver = pData;
	
	/* Stop playback */
	int i;
	
	printk( "Suspend %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		VIAStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Stop playback on %s\n", psStream->zName );
			via_stop( psStream );
			psStream->bIsActive = true; /* Remember to start playback after resume */
		}
	}
	
	ac97_suspend( &psDriver->sAC97 );
	
	return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_resume( int nDeviceID, int nDeviceHandle, void* pData )
{
	VIAAudioDriver_s* psDriver = pData;
	
	/* Restart playback */
	int i;
	
	printk( "Resume %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	/* Reset AC97 */
	via_ac97_reset( psDriver );
	
	/* Resume AC97 */
	ac97_resume( &psDriver->sAC97 );
	
	/* Init SPDIF */
	via_init_spdif( psDriver );
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		VIAStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Restarting playback on %s\n", psStream->zName );
			psStream->bIsActive = false;
			via_set_format( psDriver, psStream, psStream->nChannels, psStream->nSampleRate, 16 );
			via_start( psStream );
		}
	}
	
	
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void device_release( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	VIAAudioDriver_s* psDriver = pPrivateData;
	/* Release IRQ */
	release_irq( psDriver->sPCI.u.h0.nInterruptLine, psDriver->nIRQHandle );
	/* Release device */
	release_device( psDriver->sPCI.nHandle );
	kfree( psDriver );
	/* The devices nodes are deleted by the system */
}






































