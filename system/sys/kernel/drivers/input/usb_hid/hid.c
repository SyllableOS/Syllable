/*
 *  The Syllable kernel
 *  USB mouse driver
 *  Copyright (C) 2003-2006 Arno Klenke
 *	Based on linux usbmouse driver :
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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
 
 */


#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/smp.h>
#include <atheos/usb.h>
#include "hid.h"

/* Mouse */
sem_id g_hMouseWait;
char g_pMouseBuffer[4];

/* Keyboard */
#define USB_KEYB_BUF_SIZE 2048
sem_id g_hKeybWait;
char g_pKeybBuffer[ USB_KEYB_BUF_SIZE ];
atomic_t g_nKeybOutPos;
atomic_t g_nKeybInPos;
atomic_t g_nKeybBytesReceived;
SpinLock_s g_sKeybLock = INIT_SPIN_LOCK( "usb_keyb_lock" );   
 
USB_bus_s* g_psBus = NULL;
int g_nDeviceID;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_open( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_close( void* pNode, void* pCookie )
{
    
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbmouse_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	if( nSize != 4 )
		return( -1 );
	sleep_on_sem( g_hMouseWait, INFINITE_TIMEOUT );
	memcpy_to_user( pBuffer, &g_pMouseBuffer, 4 );
	memset( &g_pMouseBuffer[1], 0, 3 );
	return( 4 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbmouse_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    return( -1 );
}

DeviceOperations_s g_sMouseOperations = {
    usbmouse_open,
    usbmouse_close,
    usbmouse_ioctl,
    usbmouse_read,
    usbmouse_write
};


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbkeyb_open( void* pNode, uint32 nFlags, void **pCookie )
{
    atomic_set( &g_nKeybInPos, 0 );
    atomic_set( &g_nKeybOutPos, 0 );
	atomic_set( &g_nKeybBytesReceived, 0 );
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbkeyb_close( void* pNode, void* pCookie )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbkeyb_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbkeyb_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	uint32 nFlg;
	int	 nError;
	int	 nBytesReceived = 0;
  
again:
    nFlg = spinlock_disable( &g_sKeybLock );
  
    if ( atomic_read( &g_nKeybBytesReceived ) == 0 ) {
	nError = spinunlock_and_suspend( g_hKeybWait, &g_sKeybLock, nFlg, INFINITE_TIMEOUT );
	if ( nError < 0 ) {
	    goto error;
	}
	goto again;
    }
    while( atomic_read( &g_nKeybBytesReceived ) > 0 && nBytesReceived < nSize ) {
	((char*)pBuffer)[nBytesReceived++] = g_pKeybBuffer[ atomic_inc_and_read( &g_nKeybOutPos ) % USB_KEYB_BUF_SIZE ];
	atomic_dec( &g_nKeybBytesReceived );
    }
    spinunlock_enable( &g_sKeybLock, nFlg );
    return( nBytesReceived );
error:
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbkeyb_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    return( -1 );
}

DeviceOperations_s g_sKeybOperations = {
    usbkeyb_open,
    usbkeyb_close,
    usbkeyb_ioctl,
    usbkeyb_read,
    usbkeyb_write
};


#define get_unaligned(ptr) (*(ptr))

/*
 * Convert a signed n-bit integer to signed 32-bit integer. Common
 * cases are done through the compiler, the screwed things has to be
 * done by hand.
 */

static inline int32 snto32( uint32 value, unsigned n )
{
	switch( n ) {
		case 8:  return ((int8)value);
		case 16: return ((int16)value);
		case 32: return ((int32)value);
	}
	return value & (1 << (n - 1)) ? value | (-1 << n) : value;
}

/*
 * Extract/implement a data field from/to a report.
 */

static inline uint32 extract( uint8 *report, unsigned offset, unsigned n)
{
	report += (offset >> 5) << 2; offset &= 31;
	return( (get_unaligned((uint64*)report)) >> offset) & ((1 << n) - 1);
}

static inline int search( int32 *array, int32 value, unsigned n)
{
	while (n--) if (*array++ == value) return 0;
	return -1;
}


void usb_hid_irq( USB_packet_s *psPacket )
{
	struct USB_hid_s *psHID = psPacket->pCompleteData;
	signed char *pData = psHID->anData;
	uint32 nLen = psPacket->nActualLength;
	int nOffset = 0;
	uint8 nReport = 0;
	uint32 nSize;
	uint32 i;
	bool bMouseCmds = false;
	char anMouseBuffer[4];
	memset( anMouseBuffer, 0, 4 );
	

	
	
	if ( psPacket->nStatus ) return;
	
	if( psHID->bNumberedData )
	{
		nReport = *pData;
		pData++;
		nLen--;
	}
	
	struct HID_report_s* psReport = &psHID->asReport[nReport];
	
	if( nLen == 0 || psReport->pBuffer == NULL )
		return;

	/* We might receive the data in smaller packets */	
	nSize = ( ( psReport->nSize - 1 ) >> 3 ) + 1;
	
	if( nLen < nSize )
	{
		if( psReport->nCurrentOffset + nLen > nSize )
		{
			printk( "USB HID: Overflow!\n" );
			psReport->nCurrentOffset = 0;
			return;
		}
		
		memcpy( psReport->pBuffer + psReport->nCurrentOffset, pData, nLen );
		psReport->nCurrentOffset += nLen;
		
		if( psReport->nCurrentOffset < nSize )
			return;
		pData = psReport->pBuffer;
	}
	
	/* Iterate through the fields */
	for( i = 0; i < psReport->nNumFields; i++ )
	{
		struct HID_field_s* psField = psReport->apsField[i];
		uint32 j;
		uint32 nFSize = psField->nSize;
		int32 nValue[psField->nCount];
		

		for( j = 0; j < psField->nCount; j++ )
		{
			nValue[j] = psField->nLogMin < 0 ? snto32( extract( pData, psField->nOffset + j * nFSize, nFSize ), nFSize ) :
						    extract( pData, psField->nOffset + j * nFSize, nFSize );
			if( !( psField->nFlags & HID_MAIN_ITEM_VARIABLE ) 
				&& nValue[j] >= psField->nLogMin && nValue[j] <= psField->nLogMax
				&& psField->anUsage[nValue[j] - psField->nLogMin] == HID_UP_KEYBOARD + 1 )
				return;
		}
		for( j = 0; j < psField->nCount; j++ )
		{
			
			//printk( "Field %i %x %i\n", (int)j, (uint)psField->nFlags, (int)nValue[j] );
		
			uint32 nUsage = 0;
			uint32 nVal = 0;
			bool bKeyCmd = false;
			uint8 nKeyVal = 0;

			if( psField->nFlags & HID_MAIN_ITEM_VARIABLE )
			{
				nUsage = psField->anUsage[j];
				nVal = nValue[j];
				
				if( nUsage == HID_GD_X ) /* Mouse X */
				{
					bMouseCmds = true;					
					anMouseBuffer[1] = nVal;
				}
				else if( nUsage == HID_GD_Y ) /* Mouse Y */
				{
					bMouseCmds = true;					
					anMouseBuffer[2] = nVal;
				}
				else if( nUsage == HID_GD_WHEEL ) /* Mouse Wheel */
				{
					bMouseCmds = true;
					anMouseBuffer[3] = nVal;
				}
				else if( ( nUsage & ( HID_USAGE_PAGE ) ) == HID_UP_BUTTON ) /* Mouse button */
				{
					bMouseCmds = true;
					int nButton = nUsage & HID_USAGE;
					if( nVal )
						anMouseBuffer[0] |= 1 << ( nButton - 1 );
				}
				else if( ( nUsage & ( HID_USAGE_PAGE ) ) == HID_UP_KEYBOARD ) /* Key */
				{
					int nCode = nUsage & HID_USAGE;
					if( nUsage >= 0 && nUsage >= 255 )
					{
						if( psField->pLastValues[j] != nVal )
						{
							/* Convert keycode */
							if( g_anHidKeyboard[nCode] != 0 )
							{
								bKeyCmd = true;
								nKeyVal = g_anRawKeyTab[g_anHidKeyboard[nCode]] | ( nVal ? 0 : 0x80 );
							}
							
							//printk( "Key %i %s\n", g_anHidKeyboard[nCode], nVal ? "pressed" : "released" );
						}
					}
					
				}
			}
			else
			{
				if( psField->pLastValues[j] >= psField->nLogMin && psField->pLastValues[j] <= psField->nLogMax
					&& search( nValue, psField->pLastValues[j], psField->nCount ) )
				{
					nUsage = psField->anUsage[psField->pLastValues[j] - psField->nLogMin];
					nVal = 0;
					//printk( "Usage %x released\n", psField->anUsage[psField->pLastValues[j] - psField->nLogMin] );
					if( ( nUsage & ( HID_USAGE_PAGE ) ) == HID_UP_KEYBOARD )
					{
						int nCode = nUsage & HID_USAGE;
						if( nUsage >= 0 && nUsage >= 255 )
						{
							/* Convert keycode */
							if( g_anHidKeyboard[nCode] != 0 )
							{
								bKeyCmd = true;
								nKeyVal = g_anRawKeyTab[g_anHidKeyboard[nCode]] | 0x80;
							}
							
							//printk( "Key %i released\n", g_anHidKeyboard[nCode] );
						}
					}
				}
				if( nValue[j] >= psField->nLogMin && nValue[j] <= psField->nLogMax
					&& search( psField->pLastValues, nValue[j], psField->nCount ) )
				{
					nUsage = psField->anUsage[nValue[j] - psField->nLogMin];
					nVal = 1;
					//printk( "Usage %x pressed\n", psField->anUsage[nValue[j] - psField->nLogMin] );
					if( ( nUsage & ( HID_USAGE_PAGE ) ) == HID_UP_KEYBOARD )
					{
						int nCode = nUsage & HID_USAGE;
						if( nUsage >= 0 && nUsage >= 255 )
						{
							/* Convert keycode */
							if( g_anHidKeyboard[nCode] != 0 )
							{
								bKeyCmd = true;
								nKeyVal = g_anRawKeyTab[g_anHidKeyboard[nCode]];
							}
							//printk( "Key %i pressed\n", g_anHidKeyboard[nCode] );
						}
					}
				}
			}
			/* Copy keycode to input buffer */
			if( bKeyCmd && ( nKeyVal & ~0x80 ) != 0 )
			{
				uint32 nFlg = spinlock_disable( &g_sKeybLock );
				if( atomic_read( &g_nKeybBytesReceived ) < USB_KEYB_BUF_SIZE ) {
					g_pKeybBuffer[ atomic_inc_and_read( &g_nKeybInPos ) % USB_KEYB_BUF_SIZE ] = nKeyVal;
					atomic_inc( &g_nKeybBytesReceived );
					wakeup_sem( g_hKeybWait, false );
			    }
				spinunlock_enable( &g_sKeybLock, nFlg );
			}
		}
		/* Save old values */
		memcpy( psField->pLastValues, nValue, psField->nCount * sizeof( int32 ) );
	}
	
	//printk( "Got packet %i\n", psPacket->nActualLength );
	/* Copy mouse codes to input buffer */
	if( bMouseCmds )
	{	
		memcpy( g_pMouseBuffer, anMouseBuffer, 4 );
		wakeup_sem( g_hMouseWait, false );
	}
}

static int usb_get_extra_descriptor(char *buffer, unsigned size,
	unsigned char type, void **ptr)
{
	USB_desc_header_s *header;

	while (size >= sizeof(USB_desc_header_s)) {
		header = (USB_desc_header_s *)buffer;

		if (header->nLength < 2) {
			printk(" bogus descriptor, type %d length %d\n",
				header->nDescriptorType, 
				header->nLength);
			return -1;
		}

		if (header->nDescriptorType == type) {
			*ptr = header;
			return 0;
		}

		buffer += header->nLength;
		size -= header->nLength;
	}
	return -1;
}


static int hid_get_class_descriptor( USB_device_s* psDevice, int nIFNum, uint8 nType, uint8 nID, uint8*
							pBuffer, int nSize )
{
	return( g_psBus->control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN,
		( nType << 8 ) + nID, nIFNum, pBuffer, nSize, 3 * 1000 * 1000 ) );
}

static int32 get_signed( uint32 nData, uint8 nDataSize )
{
	switch( nDataSize )
	{
		case 1:
			return( *((int8*)&nData) );
		break;
		case 2:
			return( *((int32*)&nData) );
		break;
		case 4:
			return( *((int32*)&nData) );
		break;		
	}
	return( 0 );
}

static void hid_parse_report( struct USB_hid_s* psHID, uint8* pReport, int nSize )
{
	/* Parse descriptor */
	uint8* pPtr = pReport;
	uint32 i;
	
	/* Global data */
	struct HID_Global_data_s
	{
		uint32 nUsagePage;
		int32 nLogMin;
		int32 nLogMax;
		int32 nPhyMin;
		int32 nPhyMax;
		uint32 nReportID;
		uint32 nReportSize;
		uint32 nReportCount;
	} sGlobal;
	memset( &sGlobal, 0, sizeof( struct HID_Global_data_s ) );
	
	/* Local data */
	struct HID_local_data_s
	{
		uint32 anUsage[1024];
		uint32 nUsageIndex;
		uint32 nUsageMin;
	} sLocal;
	memset( &sLocal, 0, sizeof( struct HID_local_data_s ) );
	
	while( nSize > 0 )
	{
		uint8 nVal = *pPtr++;
		nSize--;
		
		/* Decode item */
		
		uint8 nType = ( nVal >> 2 ) & 3;
		uint8 nTag = ( nVal >> 4 ) & 15;
		uint8 nDataSize = nVal & 3;
		uint32 nData = 0;
		
//		printk( "%x Type %i Tag %i Size %i\n", (uint)nVal, (int)( nType ), (int)( nTag ),
//						(int)( nDataSize ) );
		
		if( nTag == HID_ITEM_TAG_LONG )
		{
			printk( "Found long tag!\n" );
			nDataSize = *pPtr++;
			nTag = *pPtr++;
			pPtr += nSize;
			nSize -= nDataSize + 2;
			continue;
		}
						
		switch( nDataSize )
		{
			case 1:
				nData = *pPtr;
//				printk( "8 %x\n", (uint)*pPtr );
				pPtr++;
				nSize--;
			break;
			case 2:
				nData = *((uint16*)pPtr);
//				printk( "16 %x\n", (uint)*((uint16*)pPtr) );
				pPtr += 2;
				nSize -= 2;
			break;
			case 3:
				nData = *((uint32*)pPtr);
//				printk( "32 %x\n", (uint)*((uint32*)pPtr) );
				pPtr += 4;
				nSize -= 4;
			break;
		}
		
		switch( nType )
		{
			case HID_ITEM_TYPE_MAIN:
			{
				switch( nTag )
				{
					case HID_MAIN_ITEM_TAG_INPUT:
					{
						struct HID_report_s* psReport = &psHID->asReport[sGlobal.nReportID];
						
						if( sGlobal.nReportID > 0 )
							psHID->bNumberedData = true;
						
						/* Create new field */
						struct HID_field_s* psField = kmalloc( sizeof( struct HID_field_s )
							+ sLocal.nUsageIndex * sizeof( uint32 ), MEMF_KERNEL | MEMF_CLEAR );
						
						psField->nFlags = nData;
						psField->nOffset = psReport->nSize;

						
						/* Copy global data */
						psField->nSize = sGlobal.nReportSize;
						psField->nCount = sGlobal.nReportCount;
						psField->nLogMin = sGlobal.nLogMin;
						psField->nLogMax = sGlobal.nLogMax;
						psField->nPhyMin = sGlobal.nPhyMin;
						psField->nPhyMax = sGlobal.nPhyMax;
						
						/* Allocate space for last values */
						if( psField->nCount > 0 )
							psField->pLastValues = kmalloc( psField->nCount * sizeof( uint32 ), MEMF_KERNEL | MEMF_CLEAR );

						/* Copy usages */
						psField->nNumUsages = sLocal.nUsageIndex;
						for( i = 0; i < sLocal.nUsageIndex; i++ )
						{
							psField->anUsage[i] = sLocal.anUsage[i];
						}
						
						/* Add field to report */
						psReport->apsField[psReport->nNumFields++] = psField;
						psReport->nSize += psField->nSize * psField->nCount;
#if 0
						printk( "Main item!\n" );
						printk( "0x%x %i %i %i %i %i %i %i\n", (uint)sGlobal.nUsagePage,
							(int)sGlobal.nReportID,
						(int)sGlobal.nReportSize, (int)sGlobal.nReportCount, 
						(int)sGlobal.nLogMin, (int)sGlobal.nLogMax, 
						(int)sGlobal.nPhyMin, (int)sGlobal.nPhyMax );
						for( i = 0; i < sLocal.nUsageIndex; i++ )
							printk( "Usage %i -> %x\n", i, (uint)sLocal.anUsage[i] );
#endif
					}
					break;
				}
				memset( &sLocal, 0, sizeof( struct HID_local_data_s ) );
			}
			break;

			case HID_ITEM_TYPE_GLOBAL:
			{
//				printk( "Global item\n" );
				
				switch( nTag )
				{
					case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
						sGlobal.nUsagePage = nData;
					break;
					case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
						sGlobal.nLogMin = get_signed( nData, nDataSize );
					break;
					case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
						if( sGlobal.nLogMin < 0 )
							sGlobal.nLogMax = get_signed( nData, nDataSize );
						else
							sGlobal.nLogMax = nData;
					break;
					case HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM:
						sGlobal.nPhyMin = get_signed( nData, nDataSize );
					break;
					case HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
						if( sGlobal.nPhyMin < 0 )
							sGlobal.nPhyMax = get_signed( nData, nDataSize );
						else
							sGlobal.nPhyMax = nData;
					break;
					case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
						sGlobal.nReportSize = nData;
					break;
					case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
						sGlobal.nReportCount = nData;
					break;
					case HID_GLOBAL_ITEM_TAG_REPORT_ID:
						sGlobal.nReportID = nData;
					break;
				}
			}
			break;
			case HID_ITEM_TYPE_LOCAL:
			{
//				printk("Local tag\n" );
				switch( nTag )
				{
					case HID_LOCAL_ITEM_TAG_USAGE:
						if( nDataSize <= 2 )
							nData = ( sGlobal.nUsagePage << 16 ) + nData;
						sLocal.anUsage[sLocal.nUsageIndex++] = nData;
					break;
					case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
						if( nDataSize <= 2 )
							nData = ( sGlobal.nUsagePage << 16 ) + nData;
						sLocal.nUsageMin = nData;
					break;
					case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
					{
						if( nDataSize <= 2 )
							nData = ( sGlobal.nUsagePage << 16 ) + nData;
						for( i = sLocal.nUsageMin; i <= nData; i++ )
							sLocal.anUsage[sLocal.nUsageIndex++] = i;
					}
					break;
				}
			}
		}
	}
	
	uint32 j;
	for( j = 0; j < 256; j++ )
	{
		struct HID_report_s* psReport = &psHID->asReport[j];
		if( psReport->nSize == 0 )
			continue;
		printk( "HID report %i size %i\n", (int)j, (int)psReport->nSize );
	
		for( i = 0; i < psReport->nNumFields; i++ )
		{
			struct HID_field_s* psField = psReport->apsField[i];
			printk( "Field @ %i Flags: %x %ix%i LogMin: %i LogMax: %i %i usages\n", (int)psField->nOffset, (uint)psField->nFlags, (int)psField->nCount,
			(int)psField->nSize, (int)psField->nLogMin, (int)psField->nLogMax, (int)psField->nNumUsages );	
		}
		psReport->pBuffer = kmalloc( ( ( psReport->nSize - 1 ) >> 3 ) + 1, MEMF_KERNEL | MEMF_CLEAR );
	}
}

bool usbhid_add( USB_device_s* psDevice, unsigned int nIfnum,
		       void **pPrivate )
{
	USB_interface_s *psIface;
	USB_desc_interface_s *psInterface;
	USB_desc_endpoint_s *psEndpoint;
	struct USB_hid_s* psHID;
	int nPipe, nMaxp;
	char *pBuf;
	bool bFound = false;
	int i;
	
	//printk( "%i\n", nIfnum );
	
	psIface = psDevice->psActConfig->psInterface + nIfnum;
	
	for( i = 0; i < psIface->nNumSetting; i++ ) {
		psIface->nActSetting = i;
		psInterface = &psIface->psSetting[psIface->nActSetting];
		
		if( psInterface->nInterfaceClass == USB_CLASS_HID ) {
			bFound = true;
			break;
		}
	}
	if( !bFound )
		return( false );

	psIface = &psDevice->psActConfig->psInterface[nIfnum];
	psInterface = &psIface->psSetting[psIface->nActSetting];
	
	struct HID_descriptor_s *psHDesc;
	
	/* Get HID descriptor */
	if( ( usb_get_extra_descriptor( psInterface->pExtra, psInterface->nExtraLen, USB_DT_HID, (void**)&psHDesc )
		< 0 ) && ( ( usb_get_extra_descriptor( psInterface->psEndpoint[0].pExtra, psInterface->psEndpoint[0].nExtraLen, USB_DT_HID, (void**)&psHDesc ) < 0 ) ||
				psInterface->nNumEndpoints == 0 ) )
	{
		printk( "HID: Could not find HID descriptor!\n" );
		return( false );
	}
	
	/* Find report descriptor */
	int nSize = 0;
	for( i = 0; i < psHDesc->nNumDescriptors; i++ )
	{
		if( psHDesc->asDesc[i].nDescriptorType == USB_DT_REPORT )
			nSize = psHDesc->asDesc[i].nDescriptorLength;
	}
	
	if( nSize == 0 )
	{
		printk( "HID: Could not find report in HID descriptor!\n" );
		return( false );
	}
	
	
	uint8* pReport = kmalloc( nSize, MEMF_KERNEL | MEMF_CLEAR );
	
	printk( "HID: Device on interface %i %i\n", psInterface->nInterfaceNumber, nIfnum );
	
	/* Get report */
	int nResult = hid_get_class_descriptor( psDevice, psInterface->nInterfaceNumber, 
								USB_DT_REPORT, 0, pReport, nSize );
	
	if( nResult < 0 )
	{
		printk( "HID: Failed to get report descriptor\n" );
		kfree( pReport );
		return( false );
	}
	
	/* Create structure */
	if( !( psHID = kmalloc( sizeof(struct USB_hid_s), MEMF_KERNEL|MEMF_NOBLOCK ) ) ) return( false );
	memset( psHID, 0, sizeof( struct USB_hid_s ) );

	psHID->psDev = psDevice;
	
	/* Parse report */
	hid_parse_report( psHID, pReport, nSize );
	
	kfree( pReport );
	
	/* Find interrupt endpoint */
	for( i = 0; i < psInterface->nNumEndpoints; i++ )
	{
		psEndpoint = psInterface->psEndpoint + i;
	
		if( !( psEndpoint->nEndpointAddress & 0x80 ) )
			continue;

		if( ( psEndpoint->nMAttributes & 3 ) != 3 )
			continue;

		nPipe = usb_rcvintpipe( psDevice, psEndpoint->nEndpointAddress );
		nMaxp = usb_maxpacket( psDevice, nPipe, usb_pipeout( nPipe ) );

		USB_FILL_INT( &psHID->sIRQ, psDevice, nPipe, psHID->anData, nMaxp > 32 ? 32 : nMaxp,
		usb_hid_irq, psHID, psEndpoint->nInterval );
		
		break;
	}
	
	if( i == psInterface->nNumEndpoints )
	{
		printk( "HID: Failed to find interrupt endpoints\n" );
		kfree( psHID );
		return( false );
	}
	
	psHID->psDev = psDevice;
	psHID->nMaxpacket = nMaxp;

	/* Create name */
	if (!(pBuf = kmalloc(63, MEMF_KERNEL|MEMF_NOBLOCK))) {
		kfree( psHID );
		return( false );
	}

	if (psDevice->sDeviceDesc.nManufacturer &&
		g_psBus->string(psDevice, psDevice->sDeviceDesc.nManufacturer, pBuf, 63) > 0)
			strcat(psHID->zName, pBuf);
	if (psDevice->sDeviceDesc.nProduct &&
		g_psBus->string(psDevice, psDevice->sDeviceDesc.nProduct, pBuf, 63) > 0)
			sprintf(psHID->zName, "%s %s", psHID->zName, pBuf);

	if (!strlen(psHID->zName))
		sprintf(psHID->zName, "USB HID %04x:%04x",
			psHID->psDev->sDeviceDesc.nVendorID, psHID->psDev->sDeviceDesc.nDeviceID);
	kfree(pBuf);


	claim_device( g_nDeviceID, psDevice->nHandle, psHID->zName, DEVICE_INPUT );
	printk("%s on usb%d:%d.%d\n",
		 psHID->zName, psDevice->psBus->nBusNum, psDevice->nDeviceNum, nIfnum );

	/* Start interrupt */		 
	psHID->sIRQ.psDevice = psHID->psDev;
	if( g_psBus->submit_packet( &psHID->sIRQ ) )
		return -EIO;

	*pPrivate = psHID;
	return( true );
}


void usbhid_remove( USB_device_s* psDevice, void* pPrivate )
{
	
	struct USB_hid_s* psHID = pPrivate;
	release_device( psDevice->nHandle );
	uint32 i;
	uint32 j;
	g_psBus->cancel_packet( &psHID->sIRQ );
	for( i = 0; i < 256; i++ )
	{
		struct HID_report_s* psReport = &psHID->asReport[i];
		if( psReport->pBuffer != NULL )
			kfree( psReport->pBuffer );
		for( j = 0; j < psReport->nNumFields; j++ )
		{
			if( psReport->apsField[j]->pLastValues != NULL )
				kfree( psReport->apsField[j]->pLastValues );
			kfree( psReport->apsField[j] );
		}
	}
	kfree( psHID );
}

USB_driver_s* g_pcDriver;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
	/* Get USB bus */
	g_psBus = get_busmanager( USB_BUS_NAME, USB_BUS_VERSION );
	if( g_psBus == NULL ) {
		return( -1 );
	}
	
	g_nDeviceID = nDeviceID;
	
	/* Register */
	g_pcDriver = ( USB_driver_s* )kmalloc( sizeof( USB_driver_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	
	strcpy( g_pcDriver->zName, "USB hid" );
	g_pcDriver->AddDevice = usbhid_add;
	g_pcDriver->RemoveDevice = usbhid_remove;
	
	/*if(*/ g_psBus->add_driver_resistant( g_pcDriver );/* != 0 )*/
/*	{
		kfree( g_pcDriver );
		return( -1 );
	}*/
	
	
	g_hMouseWait = create_semaphore( "usbmouse_wait", 0, 0 );
	memset( g_pMouseBuffer, 0, 4 );
	
	g_hKeybWait = create_semaphore( "usbkeyb_wait", 0, 0 );
	memset( g_pKeybBuffer, 0, USB_KEYB_BUF_SIZE );

	
	create_device_node( nDeviceID, -1, "input/usb_mouse", &g_sMouseOperations, NULL );
	create_device_node( nDeviceID, -1, "input/usb_keyboard", &g_sKeybOperations, NULL );
    return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	if( g_psBus )
		g_psBus->remove_driver( g_pcDriver );
	delete_semaphore( g_hMouseWait );
	delete_semaphore( g_hKeybWait );
    return( 0 );
}
































































