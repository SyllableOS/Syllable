/*
 *
 *  hda.c - Intel HD Audio driver. Derived from the hda alsa driver
 * 
 *	Copyright(c) 2006 Arno Klenke
 *
 *  Copyright(c) 2004 Intel Corporation. All rights reserved.
 *
 *  Copyright (c) 2004 Takashi Iwai <tiwai@suse.de>
 *                     PeiSen Hou <pshou@realtek.com.tw>
 *
 *
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <posix/signal.h>
#include <macros.h>

#include "hda.h"

static PCI_bus_s *g_psBus;

struct audio_device
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
};

struct audio_device g_sDevices[] = {
	{0x8086, 0x2668, "Intel", "HD-Audio (ICH6)"},
	{0x8086, 0x27d8, "Intel", "HD-Audio (ICH7)"},
	{0x8086, 0x269a, "Intel", "HD-Audio (ESB2)"},
	{0x8086, 0x284b, "Intel", "HD-Audio (ICH8)"},
	{0x8086, 0x293e, "Intel", "HD-Audio (ICH9)"},
	{0x8086, 0x293f, "Intel", "HD-Audio (ICH9)"},
	{0x1002, 0x437b, "ATI", "HD-Audio (SB450)"},
	{0x1002, 0x4383, "ATI", "HD-Audio (SB600)"},
	{0x1002, 0x793b, "ATI", "HD-Audio (RS600 HDMI)"},
	{0x1002, 0x7919, "ATI", "HD-Audio (RS690 HDMI)"},
	{0x1106, 0x3288, "VIA", "HD-Audio"},
	{0x1039, 0x7502, "SiS", "HD-Audio"},
	{0x10b9, 0x5461, "ULI", "HD-Audio"},
	{0x10de, 0x026c, "nVIDIA", "HD-Audio (MCP51)"},
	{0x10de, 0x0371, "nVIDIA", "HD-Audio (MCP55)"},
	{0x10de, 0x03e4, "nVIDIA", "HD-Audio (MCP61)"},
	{0x10de, 0x03f0, "nVIDIA", "HD-Audio (MCP61)"},
	{0x10de, 0x044a, "nVIDIA", "HD-Audio (MCP65)"},
	{0x10de, 0x044b, "nVIDIA", "HD-Audio (MCP65)"},
	{0x10de, 0x055c, "nVIDIA", "HD-Audio (MCP67)"},
	{0x10de, 0x055d, "nVIDIA", "HD-Audio (MCP67)"},	
};


/* Send one command to the codec */
int hda_send_cmd( HDAAudioDriver_s* psDriver, int nCodec, int nNode, int nDirect,
				int nVerb, int nPara )
{
	/* Build command */
	uint32 nVal;
	uint32 nWp;
	
	
	nVal = ( nCodec & 0x0f ) << 28;
	nVal |= nDirect << 27;
	nVal |= nNode << 20;
	nVal |= nVerb << 8;
	nVal |= nPara;
	
	/* Add command */
	LOCK( psDriver->hLock );
	
	
	nWp = readb( psDriver->nBaseAddr + HDA_REG_CORBWP );
	nWp++;
	nWp %= HDA_MAX_CORB_ENTRIES;
	
	//printk( "Write pointer %x\n", nWp );
	psDriver->sRirb.nCmds++;
	psDriver->sCorb.pBuf[nWp] = nVal;
	writel( nWp, psDriver->nBaseAddr + HDA_REG_CORBWP );
	
	UNLOCK( psDriver->hLock );
	
	return( 0 );
}

void hda_update_rirb( HDAAudioDriver_s* psDriver )
{
	uint32 nWp = readb( psDriver->nBaseAddr + HDA_REG_RIRBWP );
	uint32 nRp;
	uint32 nVal;
	uint32 nValEx;
	//printk( "RIRB pointer %x\n", nWp );
	if( nWp == psDriver->sRirb.nWp )
		return;
	psDriver->sRirb.nWp = nWp;
	
	while( psDriver->sRirb.nRp != nWp )
	{
		psDriver->sRirb.nRp++;
		psDriver->sRirb.nRp %= HDA_MAX_RIRB_ENTRIES;
		nRp = psDriver->sRirb.nRp << 1;
		nValEx = psDriver->sRirb.pBuf[nRp + 1];
		nVal = psDriver->sRirb.pBuf[nRp];
		
		if( nValEx & HDA_RIRB_EX_UNSOL_EV ) {
			printk( "Got Unsol event!\n" );
		}
		else if( psDriver->sRirb.nCmds )
		{
			psDriver->sRirb.nCmds--;
			psDriver->sRirb.nResult = nVal;
			//printk( "Result %x\n", nVal );
		}
		
	}
}

