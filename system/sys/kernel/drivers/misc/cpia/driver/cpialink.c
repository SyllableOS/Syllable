/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */

/*
	990808 : JH : Fixed all the types, to match the ones Be use.
	              Because the BeOS can handle multiple threads calling the
	              same devicedriver at the same time, i removed the
	              static gCurrPort and added a PPORT argument to the functions instead.
*/

//#include <string.h>
//#include <kernel/OS.h>
//#include <drivers/KernelExport.h>

//#include "vvl_defs.h"
#include "pport.h"
#include "packet.h"
#include "cmd.h"
#include "cpialink.h"
#include "driver.h"

//PPORT gPort[ MAX_LPT_PORTS+1 ];			  /* entry [0] not used !!! */
//BOOL gPortInitialised[ MAX_LPT_PORTS+1 ]; /* entry [0] not used !!! */
//
//PPORT *gCurrPort;

#if 0
bool CLINK_Init( void )
{
	uint8 i;

	INFO(( "CLINK_Init: in\n" ));

	for( i = 0; i < MAX_LPT_PORTS; i++ ) {
		memset( &gPort[ i ], 0, sizeof( PPORT ) );
		gPortInitialised[ i ] = FALSE;
	}
	
	gCurrPort = NULL;

	INFO(( "CLINK_Init: out(ok)\n" ));
	return( TRUE );
}
#endif

#if 0
void CLINK_Fini( void ) {
	uint8 i;

	for( i = 0; i < MAX_LPT_PORTS; i++ )
		if ( gPortInitialised[ i ] ) {
			ClosePPort( &gPort[ i ] );
			gPortInitialised[ i ] = FALSE;
		}
	gCurrPort = NULL;

	return;
}
#endif

#if 0
bool CLINK_DetectPort( uint16 IOPort, uint16 *BaseAddr, uint8 *PortType )
{
	INFO(( "CLINK_DetectPort:in\n" ));

 	if ( !DetectPortType( IOPort, &gPort[PortNumber] ) ) {
        dprintf("CLINK_DetectPort: LPT%d: not found\n", PortNumber );
		*BaseAddr = 0;
		*PortType = 0;
	dprintf( "CLINK_DetectPort:out\n" );
		return( FALSE );
	}
	*BaseAddr = gPort[PortNumber].BaseAddr;
	*PortType = gPort[PortNumber].PortType;

	INFO(( "CLINK_DetectPort:out(ok)\n" ));
	return( TRUE );
}
#endif

#if 0
bool CLINK_SelectPort( uint8 PortNumber )
{
	/* will initialise port the first time its selected */
	INFO(( "CLINK_SelectPort: in\n" ));

	if ( gPortInitialised[ PortNumber ] == FALSE ) {
	 	if ( !DetectPortType( PortNumber, &gPort[PortNumber] ) ) {
	        dprintf("CLINK_SelectPort: LPT%d: not found\n", PortNumber );
			INFO(( "CLINK_SelectPort: out(err)\n" ));
			return( FALSE );
		}
		if ( !InitialisePPort( &gPort[PortNumber] ) ) {
	        dprintf("CLINK_SelectPort: unable to initialise LPT%d\n", PortNumber );
			INFO(( "CLINK_SelectPort: out(err)\n" ));
			return( FALSE );
		}
	}

	gPortInitialised[ PortNumber ] = TRUE;
	gCurrPort = &gPort[ PortNumber ];

	INFO(( "CLINK_SelectPort: out(ok)\n" ));
	return( TRUE );
}
#endif

#if 0
bool CLINK_GetSelectedPort( uint8 *PortNumber ) {

	if ( gCurrPort == NULL ) return( FALSE );

	*PortNumber = gCurrPort->PortNumber;

	return( TRUE );
}
#endif

bool CLINK_SendWakeup( PPORT *Port ) {
	uint8 VPP[8] = { VPP_PREAMBLE0, 
					VPP_PREAMBLE1,
					VPP_PREAMBLE2,
					VPP_PREAMBLE3,
					VPP_PREAMBLE4,
					VPP_PREAMBLE5,
					VPP_MANUAL_CMD,
					VPP_TERMINATOR };
	SendVPP( Port, 8, &VPP[0] );
	return( TRUE );
}


bool CLINK_Get1284DeviceID( PPORT *Port, char *DeviceID, uint16 MaxLength )
{ 	/* use small MaxLength to 		*/ 
	/* check if camera alive		*/
	
 	char nibble_string[ MAX_1284_DEVICE_ID ];

	INFO(( "cpia:CLINK_Get1284DeviceID(): in\n" ));
 
 	if ( !Get1284DeviceID( Port, nibble_string ) )
	{
		INFO(( "cpia:CLINK_Get1284DeviceID(): out(err)\n" ));
		return( FALSE );
	}

   	strncpy( DeviceID, nibble_string, MaxLength );

	INFO(( "cpia:CLINK_Get1284DeviceID(): out(ok)\n" ));
	return( TRUE );
}


