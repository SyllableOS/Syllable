/*
 *	Intel ICH audio driver
 *	Copyright (C) 2006 Arno Klenke
 *  based on:
 *  ALSA driver for Intel ICH (i8x0) chipsets
 *
 *	Copyright (c) 2000 Jaroslav Kysela <perex@suse.cz>
 *
 *
 *   This code also contains alpha support for SiS 735 chipsets provided
 *   by Mike Pieper <mptei@users.sourceforge.net>. We have no datasheet
 *   for SiS735, so the code is not fully functional.
 *
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
#include "i8xx.h"

static PCI_bus_s *g_psBus;

struct audio_device
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
};

struct audio_device g_sDevices[] = {
	{0x8086, 0x2415, "Intel", "82801AA"},
	{0x8086, 0x2425, "Intel", "82901AB"},
	{0x8086, 0x2445, "Intel", "82801BA"},
	{0x8086, 0x2485, "Intel", "ICH3"},
	{0x8086, 0x24c5, "Intel", "ICH4"},
	{0x8086, 0x24d5, "Intel", "ICH5"},
	{0x8086, 0x25a6, "Intel", "ESB"},
	{0x8086, 0x266e, "Intel", "ICH6"},
	{0x8086, 0x27de, "Intel", "ICH7"},
	{0x8086, 0x2698, "Intel", "ESB2"},
	{0x8086, 0x7195, "Intel", "440MX"},
	{0x1039, 0x7012, "SiS", "SI7012"},
	{0x10de, 0x01b1, "nVIDIA", "nForce"},
	{0x10de, 0x003a, "nVIDIA", "MCP04"},
	{0x10de, 0x006a, "nVIDIA", "nForce 2"},	
	{0x10de, 0x0059, "nVIDIA", "CK804"},
	{0x10de, 0x008a, "nVIDIA", "CK8"},
	{0x10de, 0x00da, "nVIDIA", "nForce 3"},
	{0x10de, 0x00ea, "nVIDIA", "CK8S"},	
	{0x1022, 0x746d, "AMD", "8111"},
	{0x1022, 0x7445, "AMD", "768"}
};

/* IO methods */
void io_outb( ICHAudioDriver_s* psDriver, uint8 nVal, uint32 nReg )
{
	if( psDriver->bMMIO )
		writeb( nVal, nReg );
	else
		outb( nVal, nReg );
}

void io_outw( ICHAudioDriver_s* psDriver, uint16 nVal, uint32 nReg )
{
	if( psDriver->bMMIO )
		writew( nVal, nReg );
	else
		outw( nVal, nReg );
}

void io_outl( ICHAudioDriver_s* psDriver, uint32 nVal, uint32 nReg )
{
	if( psDriver->bMMIO )
		writel( nVal, nReg );
	else
		outl( nVal, nReg );
}

uint8 io_inb( ICHAudioDriver_s* psDriver, uint32 nReg )
{
	if( psDriver->bMMIO )
		return( readb( nReg ) );
	else
		return( inb( nReg ) );
}

uint16 io_inw( ICHAudioDriver_s* psDriver, uint32 nReg )
{
	if( psDriver->bMMIO )
		return( readw( nReg ) );
	else
		return( inw( nReg ) );
}

uint32 io_inl( ICHAudioDriver_s* psDriver, uint32 nReg )
{
	if( psDriver->bMMIO )
		return( readl( nReg ) );
	else
		return( inl( nReg ) );
}

/* AC97 functions */
status_t ich_ac97_wait( void* pData, int nCodec )
{
	snooze( 10 );
	return( 0 );
}

status_t ich_ac97_write( void* pData, int nCodec, uint8 nReg, uint16 nVal )
{
	ICHAudioDriver_s* psDriver = ( ICHAudioDriver_s* )pData;
	int nCount = 100;
	
	
	LOCK( psDriver->hLock );
	
	while( nCount-- && ( io_inb( psDriver, psDriver->nBaseAddr + ICH_REG_ACC_SEMA ) & ICH_CAS ) )
	{
		snooze( 1 );
	}
	
	io_outw( psDriver, nVal, psDriver->nAC97BaseAddr + nReg + nCodec * 0x80 );
	UNLOCK( psDriver->hLock );
	return( 0 );
}

