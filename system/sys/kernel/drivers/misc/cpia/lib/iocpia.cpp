// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ioctl.h>
//-------------------------------------
//-------------------------------------
#include "../driver/cpia.h"
#include "cpiacam.h"
//-----------------------------------------------------------------------------

status_t CPiACam::IOGetVersion( uint8 *fwver, uint8 *fwrev, uint8 *vcver, uint8 *vcrev )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CPIA_ADDR;
    cmd.proc = GET_CPIA_VERSION;
    cmd.read = true;
    cmd.len = 8;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	*fwver = cmd.data.In.GetCPIAVersion.FW_Version;
	*fwrev = cmd.data.In.GetCPIAVersion.FW_Revision;
	*vcver = cmd.data.In.GetCPIAVersion.VC_Version;
	*vcrev = cmd.data.In.GetCPIAVersion.VC_Revision;
    }

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGetVersion" );
#endif
	
    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOGetPnpId( uint16 *vendor, uint16 *product, uint16 *device )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CPIA_ADDR;
    cmd.proc = GET_PNP_ID;
    cmd.read = true;
    cmd.len = 8;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	*vendor		= cmd.data.In.GetPnPID.VendorMSB<<8  | cmd.data.In.GetPnPID.VendorLSB;
	*product	= cmd.data.In.GetPnPID.ProductMSB<<8 | cmd.data.In.GetPnPID.ProductLSB;
	*device		= cmd.data.In.GetPnPID.DeviceMSB<<8  | cmd.data.In.GetPnPID.DeviceLSB;
    }

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGetPnpId" );
#endif
	
    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOGetCameraStatus( uint8 *systemstate, uint8 *vpctrlstate, uint8 *streamstate, uint8 *fatalerror, uint8 *cmderror, uint8 *debugflags, uint8 *vpstatus, uint8 *errorcode )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CPIA_ADDR;
    cmd.proc = GET_CAMERA_STATUS;
    cmd.read = true;
    cmd.len = 8;
	
    status_t status = DoIOCTL( &cmd );

    if( status >= 0 )
    {
	if( systemstate ) *systemstate = cmd.data.In.GetCameraStatus.SystemState;
	if( vpctrlstate ) *vpctrlstate = cmd.data.In.GetCameraStatus.VPCtrlState;
	if( streamstate ) *streamstate = cmd.data.In.GetCameraStatus.StreamState;
	if( fatalerror )  *fatalerror  = cmd.data.In.GetCameraStatus.FatalError;
	if( cmderror )    *cmderror    = cmd.data.In.GetCameraStatus.CmdError;
	if( debugflags )  *debugflags  = cmd.data.In.GetCameraStatus.DebugFlags;
	if( vpstatus )    *vpstatus    = cmd.data.In.GetCameraStatus.VPStatus;
	if( errorcode )   *errorcode   = cmd.data.In.GetCameraStatus.ErrorCode;
    }

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOGotoHiPower()
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CPIA_ADDR;
    cmd.proc = GOTO_HI_POWER;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGotoHiPower" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOGotoLoPower()
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CPIA_ADDR;
    cmd.proc = GOTO_LO_POWER;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGotoLoPower" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define CPIA_UNUSED_0			0x06
#define GOTO_SUSPEND			0x07
#define GOTO_PASS_THROUGH		0x08
#define ENABLE_WATCHDOG			0x09	/* not yet implemented */
#define MODIFY_CAMERA_STATUS 	0x0A

//-----------------------------------------------------------------------------