bool CLINK_ForceWatchdogReset( PPORT *Port ) 
{
//	bigtime_t start;
	uint8 buff[4];

	INFO(( "cpia:CLINK_ForceWatchdogReset(): in\n" ));

	if ( !SetTransferMode( Port, (REQUEST_DEVICE_ID | NIBBLE_TRANSFER) ) )
	{
		INFO(( "cpia:CLINK_ForceWatchdogReset(): out(err)\n" ));
		 return( FALSE );
	}

	if ( !NibbleReadBuffer( Port, (uint8*)buff, 4 ) ) 
	{
		INFO(( "cpia:CLINK_ForceWatchdogReset(): out(err)\n" ));
		return( FALSE );
	}

	/* now wait long enough to guarantee the CPIA Watchdog Timer will 	*/
	/* have timed out and reset the board. 							 	*/
	snooze( 1500000 ); // 1.5s

	EndTransferMode( Port );

	INFO(( "cpia:CLINK_ForceWatchdogReset(): out(ok)\n" ));
	return( TRUE );
}
 
bool CLINK_GetPortInfo( PPORT *Port, uint16 *PortAddr, uint8 *PortType, uint16 *Timeout_mS ) {
	
	if ( !GetTimeout( Port, Timeout_mS ) ) return( FALSE );

	*PortAddr = Port->BaseAddr;
	*PortType = Port->PortType;

//	dprintf("GetPortInfo: Whileout = %lu\n", gCurrPort->Whileout );

	return( TRUE );
}

bool CLINK_SetPortInfo( PPORT *Port, uint8 PortType, uint16 Timeout_mS )
{
	INFO(( "cpia:CLINK_SetPortInfo(): in\n" ));

	if ( !SetTimeout( Port, Timeout_mS ) )
	{
		INFO(( "cpia:CLINK_SetPortInfo(): out(err)\n" ));
		return( FALSE );
	}
	Port->PortType = PortType;

	INFO(( "cpia:CLINK_SetPortInfo(): out(ok)\n" ));
	return( TRUE );
}

int Get1284DeviceID( PPORT *Port, char *DestString )
{
	uint16 tmp;
	uint8  length[20];

	INFO(( "cpia:Get1284DeviceID(): in\n" ));
	
	DestString[0] = '\0';

	if ( !SetTransferMode( Port, (REQUEST_DEVICE_ID | NIBBLE_TRANSFER) ) )
	{
		INFO(( "cpia:Get1284DeviceID(): out(err)\n" ));
		return( FALSE );
	}

	if ( !NibbleReadBuffer( Port, (uint8*)length, 2 ) ) {
//		dprintf("Get1284DeviceID:  NibbleReadBuffer returned an error\n" );
		EndTransferMode( Port );
		DestString[0] = '\0';
		INFO(( "cpia:Get1284DeviceID(): out(err)\n" ));
		return( FALSE );
	}

	tmp = length[0]*256 + length[1];

	if ( tmp < 3 )
	{
		EndTransferMode( Port );
		INFO(( "cpia:Get1284DeviceID(): out(err)\n" ));
		return( FALSE );
	}
	tmp = tmp - 2;			/* reduce count to remove length BYTE s */
	if ( tmp >= MAX_1284_DEVICE_ID ) tmp = MAX_1284_DEVICE_ID - 1;

	if ( !NibbleReadBuffer( Port, (uint8*)DestString, tmp ) )
	{
		DestString[0] = '\0';
		EndTransferMode( Port );
		INFO(( "cpia:Get1284DeviceID(): out(err)\n" ));
		return( FALSE );
	}
	DestString[ tmp ] = '\0';
	
	if ( !EndTransferMode( Port ) ) return( FALSE );

	INFO(( "cpia:Get1284DeviceID(): out(ok)\n" ));
 	return( TRUE );
}