uint32 hda_get_response( HDAAudioDriver_s* psDriver )
{
	int nTimeout = 50;
	for( ;; )
	{
		if( !nTimeout-- )
		{
			printk( "HDA: Timeout waiting for response!\n" );
			return 0;
		}
		if( psDriver->sRirb.nCmds == 0 )
		{
			return( psDriver->sRirb.nResult );
		}
		snooze( 1000 );
	}
	return( 0 );
}

/* Streams */

static status_t hda_check_format( HDAAudioDriver_s* psDriver, HDAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	int i;
	/* Resolution */
	if( nResolution != 16 )
		return( -1 );
		
	/* Check if we support the number of channels */
	if( nChannels < 2 )
		return( -1 );
		
	if( nChannels > psDriver->nOutputPaths * 2 )
	{
		printk( "Too many channels (%i > %i)!\n", nChannels, psDriver->nOutputPaths * 2 );
		return( -1 );
	}
	

	/* Check if we support the rate (TODO: higher rates) */
	uint32 nBit;
	if( nSampleRate == 44100 )
		nBit = 1 << 5;
	else if( nSampleRate == 48000 )
		nBit = 1 << 6;
	else
		return( -1 );
	for( i = 0; i < nChannels; i++ )
	{
		HDAOutputPath_s* psPath = &psDriver->pasOutputPaths[i/2];
		if( psPath->nDacNode != -1 && ( psPath->nFormats & nBit ) == 0 )
		{
			printk( "Output path %i does not support samplerate %i\n", i, nSampleRate );
			return( -1 );
		}
	}
	
		
	if( psStream->bIsRecord && nChannels > 2 )
	{
		return( -1 );
	}
	
	return( 0 );
}

static status_t hda_set_format( HDAAudioDriver_s* psDriver, HDAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	int nTimeout;
	uint8 nVal;
	uint32 nFormat = 0;
	int i;
	LOCK( psDriver->hLock );
	
	/* Stream format */
	if( nSampleRate == 44100 )
		nFormat = 0x4000;
	else if( nSampleRate == 48000 )
		nFormat = 0x0000;
	
	nFormat |= nChannels - 1;
	nFormat |= 0x10; // 16 bit
	
	printk( "Format %x\n", nFormat );
	
	/* Save data */
	psStream->nChannels = nChannels;
	psStream->nSampleRate = nSampleRate;
	
	/* Reset */
	writeb( readb( psStream->nIoBase + HDA_REG_SD_CTL ) &~ SD_CTL_DMA_START, psStream->nIoBase + HDA_REG_SD_CTL );
	writeb( readb( psStream->nIoBase + HDA_REG_SD_CTL ) | SD_CTL_STREAM_RESET, psStream->nIoBase + HDA_REG_SD_CTL );
	snooze( 3 );
	
	nTimeout = 300;
	while( !( ( nVal = readb( psStream->nIoBase + HDA_REG_SD_CTL ) ) & SD_CTL_STREAM_RESET ) && --nTimeout )
	;
	
	nVal &= ~SD_CTL_STREAM_RESET;
	writeb( nVal, psStream->nIoBase + HDA_REG_SD_CTL );
	
	nTimeout = 300;
	while( ( ( nVal = readb( psStream->nIoBase + HDA_REG_SD_CTL ) ) & SD_CTL_STREAM_RESET ) && --nTimeout )
	;
	
	/* Stream tag */
	writel( ( readl( psStream->nIoBase + HDA_REG_SD_CTL ) &~ SD_CTL_STREAM_TAG_MASK )
			| ( psStream->nStreamTag << SD_CTL_STREAM_TAG_SHIFT ), psStream->nIoBase + HDA_REG_SD_CTL );
	/* Buffer size */
	writel( psStream->nFragSize * psStream->nFragNumber, psStream->nIoBase + HDA_REG_SD_CBL );
	
	/* Buffer size */
	writew( nFormat, psStream->nIoBase + HDA_REG_SD_FORMAT );
	
	/* LVI */
	writew( psStream->nLVI, psStream->nIoBase + HDA_REG_SD_LVI );
	
	/* Load SGD table */
	writel( psStream->nSgdPhysAddr, psStream->nIoBase + HDA_REG_SD_BDLPL );
	writel( 0, psStream->nIoBase + HDA_REG_SD_BDLPU );
	
	/* Enable interrupts */
	writel( readl( psStream->nIoBase + HDA_REG_SD_CTL ) | SD_INT_MASK, psStream->nIoBase + HDA_REG_SD_CTL );
	writeb( SD_INT_MASK, psStream->nIoBase + HDA_REG_SD_STS );
	
	/* Enable codec */
	for( i = 0; i < psDriver->nOutputPaths; i++ )
	{
		/* The first output paths play the channels, the other ones the front channel */
		int nCh = i * 2;
		if( nCh + 1 >= nChannels )
			nCh = 0;
		HDAOutputPath_s* psPath = &psDriver->pasOutputPaths[i];
		printk( "Use pin %i, output %i for channels %i,%i ( stream: %i )\n", psPath->nPinNode, psPath->nDacNode, nCh, nCh + 1, psStream->nStreamTag );
		if( psPath->nDacNode != -1 )
		{
			hda_send_cmd( psDriver, 0, psPath->nDacNode, 0, AC_VERB_SET_CHANNEL_STREAMID,
						( psStream->nStreamTag << 4 ) | nCh );
			snooze( 1000 );
			hda_send_cmd( psDriver, 0, psPath->nDacNode, 0, AC_VERB_SET_STREAM_FORMAT,
						nFormat );
		}
	}
	
	
	UNLOCK( psDriver->hLock );
	
	printk( "Format programmed!\n");
	
	return( 0 );
}