uint16 ich_ac97_read( void* pData, int nCodec, uint8 nReg )
{
	ICHAudioDriver_s* psDriver = ( ICHAudioDriver_s* )pData;
	int nCount = 100;
	
	LOCK( psDriver->hLock );
	
	while( nCount-- && ( io_inb( psDriver, psDriver->nBaseAddr + ICH_REG_ACC_SEMA ) & ICH_CAS ) )
	{
		snooze( 1 );
	}
	
	uint16 nVal = io_inw( psDriver, psDriver->nAC97BaseAddr + nReg + nCodec * 0x80 );
	uint32 nTemp;
	if( ( nTemp = io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA ) ) & ICH_RCS )
	{
		io_outl( psDriver, nTemp & ~( ICH_SRI|ICH_PRI|ICH_TRI|ICH_GSCI ), psDriver->nBaseAddr + ICH_REG_GLOB_STA );
		printk( "AC97 Read timeout!\n" );
		nVal = 0xffff;
	}
	
	
	UNLOCK( psDriver->hLock );
	
	return( nVal );
}


status_t ich_ac97_reset( ICHAudioDriver_s* psDriver )
{
	/* Clear status */
	unsigned int nStatus = ICH_RCS | ICH_MCINT | ICH_POINT | ICH_PIINT;
	if( psDriver->bNVIDIA )
		nStatus |= ICH_NVSPINT;
		
	unsigned int nCnt = io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA );
	io_outl( psDriver, nCnt & nStatus, psDriver->nBaseAddr + ICH_REG_GLOB_STA );
	
	/* Enable ACLink */
	nCnt = io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_CNT );
	nCnt &= ~( ICH_ACLINK | ICH_PCM_246_MASK);
	nCnt |= ( ( nCnt & ICH_AC97COLD ) == 0 ) ? ICH_AC97COLD : ICH_AC97WARM;
	io_outl( psDriver, nCnt, psDriver->nBaseAddr + ICH_REG_GLOB_CNT );
	
	int nTries = 1000;
	/* Wait until the reset has finished */
	do
	{
		snooze( 1000 );
	} while( --nTries > 0 && ( io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_CNT ) & ICH_AC97WARM ) != 0 );
	if( nTries <= 0 )
		printk( "AC97 Reset failed: card still not reset (GLOB_CNT 0x%x)\n", (uint)io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_CNT ) );
	/* Wait for codecs */
	nTries = 1000;
	do
	{
		snooze( 1000 );
	} while( --nTries > 0 && ( io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA ) & (ICH_PCR | ICH_SCR | ICH_TCR ) ) == 0 );
	if( nTries <= 0 )
		printk( "AC97 Reset failed: codecs still not ready (GLOB_CNT 0x%x)\n", (uint)io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_CNT ) );
	
	/* Unmute SiS */
	if( psDriver->bSIS )
		io_outw( psDriver, io_inw( psDriver, psDriver->nBaseAddr + 0x4c ) | 1, psDriver->nBaseAddr + 0x4c );
	
	printk( "AC97 Reset finished\n" );
	

	return( 0 );
}


/* Streams */