bool CLINK_TransferMsg( PPORT *Port, uint8  Module, uint8  Proc, uint8 *CmdData,
					bool DataReadFlag, uint16 DataBytes, uint8 *DataBuffer ) {

	uint16 gDataPacketsPending;
	uint8  Cmd[PACKET_LEN];
 	uint8 *DataPtr;

 	Cmd[ CMD_TYPE ] = CAMERA_REQUEST;
	if ( DataReadFlag ) Cmd[ CMD_TYPE ] |= READ_DATA_REQUEST;
	
	Cmd[ CMD_DEST_ADDR ] = (uint8)((Module & MODULE_ADDR_BITS) | (Proc & PROC_ADDR_BITS));

	if ( CmdData != NULL ) {
		Cmd[ CMD_D0 ] = CmdData[0];
		Cmd[ CMD_D1 ] = CmdData[1];
		Cmd[ CMD_D2 ] = CmdData[2];
		Cmd[ CMD_D3 ] = CmdData[3];
	}
	else {
		Cmd[ CMD_D0 ] = 0; Cmd[ CMD_D1 ] = 0;
		Cmd[ CMD_D2 ] = 0; Cmd[ CMD_D3 ] = 0;
	}

	Cmd[ CMD_DATA_BYTES_LO ] = (uint8)(DataBytes&0xFF);
	Cmd[ CMD_DATA_BYTES_HI ] = (uint8)(DataBytes>>8);

//	dprintf( ">>%02X %02X %02X %02X %02X %02X %02X %02X\n", (int)Cmd[0],(int)Cmd[1], (int)Cmd[2], (int)Cmd[3], (int)Cmd[4], (int)Cmd[5], (int)Cmd[6], (int)Cmd[7] );

	DataPtr = (uint8*)DataBuffer;

	if ( !SetTransferMode( Port, ECP_TRANSFER ) ) {
		ERROR(( "cpia:TransferMsg: SetTransferMode failed\n" ));
		goto TransferError;
	}
	if ( Port->PortType & ECP_PORT_TYPE ) {
		if ( !ECPWriteBuffer( Port, (uint8*)Cmd, (uint32)PACKET_LEN ) ) {
			ERROR(( "cpia:TransferMsg: ECPWriteBuffer failed\n" ));
			EndTransferMode( Port );
			goto TransferError;
		}
	}
	else {
		if ( !SimECPWriteBuffer( Port, (uint8*)Cmd, (uint32)PACKET_LEN ) ) {
			ERROR(( "cpia:TransferMsg: SimECPWriteBuffer failed\n" ));
			EndTransferMode( Port );
			goto TransferError;
		}
	}
	if( !EndTransferMode( Port ) ) {
		ERROR(( "cpia:TransferMsg: EndTransferMode failed\n" ));
		goto TransferError;
	}

	if ( DataBytes !=0 ) {
		gDataPacketsPending = DataBytes / PACKET_LEN;

		while( gDataPacketsPending ) {

			if ( !DataReadFlag ) {
				/* write data packet to camera */	    
				if ( !SetTransferMode( Port, ECP_TRANSFER ) ) {
					ERROR(( "cpia:TransferMsg: SetTransferMode failed\n" ));
					ERROR(( "cpia:TransferMsg: gDataPacketsPending = %u\n", gDataPacketsPending ));
					goto TransferError;
				}
				if ( Port->PortType & ECP_PORT_TYPE ) {
					if ( !ECPWriteBuffer( Port, DataPtr, (uint32)PACKET_LEN ) ) {
						ERROR(( "cpia:TransferMsg: ECPWriteBuffer failed\n" ));
						goto TransferError;
					}
				}
				else {
					if ( !SimECPWriteBuffer( Port, DataPtr, (uint32)PACKET_LEN ) ) {
						ERROR(( "cpia:TransferMsg: SimECPWriteBuffer failed\n" ));
						goto TransferError;
					}
				}
				if( !EndTransferMode( Port ) ) {
					ERROR(( "cpia:TransferMsg: EndTransferMode failed\n" ));
					goto TransferError;
				}
			}
			else {
				/* read data packet from camera */	 
 				if ( Port->PortType & BIDIR_PORT_TYPE )	{
					if ( !SetTransferMode( Port, ECP_TRANSFER ) ) {
 						ERROR(( "cpia:TransferMsg: SetTransferMode failed\n" ));
						goto TransferError;
					}
					if ( !Forward2Reverse( Port ) ) {
						EndTransferMode( Port );
						goto TransferError;
					}
					if ( Port->PortType & ECP_PORT_TYPE ) {
						if ( !ECPReadBuffer( Port, DataPtr, (uint32)PACKET_LEN ) ) {
							ERROR(( "cpia:TransferMsg: ECPReadBuffer failed\n" ));
							goto TransferError;
						}
					}
					else {
						if ( !SimECPReadBuffer( Port, DataPtr, (uint32)PACKET_LEN ) ) {
							ERROR(( "cpia:TransferMsg: SimECPReadBuffer failed\n" ));
							goto TransferError;
						}
					}
					if( !EndTransferMode( Port ) ) {
						ERROR(( "cpia:TransferMsg: EndTransferMode failed\n" ));
						goto TransferError;
					}
				}
				else {
					if ( !SetTransferMode( Port, NIBBLE_TRANSFER ) ) {
						ERROR(( "cpia:TransferMsg: SetTransferMode failed\n" ));
						goto TransferError;
					}
					if ( !NibbleReadBuffer( Port, DataPtr, (uint32)PACKET_LEN ) ) {
						ERROR(( "cpia:TransferMsg: NibbleReadBuffer failed\n" ));
						goto TransferError;
					}
					if( !EndTransferMode( Port ) ) {
						ERROR(( "cpia:TransferMsg: EndTransferMode failed\n" ));
						goto TransferError;
					}
				}
			}
			gDataPacketsPending--;
			DataPtr += PACKET_LEN; 
		}
	}
	
	return( TRUE );

TransferError:
	return( FALSE );
}