static status_t hda_create_buffers( HDAAudioDriver_s* psDriver, HDAStream_s* psStream, int nChannels, int nSampleRate, int nResolution )
{
	/* Set fragments */
	psStream->nFragSize = HDA_MAX_FRAG_SIZE / 2;
	psStream->nFragNumber = HDA_MAX_FRAGS;
	
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
	psStream->hSgdArea = create_area( "hda_dma", (void**)&pAddress, PAGE_SIZE, PAGE_SIZE, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	psStream->pasSgTable = (volatile struct hda_sgd_table*)pAddress;
	memset( pAddress, 0, sizeof( struct hda_sgd_table ) * psStream->nFragNumber );
	
	/* Calculate number of pages */
	psStream->nPageNumber = ( psStream->nFragSize * psStream->nFragNumber ) / PAGE_SIZE +
		( ( ( psStream->nFragSize * psStream->nFragNumber ) % PAGE_SIZE ) ? 1 : 0 );
		
	printk( "Need %i pages\n", (int)psStream->nPageNumber );
	
	/* Create audio buffer */
	psStream->pBuffer = NULL;
	psStream->hBufArea = create_area( "hda_buf", (void**)&psStream->pBuffer, PAGE_SIZE * psStream->nPageNumber, PAGE_SIZE * psStream->nPageNumber, AREA_FULL_ACCESS | AREA_KERNEL, AREA_CONTIGUOUS );
	memset( psStream->pBuffer, 0,  PAGE_SIZE * psStream->nPageNumber );
	uint32 nPhysAddr;
	get_area_physical_address( psStream->hBufArea, &nPhysAddr );
	printk("Buf: Address 0x%x -> 0x%x\n", (uint)psStream->pBuffer, (uint)nPhysAddr );
		
	/* Fill sgd table */
	uint i;
	for( i = 0; i < psStream->nFragNumber; i++ )
	{
		psStream->pasSgTable[i].count = psStream->nFragSize;
		psStream->pasSgTable[i].flags = 0x01;
		psStream->pasSgTable[i].addr = nPhysAddr;
		psStream->pasSgTable[i].addr_high = 0;
		printk( "SG: %i -> 0x%x\n", i, (uint)nPhysAddr );
		nPhysAddr += psStream->nFragSize;
	}
	
	/* Get SGD address */
	get_area_physical_address( psStream->hSgdArea, &psStream->nSgdPhysAddr );
	printk("SG: Address 0x%x -> 0x%x\n", (uint)pAddress, (uint)psStream->nSgdPhysAddr );
	
	psStream->nLVI = psStream->nFragNumber - 1;
	psStream->nCIV = 0;
	
	generic_stream_clear( &psStream->sGeneric );
	
	return( 0 );
}


uint32 hda_get_frag_size( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	return( psStream->nFragSize );
}

uint32 hda_get_frag_number( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	return( psStream->nFragNumber );
}

uint8* hda_get_buffer( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	return( psStream->pBuffer );
}

uint32 hda_get_current_position( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	HDAAudioDriver_s* psDriver = psStream->psDriver;
	uint32 nPos = readl( psStream->nIoBase + HDA_REG_SD_LPIB );
	nPos %= psStream->nFragSize;
	return( nPos );
}

status_t hda_start( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	HDAAudioDriver_s* psDriver = psStream->psDriver;
	if( !psStream->bIsActive )
	{
		printk( "Start!\n" );
		psStream->bIsActive = true;
		writeb( readb( psDriver->nBaseAddr + HDA_REG_INTCTL ) | psStream->nIRQMask,
			psDriver->nBaseAddr + HDA_REG_INTCTL );
		writeb( readb( psStream->nIoBase + HDA_REG_SD_CTL ) | SD_CTL_DMA_START | SD_INT_MASK,
			psStream->nIoBase + HDA_REG_SD_CTL );
		
	}
	return( 0 );
}

void hda_stop( void* pDriverData )
{
	HDAStream_s* psStream = pDriverData;
	HDAAudioDriver_s* psDriver = psStream->psDriver;
	if( psStream->bIsActive )
	{
		psStream->bIsActive = false;
		writeb( readb( psStream->nIoBase + HDA_REG_SD_CTL ) & ~( SD_CTL_DMA_START | SD_INT_MASK ),
			psStream->nIoBase + HDA_REG_SD_CTL );
		writeb( readb( psDriver->nBaseAddr + HDA_REG_INTCTL ) & ~psStream->nIRQMask,
			psDriver->nBaseAddr + HDA_REG_INTCTL );
		printk( "stoped!\n" );
	}
}


static void hda_clear( HDAAudioDriver_s* psDriver, HDAStream_s* psStream )
{
	LOCK( psDriver->hLock );
	
	hda_stop( psStream );
	
	// FIXME: We need to reset the playback pointer somehow!
	
	UNLOCK( psDriver->hLock );
	
	generic_stream_clear( &psStream->sGeneric );
}


status_t hda_open( void *pNode, uint32 nFlags, void **pCookie )
{
	HDAStream_s* psStream = pNode;
	
	
	/* Check what open mode is requested */
	if( nFlags & O_RDWR )
	{
		/* This means that the application wants to read/write from the device
		and not just use the ioctl commands for the mixer */
		if( psStream->bIsLocked )
		{
			printk( "Error: hda_open() stream already used\n" );
			return( -EBUSY );
		}
		psStream->bIsLocked = true;
		psStream->hBufArea = -1;
		psStream->hSgdArea = -1;
		generic_stream_init( &psStream->sGeneric, psStream->bIsRecord );
		*((int*)pCookie) = 1;
		printk( "hda_open() for writing\n" );
	}
	else
	{
		*((int*)pCookie) = 0;
		printk( "hda_open() for reading\n" );
	}
	
	
	return( 0 );		
}

status_t hda_close( void *pNode, void *pCookie )
{
	HDAStream_s* psStream = pNode;
	printk( "hda_close()\n");
	
	if( (int)pCookie == 1 )
	{
		/* Stop playback */
		hda_stop( psStream );
	
		// TODO: Reset
	
		/* Delete areas */
		if( psStream->hBufArea > -1 )
			delete_area( psStream->hBufArea );
		if( psStream->hSgdArea > -1 )
			delete_area( psStream->hSgdArea );
			
		psStream->hBufArea = psStream->hSgdArea = -1;
		
			
		generic_stream_free( &psStream->sGeneric );
		psStream->bIsLocked = false;
		printk( "hda_close() for writing\n");
	} else
		printk( "hda_close() for reading\n");
		
	return( 0 );
}


int	hda_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	HDAStream_s* psStream = pNode;
	if( !psStream->bIsRecord || !psStream->bIsLocked )
		return( -EINVAL );
	return( generic_stream_read( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

int	hda_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	HDAStream_s* psStream = pNode;
	return( generic_stream_write( &psStream->sGeneric, (void*)pBuffer, nSize ) );
}

status_t hda_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	HDAStream_s* psStream = pNode;
	HDAAudioDriver_s* psDriver = psStream->psDriver;
	switch( nCommand )
	{
		case IOCTL_GET_USERSPACE_DRIVER:
			memcpy_to_user( pArgs, "hd_audio.so", strlen( "hd_audio.so" ) );
		break;
		
		case HDA_GET_CARD_INFO:
		{
			HDACardInfo_s sInfo;
			sInfo.nDeviceID = psDriver->nDeviceID;
			sInfo.nDeviceHandle = psDriver->sPCI.nHandle;
			strcpy( sInfo.zName, psStream->zName );
			sInfo.bRecord = psStream->bIsRecord;
			sInfo.bSPDIF = false;
			if( psStream->bIsRecord )
				sInfo.nMaxChannels = 2;
			else
				sInfo.nMaxChannels = psDriver->nOutputPaths * 2;
			sInfo.nFormats = psDriver->pasOutputPaths[0].nFormats;
			sInfo.nMaxSampleRate = 48000; /* TODO: 96000 support */
			sInfo.nMaxResolution = 16; /* TODO: 24bit support */
			memcpy_to_user( pArgs, &sInfo, sizeof( sInfo ) );
		}
		break;
		case HDA_SET_FORMAT:
		{
			HDAFormat_s sFormat;
			int nError;
			memcpy_from_user( &sFormat, pArgs, sizeof( sFormat ) );
			
			nError = hda_check_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			nError = hda_create_buffers( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution );
			if( nError < 0 )
				return( nError );
			return( hda_set_format( psStream->psDriver, psStream, sFormat.nChannels, sFormat.nSampleRate, sFormat.nResolution ) );
		}
		break;
		case HDA_GET_BUFFER_SIZE:
		{
			//printk("GetBufferSize!\n");
			uint32 nBufferSize = psStream->nFragSize * psStream->nFragNumber;
			memcpy_to_user( pArgs, &nBufferSize, sizeof( uint32 ) );
		}
		break;
		case HDA_GET_DELAY:
		{
			//printk( "Get Delay\n");
			uint32 nValue = generic_stream_get_delay( &psStream->sGeneric );
			memcpy_to_user( pArgs, &nValue, sizeof( uint32 ) );
		}
		break;
		case HDA_CLEAR:
		{
			hda_clear( psDriver, psStream );
		}
		break;
		case HDA_GET_NODE:
		{
			HDANode_s* psNode = ( HDANode_s* )pArgs;
			if( psNode->nNode >= psDriver->nNodes )
				return( EINVAL );
			memcpy_to_user( pArgs, &psDriver->pasNodes[psNode->nNode], sizeof( HDANode_s ) );
		}
		break;
		case HDA_GET_OUTPUT_PATH:
		{
			HDAOutputPath_s* psPath = ( HDAOutputPath_s* )pArgs;
			if( psPath->nDacNode >= psDriver->nOutputPaths )
				return( EINVAL );
			memcpy_to_user( pArgs, &psDriver->pasOutputPaths[psPath->nDacNode], sizeof( HDAOutputPath_s ) );
		}
		break;
		case HDA_CODEC_SEND_CMD:
		{
			HDACmd_s* psCmd = ( HDACmd_s* )pArgs;
			return( hda_send_cmd( psDriver, psCmd->nCodec, psCmd->nNode, psCmd->nDirect,
					psCmd->nVerb, psCmd->nPara ) );
		}
		break;
		case HDA_CODEC_READ:
		{
			HDACmd_s* psCmd = ( HDACmd_s* )pArgs;
			hda_send_cmd( psDriver, psCmd->nCodec, psCmd->nNode, psCmd->nDirect,
					psCmd->nVerb, psCmd->nPara );
			psCmd->nResult = hda_get_response( psDriver );
		}
		break;
	}
	return( 0 );
}


DeviceOperations_s g_sOperations = {
	hda_open,
	hda_close,
	hda_ioctl,
	hda_read,
	hda_write
};



static void hda_init_stream( HDAAudioDriver_s* psDriver, int nChannel, char* pzName, int nReg, uint32 nIRQMask, bool bSPDIF, bool bRecord )
{
	/* Initialize stream structure */
	psDriver->sStream[nChannel].psDriver = psDriver;
	psDriver->sStream[nChannel].nStreamTag = nChannel + 1;
	psDriver->sStream[nChannel].nBaseAddr = psDriver->nBaseAddr;
	psDriver->sStream[nChannel].nIoBase = psDriver->nBaseAddr + nReg;
	psDriver->sStream[nChannel].hSgdArea = -1;
	psDriver->sStream[nChannel].nIRQMask = nIRQMask;
	//psDriver->sStream[nChannel].bIsSPDIF = bSPDIF;
	psDriver->sStream[nChannel].bIsRecord = bRecord;
	psDriver->sStream[nChannel].nChannels = 2;
	psDriver->sStream[nChannel].nSampleRate = 48000;
	sprintf( psDriver->sStream[nChannel].zName, "%s %s", psDriver->zName, pzName );
	
	/* Create generic output interface */
	GenericStream_s* psGeneric = &psDriver->sStream[nChannel].sGeneric;
	psGeneric->pfGetFragSize = hda_get_frag_size;
	psGeneric->pfGetFragNumber = hda_get_frag_number;
	psGeneric->pfGetBuffer = hda_get_buffer;
	psGeneric->pfGetCurrentPosition = hda_get_current_position;
	psGeneric->pfStart = hda_start;
	psGeneric->pfStop = hda_stop;
	psGeneric->pDriverData = &psDriver->sStream[nChannel];
	
	/* Create node */
	char zNodePath[PATH_MAX];
	sprintf( zNodePath, "audio/hda_%i_%s", psDriver->sPCI.nHandle, pzName );

	if( create_device_node( psDriver->nDeviceID,  psDriver->sPCI.nHandle, zNodePath, &g_sOperations, &psDriver->sStream[nChannel] ) < 0 )
	{
		printk( "Error: Failed to create device node %s\n", zNodePath );
	}
	
	psDriver->nNumStreams++;
	
	printk( "Add stream %s @ 0x%x %x\n", pzName, (uint)psDriver->sStream[nChannel].nIoBase, nIRQMask );
}

/* Interrupts */
void hda_stream_interrupt( HDAAudioDriver_s* psDriver, HDAStream_s* psStream )
{
	uint32 nPos = readl( psStream->nIoBase + HDA_REG_SD_LPIB );
	uint8 nCIV;
	//printk( "Position %i\n", nPos );
	nPos /= psStream->nFragSize;
	nCIV = nPos;
	//printk( "CIV %i %i\n", psStream->nCIV, nCIV );
	/* Compare CIV with saved value */
	while( nCIV != psStream->nCIV )
	{
		/* Increase CIV and LVI */
		psStream->nCIV++;
		psStream->nCIV &= HDA_LVI_MASK;

		//psStream->nLVI++;
		//psStream->nLVI &= HDA_LVI_MASK;
		/* Tell the generic output */
		generic_stream_fragment_processed( &psStream->sGeneric );
	}	
	
	//printk( "CIV now at %i, LVI at %i\n", psStream->nCIV,  psStream->nLVI);
	
	/* Update LVI */
	//writew( psStream->nLVI, psStream->nIoBase + HDA_REG_SD_LVI );		
}

int hda_interrupt( int irq, void *dev_id, SysCallRegs_s *regs )
{
	HDAAudioDriver_s* psDriver = dev_id;
	
	uint32 nStatus = readl( psDriver->nBaseAddr + HDA_REG_INTSTS );
	if( nStatus == 0 )
		return( 0 );
		
	//printk( "Got IRQ!\n" );
	
	/* Check streams */
	int i = 0;
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		HDAStream_s* psStream = &psDriver->sStream[i];
		if( !( nStatus & ( psStream->nIRQMask ) ) )
			continue;
		//printk( "%x\n", readb( psStream->nIoBase + HDA_REG_SD_STS ) );
		writeb( SD_INT_MASK, psStream->nIoBase + HDA_REG_SD_STS );
		hda_stream_interrupt( psDriver, psStream );
	}
	
	/* Check RIRB status */
	nStatus = readb( psDriver->nBaseAddr + HDA_REG_RIRBSTS );
	if( nStatus & RIRB_INT_MASK )
	{
		//printk( "Got RIRB interrupt!\n" );
		if( nStatus & RIRB_INT_RESPONSE )
			hda_update_rirb( psDriver );
		writeb( RIRB_INT_MASK, psDriver->nBaseAddr + HDA_REG_RIRBSTS );
	}
	
	return( 0 );
}