static status_t ich_check_format( ICHAudioDriver_s* psDriver, ICHStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	/* Resolution */
	if( nResolution != 16 )
		return( -1 );
	
	/* Check if we support the rate */
	if( ( !ac97_supports_vra( &psDriver->sAC97, 0 ) || psDriver->bNVIDIA || psStream->bIsSPDIF ) && nSampleRate != 48000  )
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

static status_t ich_set_format( ICHAudioDriver_s* psDriver, ICHStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	
	
	LOCK( psDriver->hLock );
	
	/* Save data */
	psStream->nChannels = nChannels;
	psStream->nSampleRate = nSampleRate;
	
	/* Reset channel  */
	io_outb( psDriver, 0, psStream->nIoBase + ICH_REG_CR );
	io_outb( psDriver, ICH_RESETREGS, psStream->nIoBase + ICH_REG_CR );
	
	
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
	
	/* Set format */
	if( !psStream->bIsRecord )
	{
		if( psDriver->bSIS )
		{
			psStream->nCnt = io_inl( psDriver, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
			psStream->nCnt &= ~( ICH_SIS_PCM_246_MASK );
			if( nChannels == 4 )
				psStream->nCnt |= ICH_SIS_PCM_4;
			else if( nChannels == 6 )
				psStream->nCnt |= ICH_SIS_PCM_6;
			io_outl( psDriver, psStream->nCnt, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
		}
		else
		{
			psStream->nCnt = io_inl( psDriver, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
			psStream->nCnt &= ~( ICH_PCM_246_MASK | ICH_PCM_20BIT );
			if( nChannels == 4 )
				psStream->nCnt |= ICH_PCM_4;
			else if( nChannels == 6 )
				psStream->nCnt |= ICH_PCM_6;
			if( psDriver->bNVIDIA )
			{
				/* nVIDIA workaround */
				if( psStream->nCnt & ICH_PCM_246_MASK )
				{
					io_outl( psDriver, psStream->nCnt & ~ICH_PCM_246_MASK, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
					snooze( 50000 );
				}
			}
			io_outl( psDriver, psStream->nCnt, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
		}
	}
	
	
	
	/* Set LVI */
	io_outb( psDriver, psStream->nLVI, psStream->nIoBase + ICH_REG_LVI );
	
	
	/* Set CIV */
	io_outb( psDriver, psStream->nCIV, psStream->nIoBase + ICH_REG_CIV );
	
	/* Clear status */
	if( psDriver->bSIS )
		io_outb( psDriver, ICH_FIFOE | ICH_BCIS | ICH_LVBCI, psStream->nIoBase + ICH_REG_PICB );
	else
		io_outb( psDriver, ICH_FIFOE | ICH_BCIS | ICH_LVBCI, psStream->nIoBase + ICH_REG_SR );
	
	/* Load SGD table */
	io_outl( psDriver, psStream->nSgdPhysAddr, psStream->nIoBase + ICH_REG_BDBAR );
	
	
	UNLOCK( psDriver->hLock );
	
	
	return( 0 );
}

static status_t ich_create_buffers( ICHAudioDriver_s* psDriver, ICHStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	/* Set fragments */
	psStream->nFragSize = ICH_MAX_FRAG_SIZE / 2;
	psStream->nFragNumber = ICH_MAX_FRAGS;
	
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
	psStream->hSgdArea = create_area( "ich_dma", (void**)&pAddress, PAGE_SIZE, PAGE_SIZE, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	psStream->pasSgTable = (volatile struct ich_sgd_table*)pAddress;
	memset( pAddress, 0, sizeof( struct ich_sgd_table ) * psStream->nFragNumber );
	
	/* Calculate number of pages */
	psStream->nPageNumber = ( psStream->nFragSize * psStream->nFragNumber ) / PAGE_SIZE +
		( ( ( psStream->nFragSize * psStream->nFragNumber ) % PAGE_SIZE ) ? 1 : 0 );
		
	printk( "Need %i pages\n", (int)psStream->nPageNumber );
	
	/* Create audio buffer */
	psStream->pBuffer = NULL;
	psStream->hBufArea = create_area( "ich_buf", (void**)&psStream->pBuffer, PAGE_SIZE * psStream->nPageNumber, PAGE_SIZE * psStream->nPageNumber, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	memset( psStream->pBuffer, 0,  PAGE_SIZE * psStream->nPageNumber );
	uint32 nPhysAddr;
	get_area_physical_address( psStream->hBufArea, &nPhysAddr );
	printk("Buf: Address 0x%x -> 0x%x\n", (uint)psStream->pBuffer, (uint)nPhysAddr );
		
	/* Fill sgd table */
	uint i;
	for( i = 0; i < psStream->nFragNumber; i++ )
	{
		if( psDriver->bSIS )
		{
			psStream->pasSgTable[i].count = ( psStream->nFragSize | ICH_INT );
		}
		else
			psStream->pasSgTable[i].count = ( ( psStream->nFragSize >> 1 ) | ICH_INT );
		psStream->pasSgTable[i].addr = nPhysAddr;
		printk( "SG: %i -> 0x%x\n", i, (uint)nPhysAddr );
		nPhysAddr += psStream->nFragSize;
	}
	
	/* Get SGD address */
	get_area_physical_address( psStream->hSgdArea, &psStream->nSgdPhysAddr );
	printk("SG: Address 0x%x -> 0x%x\n", (uint)pAddress, (uint)psStream->nSgdPhysAddr );
	
	psStream->nLVI = ICH_REG_LVI_MASK;
	psStream->nCIV = 0;
	
	generic_stream_clear( &psStream->sGeneric );
	
	return( 0 );
}

uint32 ich_get_frag_size( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	return( psStream->nFragSize );
}

uint32 ich_get_frag_number( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	return( psStream->nFragNumber );
}

uint8* ich_get_buffer( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	return( psStream->pBuffer );
}

uint32 ich_get_current_position( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	ICHAudioDriver_s* psDriver = psStream->psDriver;
	if( psDriver->bSIS )
		return( ich_get_frag_size( psStream ) - io_inw( psDriver, psStream->nIoBase + ICH_REG_SR ) );
	else
		return( ich_get_frag_size( psStream ) - ( io_inw( psDriver, psStream->nIoBase + ICH_REG_PICB ) << 1 ) );
}

status_t ich_start( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	ICHAudioDriver_s* psDriver = psStream->psDriver;
	if( !psStream->bIsActive )
	{
		psStream->bIsActive = true;
		io_outb( psDriver, ICH_IOCE | ICH_STARTBM, psStream->nIoBase + ICH_REG_CR );
		//printk( "Start!\n" );
	}
	return( 0 );
}

void ich_stop( void* pDriverData )
{
	ICHStream_s* psStream = pDriverData;
	ICHAudioDriver_s* psDriver = psStream->psDriver;
	if( psStream->bIsActive )
	{
		psStream->bIsActive = false;
		io_outb( psDriver, 0, psStream->nIoBase + ICH_REG_CR );
		if( psDriver->bSIS )
			while( !( io_inb( psDriver, psStream->nIoBase + ICH_REG_PICB ) & ICH_DCH ) ) {}
		else
			while( !( io_inb( psDriver, psStream->nIoBase + ICH_REG_SR ) & ICH_DCH ) ) {}
		//printk( "stoped!\n" );
	}
}


static void ich_clear( ICHAudioDriver_s* psDriver, ICHStream_s* psStream )
{
	LOCK( psDriver->hLock );
	
	ich_stop( psStream );
	
	
	/* Reload the state */
	io_outb( psDriver, 0, psStream->nIoBase + ICH_REG_CR );
	io_outb( psDriver, ICH_RESETREGS, psStream->nIoBase + ICH_REG_CR );
	if( !psStream->bIsRecord )
		io_outl( psDriver, psStream->nCnt, psStream->nBaseAddr + ICH_REG_GLOB_CNT );
	io_outl( psDriver, psStream->nSgdPhysAddr, psStream->nIoBase + ICH_REG_BDBAR );
	
	
	/* Set LVI */
	psStream->nLVI = ICH_REG_LVI_MASK;
	io_outb( psDriver, psStream->nLVI, psStream->nIoBase + ICH_REG_LVI );

	/* Set CIV */
	psStream->nCIV = 0;
	io_outb( psDriver, psStream->nCIV, psStream->nIoBase + ICH_REG_CIV );
	

	UNLOCK( psDriver->hLock );
	
	generic_stream_clear( &psStream->sGeneric );
}



status_t ich_open( void *pNode, uint32 nFlags, void **pCookie )
{
	ICHStream_s* psStream = pNode;
	
	
	/* Check what open mode is requested */
	if( nFlags & O_RDWR )
	{
		/* This means that the application wants to read/write from the device
		and not just use the ioctl commands for the mixer */
		if( psStream->bIsLocked )
		{
			printk( "Error: ich_open() stream already used\n" );
			return( -EBUSY );
		}
		psStream->bIsLocked = true;
		psStream->hBufArea = -1;
		psStream->hSgdArea = -1;
		generic_stream_init( &psStream->sGeneric, psStream->bIsRecord );
		*((int*)pCookie) = 1;
		printk( "ich_open() for writing\n" );
	}
	else
	{
		*((int*)pCookie) = 0;
		printk( "ich_open() for reading\n" );
	}
	
	
	return( 0 );		
}

status_t ich_close( void *pNode, void *pCookie )
{
	ICHStream_s* psStream = pNode;
	printk( "ich_close()\n");
	
	if( (int)pCookie == 1 )
	{
		/* Stop playback */
		ich_stop( psStream );
	
		io_outb( psStream->psDriver, 0, psStream->nIoBase + ICH_REG_CR );
		io_outb( psStream->psDriver, ICH_RESETREGS, psStream->nIoBase + ICH_REG_CR );
	
		/* Delete areas */
		if( psStream->hBufArea > -1 )
			delete_area( psStream->hBufArea );
		if( psStream->hSgdArea > -1 )
			delete_area( psStream->hSgdArea );
			
		psStream->hBufArea = psStream->hSgdArea = -1;
		
			
		generic_stream_free( &psStream->sGeneric );
		psStream->bIsLocked = false;
		printk( "ich_close() for writing\n");
	} else
		printk( "ich_close() for reading\n");
	return( 0 );
}


int	ich_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	ICHStream_s* psStream = pNode;
	if( !psStream->bIsRecord || !psStream->bIsLocked )
		return( -EINVAL );
	return( generic_stream_read( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

int	ich_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	ICHStream_s* psStream = pNode;
	return( generic_stream_write( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

status_t ich_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	ICHStream_s* psStream = pNode;
	ICHAudioDriver_s* psDriver = psStream->psDriver;
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
			sInfo.nMaxSampleRate = 48000; /* TODO: 96000 support */
			sInfo.nMaxResolution = 16; /* TODO: 24bit support */
			memcpy_to_user( pArgs, &sInfo, sizeof( sInfo ) );
		}
		break;
		case AC97_SET_FORMAT:
		{
			AC97Format_s sFormat;
			int nError;
			memcpy_from_user( &sFormat, pArgs, sizeof( sFormat ) );
			
			nError = ich_check_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			nError = ich_create_buffers( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			return( ich_set_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution ) );
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
			ich_clear( psDriver, psStream );
		}
		break;
		default:
			return( ac97_ioctl( &psDriver->sAC97, nCommand, pArgs, bFromKernel ) );
	}
	return( 0 );
}


DeviceOperations_s g_sOperations = {
	ich_open,
	ich_close,
	ich_ioctl,
	ich_read,
	ich_write
};


static void ich_init_stream( ICHAudioDriver_s* psDriver, int nChannel, char* pzName, int nReg, uint32 nIRQMask, bool bSPDIF, bool bRecord )
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
	psGeneric->pfGetFragSize = ich_get_frag_size;
	psGeneric->pfGetFragNumber = ich_get_frag_number;
	psGeneric->pfGetBuffer = ich_get_buffer;
	psGeneric->pfGetCurrentPosition = ich_get_current_position;
	psGeneric->pfStart = ich_start;
	psGeneric->pfStop = ich_stop;
	psGeneric->pDriverData = &psDriver->sStream[nChannel];
	
	/* Create node */
	char zNodePath[PATH_MAX];
	sprintf( zNodePath, "audio/ich_%i_%s", psDriver->sPCI.nHandle, pzName );

	if( create_device_node( psDriver->nDeviceID,  psDriver->sPCI.nHandle, zNodePath, &g_sOperations, &psDriver->sStream[nChannel] ) < 0 )
	{
		printk( "Error: Failed to create device node %s\n", zNodePath );
	}
	
	psDriver->nNumStreams++;
	
	printk( "Add stream %s @ 0x%x %i\n", pzName, (uint)psDriver->sStream[nChannel].nIoBase, bRecord );
}

/* Interrupts */
void ich_stream_interrupt( ICHAudioDriver_s* psDriver, ICHStream_s* psStream )
{
	uint8 nCIV = io_inb( psDriver, psStream->nIoBase + ICH_REG_CIV );
	
	//printk( "Got interrupt %x\n", psStream->nIoBase );
	
	/* Compare CIV with saved value */
	while( nCIV != psStream->nCIV )
	{
		/* Increase CIV and LVI */
		psStream->nCIV++;
		psStream->nCIV &= ICH_REG_LVI_MASK;

		psStream->nLVI++;
		psStream->nLVI &= ICH_REG_LVI_MASK;
		/* Tell the generic output */
		generic_stream_fragment_processed( &psStream->sGeneric );
	}	
	
	//printk( "CIV now at %i\n", psStream->nCIV );
	
	/* Update LVI */
	io_outb( psDriver, psStream->nLVI, psStream->nIoBase + ICH_REG_LVI );		
}

int ich_interrupt( int irq, void *dev_id, SysCallRegs_s *regs )
{
	ICHAudioDriver_s* psDriver = dev_id;
	uint8 nGlobalStatus;
	
	/* Read global status */
	nGlobalStatus = io_inb( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA );
	if( ( nGlobalStatus & 0x070000f0 ) == 0 )
		return( 0 );
	io_outb( psDriver, ( nGlobalStatus & 0x070000f0 ), psDriver->nBaseAddr + ICH_REG_GLOB_STA );
	
	//printk( "Got irq!\n" );
	
	/* Check streams */
	int i = 0;
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		ICHStream_s* psStream = &psDriver->sStream[i];
		if( !( nGlobalStatus & ( psStream->nIRQMask ) ) )
			continue;
		if( psDriver->bSIS )
		{
			uint8 nStatus = io_inb( psDriver, psStream->nIoBase + ICH_REG_PICB );
			io_outb( psDriver, nStatus & (ICH_FIFOE | ICH_BCIS | ICH_LVBCI), psStream->nIoBase + ICH_REG_PICB );
		} 
		else
		{
			uint8 nStatus = io_inb( psDriver, psStream->nIoBase + ICH_REG_SR );
			io_outb( psDriver, nStatus & (ICH_FIFOE | ICH_BCIS | ICH_LVBCI), psStream->nIoBase + ICH_REG_SR );
		}
		ich_stream_interrupt( psDriver, psStream );
	}
	return( 0 );
}

/* Set SPDIF output to slot 10/11 */
void ich_init_spdif( ICHAudioDriver_s* psDriver )
{
	/* Set Intel SPDIF */
	if( psDriver->bICH4 )
	{
		printk( "Setting SPDIF output to slot 10/11\n" );
		uint32 nVal = io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_CNT ) & ~ICH_PCM_SPDIF_MASK;
		nVal |= ICH_PCM_SPDIF_1011;
		io_outb( psDriver, nVal, psDriver->nBaseAddr + ICH_REG_GLOB_CNT );
		uint16 nVal16 = ich_ac97_read( psDriver, 0, AC97_EXTENDED_STATUS ) & ~AC97_EA_SPSA_SLOT_MASK;
		nVal16 |= AC97_EA_SPSA_10_11;
		ich_ac97_write( psDriver, 0, AC97_EXTENDED_STATUS, nVal16 );
	}
		
	/* Enable NVIDIA SPDIF */
	if( psDriver->bNVIDIA )
	{
		uint32 nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, 0x4c, 4 );
		nVal |= 0x1000000;
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, 0x4c, 4, nVal );
		printk( "NVIDIA SPDIF output enabled\n" );
	}
		
}
	

static status_t ich_init( int nDeviceID, PCI_Info_s sPCI, char* zName )
{
	printk( "ich_init()\n" );
	ICHAudioDriver_s* psDriver;
	
	psDriver = kmalloc( sizeof( ICHAudioDriver_s ), MEMF_KERNEL | MEMF_CLEAR );
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
	
	/* Enable register access */
	if( !( sPCI.u.h0.nBase2 & PCI_ADDRESS_SPACE ) &&
		( sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK ) &&
		!( sPCI.u.h0.nBase3 & PCI_ADDRESS_SPACE ) &&
		( sPCI.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK ) )
	{
		/* MMIO */
		psDriver->bMMIO = true;
		void* pAddr = NULL;
		
		printk( "MMIO IO @ 0x%x AC97 @ 0x%x\n", sPCI.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK, sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK );
		
		/* Remap registers. Be careful with page alignment */
		psDriver->hAC97Area = create_area( "ich_ac97_mmio", &pAddr, PAGE_SIZE, PAGE_SIZE, AREA_KERNEL | AREA_ANY_ADDRESS, AREA_NO_LOCK );
		remap_area( psDriver->hAC97Area, (void*)( ( sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) );
		psDriver->nAC97BaseAddr = (uint32)pAddr + ( ( sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK ) - ( ( sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) );
		
		pAddr = NULL;
		psDriver->hBaseArea = create_area( "ich_mmio", &pAddr, PAGE_SIZE, PAGE_SIZE, AREA_KERNEL | AREA_ANY_ADDRESS, AREA_NO_LOCK );
		remap_area( psDriver->hBaseArea, (void*)( ( sPCI.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) ); 
		psDriver->nBaseAddr = (uint32)pAddr + ( ( sPCI.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK ) - ( ( sPCI.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) );
	}
	else
	{
		/* I/O */
		psDriver->nAC97BaseAddr = sPCI.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
		psDriver->nBaseAddr = sPCI.u.h0.nBase1 & PCI_ADDRESS_IO_MASK;
		printk( "IO @ 0x%x AC97 @ 0x%x\n", (uint)psDriver->nBaseAddr, (uint)psDriver->nAC97BaseAddr );
	}
	
	
	/* SiS has some registers swapped */
	if( sPCI.nVendorID == 0x1039 )
		psDriver->bSIS = true;
	
	/* nVIDIA has special spdif registers */
	if( sPCI.nVendorID == 0x10de )
		psDriver->bNVIDIA = true;
		
	/* Special features for ICH >=4 */
	if( sPCI.nVendorID == 0x8086 && sPCI.nDeviceID >= 0x24c5 )
		psDriver->bICH4 = true;

	/* Reset AC97 Codec */
	if( ich_ac97_reset( psDriver ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}
		
	/* Calculate number of codecs */
	int nCodecs = 1;
	if( io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA ) & ICH_SCR )
		nCodecs++;
	if( psDriver->bICH4 && io_inl( psDriver, psDriver->nBaseAddr + ICH_REG_GLOB_STA ) & ICH_TRI )
		nCodecs++;
		
	
	/* Create lock */
	psDriver->hLock = create_semaphore( "ich_lock", 1, SEM_RECURSIVE );
	

	/* Initialize AC97 Codec */
	psDriver->sAC97.pfWait = ich_ac97_wait;
	psDriver->sAC97.pfRead = ich_ac97_read;
	psDriver->sAC97.pfWrite = ich_ac97_write;
	psDriver->sAC97.pDriverData = psDriver;
	
	char zAC97ID[255];
	sprintf( zAC97ID, "intel_%i", sPCI.nHandle );
	
	if( ac97_initialize( &psDriver->sAC97, zAC97ID, nCodecs ) != 0 )
	{
		delete_semaphore( psDriver->hLock );
		kfree( psDriver );
		return( -EIO );
	}

	/* Init SPDIF */
	ich_init_spdif( psDriver );
	
	/* Init streams */
	ich_init_stream( psDriver, 0, "pcm", ICH_REG_BASE_PCMOUT, ICH_POINT, false, false );
	if( psDriver->bNVIDIA )
		ich_init_stream( psDriver, 1, "spdif", ICH_REG_BASE_NV_SPDIF, ICH_NVSPINT, true, false );
	if( psDriver->bICH4 )
		ich_init_stream( psDriver, 1, "spdif", ICH_REG_BASE_SPDIF, ICH_SPINT, true, false );
	ich_init_stream( psDriver, 2, "pcmin", 0, ICH_PIINT, false, true );
	if ( claim_device( nDeviceID, sPCI.nHandle, zName, DEVICE_AUDIO ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}
	
	
	set_device_data( sPCI.nHandle, psDriver );
		
	/* Request irq */
	psDriver->nIRQHandle = request_irq( sPCI.u.h0.nInterruptLine, ich_interrupt, NULL, SA_SHIRQ, "ich_audio", psDriver );	
	
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
				if( ich_init( nDeviceID, sInfo, zTemp ) == 0 )
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
	ICHAudioDriver_s* psDriver = pData;
	
	/* Stop playback */
	int i;
	
	printk( "Suspend %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		ICHStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Stop playback on %s\n", psStream->zName );
			ich_stop( psStream );
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
	ICHAudioDriver_s* psDriver = pData;
	
	/* Restart playback */
	int i;
	
	printk( "Resume %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	/* Reset AC97 */
	ich_ac97_reset( psDriver );
	
	/* Resume AC97 */
	ac97_resume( &psDriver->sAC97 );
	
	/* Init SPDIF */
	ich_init_spdif( psDriver );
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		ICHStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Restarting playback on %s\n", psStream->zName );
			psStream->bIsActive = false;
			ich_set_format( psDriver, psStream, psStream->nChannels, psStream->nSampleRate, 16 );
			ich_start( psStream );
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
	ICHAudioDriver_s* psDriver = pPrivateData;
	/* Release IRQ */
	release_irq( psDriver->sPCI.u.h0.nInterruptLine, psDriver->nIRQHandle );
	/* Release device */
	release_device( psDriver->sPCI.nHandle );
	kfree( psDriver );
	/* The devices nodes are deleted by the system */
}














