bool CLINK_UploadStreamData( PPORT *Port, bool EndOnFF, uint32 DataBytes, uint8 *DataBuffer, uint32 *ActualBytes, uint32 StreamChunkSize )
{
	uint32 curr;

	if ( Port->PortType & BIDIR_PORT_TYPE )	{
		if ( !SetUploadMode( Port, ECP_UPLOAD, NORMAL_DATA_FLAG ) ) {
			ERROR(( "cpia:UploadStreamData: SetUploadMode failed\n" ));
			goto UploadError;
		}
		if ( !EndOnFF ) {
			if ( Port->PortType & ECP_PORT_TYPE ) {
				if ( !ECPReadBuffer( Port, DataBuffer, DataBytes ) ) {
					ERROR(( "cpia:UploadStreamData: ECPReadBuffer failed\n" ));
					EndUploadMode( Port );
					goto UploadError;
				}
			}
			else {
				if ( !SimECPReadBuffer( Port, DataBuffer, DataBytes ) ) {
					ERROR(( "cpia:UploadStreamData: SimECPReadBuffer failed\n" ));
					EndUploadMode( Port );
					goto UploadError;
				}
			}
			*ActualBytes = DataBytes;
		}
		else {
			if ( (StreamChunkSize < 256) || (StreamChunkSize > DataBytes) 
			|| ( (DataBytes % StreamChunkSize) != 0 ) ) return( FALSE );
			
			for( curr = 0; curr < DataBytes; curr += StreamChunkSize ) {			
				if ( Port->PortType & ECP_PORT_TYPE ) {
					if ( !ECPReadBuffer( Port, &DataBuffer[ curr ], StreamChunkSize ) ) {
						ERROR(( "cpia:UploadStreamData: ECPReadBuffer failed\n" ));
						EndUploadMode( Port );
						goto UploadError;
					}
				}
				else {
					if ( !SimECPReadBuffer( Port, &DataBuffer[ curr ], StreamChunkSize ) ) {
						ERROR(( "cpia:UploadStreamData: SimECPReadBuffer failed\n" ));
						EndUploadMode( Port );
						goto UploadError;
					}
				}

				*ActualBytes = curr + StreamChunkSize;

				if ( (DataBuffer[curr+StreamChunkSize-4] == 0xFF)
						&& (DataBuffer[curr+StreamChunkSize-3] == 0xFF) 
						&& (DataBuffer[curr+StreamChunkSize-2] == 0xFF)
						&& (DataBuffer[curr+StreamChunkSize-1] == 0xFF) ) break;
			}
		}
		if( !EndUploadMode( Port ) ) {
			ERROR(( "cpia:UploadStreamData: EndUploadMode failed\n" ));
			goto UploadError;
		}
	}
	else {
		if ( !SetUploadMode( Port, NIBBLE_UPLOAD, NORMAL_DATA_FLAG ) ) {
			ERROR(( "cpia:UploadStreamData: SetUploadMode failed\n" ));
			goto UploadError;
		}
		if ( !EndOnFF ) {
			if ( !NibbleUpload( Port, DataBuffer, DataBytes ) ) {
				ERROR(( "cpia:UploadStreamData: NibbleUpload failed\n" ));
				EndUploadMode( Port );
				goto UploadError;
			}
			*ActualBytes = DataBytes;
		}
  		else {
			for( curr = 0; curr < DataBytes; curr += StreamChunkSize ) {			
				if ( !NibbleUpload( Port, &DataBuffer[curr], StreamChunkSize ) ) {
					ERROR(( "cpia:UploadStreamData: NibbleUpload failed\n" ));
					EndUploadMode( Port );
					goto UploadError;
				}

				*ActualBytes = curr + StreamChunkSize;

				if ( (DataBuffer[curr+StreamChunkSize-4] == 0xFF)
						&& (DataBuffer[curr+StreamChunkSize-3] == 0xFF) 
						&& (DataBuffer[curr+StreamChunkSize-2] == 0xFF)
						&& (DataBuffer[curr+StreamChunkSize-1] == 0xFF) ) break;
			}
		}
  		if( !EndUploadMode( Port ) ) {
			ERROR(( "cpia:UploadStreamData: EndUploadMode failed\n" ));
			goto UploadError;
		}
	}

	return( TRUE );

UploadError:
	return( FALSE );
}
