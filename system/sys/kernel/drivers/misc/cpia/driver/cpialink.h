/* Copyright 1999 VLSI Vision Ltd. Free for use under the Gnu Public License */
#ifndef CPIAPPCDRIVER_CPIALINK_H
#define CPIAPPCDRIVER_CPIALINK_H
/*
	990808 : JH : Fixed all the types, to match the ones Be use.
*/

//#include <support/SupportDefs.h>
#include "pport.h"
//#include "recreated.h"

#define MAX_LPT_PORTS			3

#define MAX_1284_DEVICE_ID		256


typedef enum {
	NO_ERROR,
	NO_PORT_ERROR,
	SET_TRANSFER_MODE_ERROR, 

	END_TRANSFER_MODE_ERROR,
	END_FAST_MODE_ERROR,

	FORWARD2REVERSE_ERROR,
	SIM_ECP_READ_ERROR,
	NIBBLE_READ_ERROR

} CLINK_ERROR;	 



//bool CLINK_Init( void );
//void CLINK_Fini( void );


//bool CLINK_DetectPort( uint8 PortNumber, uint16 *BaseAddr, uint8 *PortType );
//bool CLINK_SelectPort( uint8 PortNumber );	/* will initialise port the first time its selected */

//bool CLINK_GetSelectedPort( uint8 *PortNumber );


bool CLINK_SendWakeup( PPORT *Port );
bool CLINK_Get1284DeviceID( PPORT *Port, char *DeviceID, uint16 MaxLength ); 	/* use small MaxLength to 		*/ 
																/* check if camera alive		*/

bool CLINK_ForceWatchdogReset( PPORT *Port );

 
bool CLINK_GetPortInfo( PPORT *Port, uint16 *BaseAddr, uint8 *PortType, uint16 *Timeout_mS );
bool CLINK_SetPortInfo( PPORT *Port, uint8 PortType, uint16 Timeout_mS );


bool CLINK_TransferMsg( PPORT *Port, uint8 Module, uint8 Proc, uint8 *CmdData, bool DataReadFlag, uint16 DataBytes, uint8 *DataBuffer );

bool CLINK_UploadStreamData( PPORT *Port, bool EndOnFF, uint32 DataBytes, uint8 *DataBuffer, uint32 *ActualBytes, uint32 StreamChunkSize );

/* private functions */
int Get1284DeviceID( PPORT *Port, char *DestString );

#endif