static status_t hda_reset( HDAAudioDriver_s* psDriver )
{
	/* Reset */
	int nCount;
	writel( readl( psDriver->nBaseAddr + HDA_REG_GCTL ) & ~HDA_GCTL_RESET, psDriver->nBaseAddr + HDA_REG_GCTL );
	nCount = 50;
	while( readb( psDriver->nBaseAddr + HDA_REG_GCTL ) && nCount-- )
		snooze( 1000 );
	
	snooze( 1000 );
	
	writeb( readb( psDriver->nBaseAddr + HDA_REG_GCTL ) | HDA_GCTL_RESET, psDriver->nBaseAddr + HDA_REG_GCTL );
		
	
	nCount = 50;
	while( !readb( psDriver->nBaseAddr + HDA_REG_GCTL ) && nCount-- )
		snooze( 1000 );
		
	snooze( 1000 );
	
	if( !readb( psDriver->nBaseAddr + HDA_REG_GCTL ) )
	{
		printk( "HDA: reset failed!\n" );
		return( -1 );
	}
	
	writel( readl( psDriver->nBaseAddr + HDA_REG_GCTL ) | HDA_GCTL_UREN, psDriver->nBaseAddr + HDA_REG_GCTL	);
		
	/* Detect codecs */
	psDriver->nCodecMask = readw( psDriver->nBaseAddr + HDA_REG_STATESTS );
	
	printk( "Codec mask %x\n", psDriver->nCodecMask );
	
	if( !( psDriver->nCodecMask & 1 ) )
	{
		printk( "Error: Codec #0 not present!\n" );
		return -EIO;
	}
	
	return( 0 );
		
}

static void hda_init_irq( HDAAudioDriver_s* psDriver )
{
	/* Clear interrupts */
	writeb( STATESTS_INT_MASK, psDriver->nBaseAddr + HDA_REG_STATESTS );
	writeb( RIRB_INT_MASK, psDriver->nBaseAddr + HDA_REG_RIRBSTS );
	writel( HDA_INT_CTRL_EN | HDA_INT_ALL_STREAM, psDriver->nBaseAddr + HDA_REG_INTSTS );
	
	/* Enable interrupts */
	writel( readl( psDriver->nBaseAddr + HDA_REG_INTCTL )
		| HDA_INT_CTRL_EN | HDA_INT_GLOBAL_EN, psDriver->nBaseAddr + HDA_REG_INTCTL );
	
}

static void hda_init_cmd_buf( HDAAudioDriver_s* psDriver )
{
	uint32 nPhysical;
	get_area_physical_address( psDriver->hCmdBufArea, &nPhysical );
	
	printk( "CORB @ 0x%x mapped to 0x%x\n", (uint32)psDriver->pCmdBufAddr, nPhysical );
	
	/* Initialize corb */
	psDriver->sCorb.pBuf = psDriver->pCmdBufAddr;
	writel( nPhysical, psDriver->nBaseAddr + HDA_REG_CORBLBASE );	
	writel( 0, psDriver->nBaseAddr + HDA_REG_CORBUBASE );
	
	writeb( 0x02, psDriver->nBaseAddr + HDA_REG_CORBSIZE );	
	writew( 0, psDriver->nBaseAddr + HDA_REG_CORBWP );	
	writew( HDA_RBRWP_CLR, psDriver->nBaseAddr + HDA_REG_CORBRP );
	writeb( HDA_RBCTL_DMA_EN, psDriver->nBaseAddr + HDA_REG_CORBCTL );
	
	/* Initialize rirb */
	nPhysical += PAGE_SIZE / 2;
	psDriver->sRirb.pBuf = psDriver->pCmdBufAddr + PAGE_SIZE / 2 / 4;
	
	
	printk( "RIRB @ 0x%x mapped to 0x%x\n", (uint32)psDriver->sRirb.pBuf, nPhysical );
	
	writel( nPhysical, psDriver->nBaseAddr + HDA_REG_RIRBLBASE );	
	writel( 0, psDriver->nBaseAddr + HDA_REG_RIRBUBASE );
	
	writeb( 0x02, psDriver->nBaseAddr + HDA_REG_RIRBSIZE );	
	writew( HDA_RBRWP_CLR, psDriver->nBaseAddr + HDA_REG_RIRBWP );	
	writew( 1, psDriver->nBaseAddr + HDA_REG_RINTCNT );	
	writeb( HDA_RBCTL_DMA_EN | HDA_RBCTL_IRQ_EN, psDriver->nBaseAddr + HDA_REG_RIRBCTL );
}

static status_t hda_init( int nDeviceID, PCI_Info_s sPCI, char* zName )
{
	printk( "hda_init()\n" );
	HDAAudioDriver_s* psDriver;
	void* pAddr = NULL;
	
	psDriver = kmalloc( sizeof( HDAAudioDriver_s ), MEMF_KERNEL | MEMF_CLEAR );
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
	printk( "MMIO IO @ 0x%x\n", sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK );
		
	/* Remap registers. Be careful with page alignment */
	psDriver->hBaseArea = create_area( "hda_mmio", &pAddr, PAGE_SIZE, PAGE_SIZE, AREA_KERNEL | AREA_ANY_ADDRESS, AREA_NO_LOCK );
	remap_area( psDriver->hBaseArea, (void*)( ( sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) ); 
	psDriver->nBaseAddr = (uint32)pAddr + ( ( sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) - ( ( sPCI.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) & PAGE_MASK ) );
	
	{
		uint8 nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, HDA_PCIREG_TCSEL, 1 );
		nVal &= 0xf8;
		g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, HDA_PCIREG_TCSEL, 1, nVal );
		
		/* For ATI SB450 azalia HD audio, we need to enable snoop */
		if( sPCI.nVendorID == 0x1002 && ( sPCI.nDeviceID != 0x793b && sPCI.nDeviceID != 0x7919 ) )
		{
			uint8 nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, ATI_SB450_HDAUDIO_MISC_CNTR2_ADDR, 1 );
			nVal &= 0xf8;
			nVal |= ATI_SB450_HDAUDIO_ENABLE_SNOOP;
			g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, ATI_SB450_HDAUDIO_MISC_CNTR2_ADDR, 1, nVal );
		}
		/* For NVIDIA HDA, enable snoop */
		if( sPCI.nVendorID == 0x10de )
		{
			uint8 nVal = g_psBus->read_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction,NVIDIA_HDA_TRANSREG_ADDR, 1 );
			nVal &= 0xf0;
			nVal |= NVIDIA_HDA_ENABLE_COHBITS;
			g_psBus->write_pci_config( psDriver->sPCI.nBus, psDriver->sPCI.nDevice, psDriver->sPCI.nFunction, NVIDIA_HDA_TRANSREG_ADDR, 1, nVal );
		}
	}
	
	/* Reset */
	if( hda_reset( psDriver ) != 0 )
	{
		delete_area( psDriver->hBaseArea );
		kfree( psDriver );
		return( -EIO );
	}
	
	
	/* Enable interrupts */
	hda_init_irq( psDriver );

	/* Allocate and initialize command buffer */
	psDriver->hCmdBufArea = create_area( "hda_cmd_buf", (void**)&psDriver->pCmdBufAddr, PAGE_SIZE, PAGE_SIZE, AREA_KERNEL | AREA_FULL_ACCESS, AREA_CONTIGUOUS );
	hda_init_cmd_buf( psDriver );
	
	/* Disable position buffer */
	writel( 0, psDriver->nBaseAddr + HDA_REG_DPLBASE );
	writel( 0, psDriver->nBaseAddr + HDA_REG_DPUBASE );
	
	
	/* Init streams */
	if( sPCI.nVendorID == 0x10b9 )
		hda_init_stream( psDriver, 0, "pcm", 0x120, 1 << 5, false, false );
	else if( sPCI.nVendorID == 0x1002 &&  ( sPCI.nDeviceID == 0x793b || sPCI.nDeviceID == 0x7919 ) )
		hda_init_stream( psDriver, 0, "hdmi", 0x80, 1 << 0, false, false );
	else 
		hda_init_stream( psDriver, 0, "pcm", 0x100, 1 << 4, false, false );
	
	if ( claim_device( nDeviceID, sPCI.nHandle, zName, DEVICE_AUDIO ) != 0 )
	{
		kfree( psDriver );
		return( -EIO );
	}
	
	
	set_device_data( sPCI.nHandle, psDriver );
		
	/* Request irq */
	psDriver->nIRQHandle = request_irq( sPCI.u.h0.nInterruptLine, hda_interrupt, NULL, SA_SHIRQ, "hda_audio", psDriver );	
	
	/* Create lock */
	psDriver->hLock = create_semaphore( "hda_lock", 1, SEM_RECURSIVE );
	
	/* Initialize codec */
	hda_initialize_codec( psDriver, 0 );
	
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
	for ( nPCINum = 0; g_psBus->get_pci_info( &sInfo, nPCINum ) == 0 && bDevFound == false; ++nPCINum )
	{
		for ( nDeviceNum = 0; nDeviceNum < nNumDevices; ++nDeviceNum )
		{
			/* Compare vendor and device id */
			if ( sInfo.nVendorID == g_sDevices[nDeviceNum].nVendorID && sInfo.nDeviceID == g_sDevices[nDeviceNum].nDeviceID )
			{
				char zTemp[255];
				sprintf( zTemp, "%s %s", g_sDevices[nDeviceNum].zVendorName, g_sDevices[nDeviceNum].zDeviceName );
				/* Init card */
				if( hda_init( nDeviceID, sInfo, zTemp ) == 0 )
					bDevFound = true;
			}
		}
	}
	end:

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
	HDAAudioDriver_s* psDriver = pData;
	
	/* Stop playback */
	int i;
	
	printk( "Suspend %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		HDAStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Stop playback on %s\n", psStream->zName );
			hda_stop( psStream );
			psStream->bIsActive = true; /* Remember to start playback after resume */
		}
	}
	
	// TODO: Suspend codec
	
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
	HDAAudioDriver_s* psDriver = pData;
	
	/* Restart playback */
	int i;
	
	printk( "Resume %s @ 0x%x\n", psDriver->zName, (uint)psDriver->nBaseAddr );
	
	// TODO: Resume codec
	
	for( i = 0; i < psDriver->nNumStreams; i++ )
	{
		HDAStream_s* psStream = &psDriver->sStream[i];
		
		if( psStream->bIsActive )
		{
			printk( "Restarting playback on %s\n", psStream->zName );
			psStream->bIsActive = false;
			hda_set_format( psDriver, psStream, psStream->nChannels, psStream->nSampleRate, 16 );
			hda_start( psStream );
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
	HDAAudioDriver_s* psDriver = pPrivateData;
	/* Release IRQ */
	release_irq( psDriver->sPCI.u.h0.nInterruptLine, psDriver->nIRQHandle );
	/* Delete area */
	delete_area( psDriver->hBaseArea );
	/* Delete area */
	delete_area( psDriver->hCmdBufArea );
	/* Release device */
	release_device( psDriver->sPCI.nHandle );
	/* Delete private data */
	delete_semaphore( psDriver->hLock );
	if( psDriver->pasNodes != NULL )
		kfree( psDriver->pasNodes );
	kfree( psDriver );
	/* The devices nodes are deleted by the system */
}

























































