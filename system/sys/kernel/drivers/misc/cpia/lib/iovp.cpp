// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <sys/ioctl.h>
//-------------------------------------
//-------------------------------------
#include "../driver/cpia.h"
#include "cpiacam.h"
//-----------------------------------------------------------------------------

#define GET_VP_VERSION			0x01
#define RESET_FRAME_COUNTER		0x02

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetColorParms( uint8 brightness, uint8 contrast, uint8 saturation )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = SET_COLOUR_PARAMS;
    cmd.read = false;
    cmd.cmd.SetColourParams.Brightness = brightness;
    cmd.cmd.SetColourParams.Contrast = contrast;
    cmd.cmd.SetColourParams.Saturation = saturation;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetColorParms" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetExposure(
    uint8 gainmode, uint8 expmode, uint8 compmode, uint8 centreweight,
    uint8 gain, uint8 fineexp, uint8 coarseexplo, uint8 coarseexphi, 
    int8 redcomp, int8 green1comp, int8 green2comp, int8 bluecomp )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module					= VP_CTRL_ADDR;
    cmd.proc					= SET_EXPOSURE;
    cmd.cmd.SetExposure.GainMode		= gainmode;
    cmd.cmd.SetExposure.ExpMode			= expmode;
    cmd.cmd.SetExposure.CompMode		= compmode;
    cmd.cmd.SetExposure.CentreWeight		= centreweight;
    cmd.read					= false;
    cmd.len					= 8;
    cmd.data.Out.SetExposure.Gain 		= gain;
    cmd.data.Out.SetExposure.FineExp		= fineexp;
    cmd.data.Out.SetExposure.CoarseExpLo	= coarseexplo;
    cmd.data.Out.SetExposure.CoarseExpHi	= coarseexphi;
    cmd.data.Out.SetExposure.RedComp		= redcomp;
    cmd.data.Out.SetExposure.Green1Comp		= green1comp;
    cmd.data.Out.SetExposure.Green2Comp		= green2comp;
    cmd.data.Out.SetExposure.BlueComp		= bluecomp;
		
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetExposure" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define VP_SPARE_CMD  			0x05	/* used to be SET_GAMMA */	

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetColorBalance(
    uint8 balancemode, uint8 redgain, uint8 greengain, uint8 bluegain )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = SET_COLOUR_BALANCE;
    cmd.read = false;
    cmd.cmd.SetColourBalance.BalanceMode = balancemode;
    cmd.cmd.SetColourBalance.RedGain = redgain;
    cmd.cmd.SetColourBalance.GreenGain = greengain;
    cmd.cmd.SetColourBalance.BlueGain = bluegain;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetColorBalance" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetSensorFPS( uint8 clrdiv, uint8 rate )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = SET_SENSOR_FPS;
    cmd.read = false;
    cmd.cmd.SetSensorFPS.SensorClkDivisor = clrdiv;
    cmd.cmd.SetSensorFPS.SensorBaseRate = rate;
    cmd.cmd.SetSensorFPS.PhaseOverride = 0; // not ducumented!!?!
    cmd.cmd.SetSensorFPS.NewPhase = 0; // not ducumented!!?!
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetSensorFPS" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetVPDefaults()
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module	= VP_CTRL_ADDR;
    cmd.proc	= SET_VP_DEFAULTS;
    cmd.read	= false;
		
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetVPDefaults" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetApcor( uint8 gain1, uint8 gain2, uint8 gain4, uint8 gain8 )
{
    assert( 0 );

    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = SET_APCOR;
    cmd.read = false;
    cmd.cmd.SetApcor.gain1 = gain1;
    cmd.cmd.SetApcor.gain2 = gain2;
    cmd.cmd.SetApcor.gain4 = gain4;
    cmd.cmd.SetApcor.gain8 = gain8;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetApcor" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define FLICKER_CTRL			0x0A

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetVLOffset( uint8 gain1, uint8 gain2, uint8 gain4, uint8 gain8 )
{
    assert( 0 );

    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = SET_VLOFFSET;
    cmd.read = false;
    cmd.cmd.SetVLOffset.gain1 = gain1;
    cmd.cmd.SetVLOffset.gain2 = gain2;
    cmd.cmd.SetVLOffset.gain4 = gain4;
    cmd.cmd.SetVLOffset.gain8 = gain8;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetVLOffset" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define GET_COLOUR_PARAMS		0x10

//-----------------------------------------------------------------------------

status_t CPiACam::IOGetColorBalance( uint8 *red, uint8 *green, uint8 *blue )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = GET_COLOUR_BALANCE;
    cmd.read = true;
    cmd.len = 8;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

    if( status >= 0 )
    {
	if( red )     	*red = cmd.data.In.GetColourBalance.RedGain;
	if( green )    	*green = cmd.data.In.GetColourBalance.GreenGain;
	if( blue )     	*blue = cmd.data.In.GetColourBalance.BlueGain;
    }
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGetColorBalance" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define GET_COLOUR_PARAMS		0x10
#define GET_COLOUR_BALANCE		0x11

//-----------------------------------------------------------------------------

status_t CPiACam::IOGetExposure( uint8 *gain, uint8 *fineexp, uint8 *coarseexplo, uint8 *coarseexphi, int8 *redcomp, int8 *green1comp, int8 *green2comp, int8 *bluecomp )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = GET_EXPOSURE;
    cmd.read = true;
    cmd.len = 8;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

    if( status >= 0 )
    {
	if( gain )        *gain        = cmd.data.In.GetExposure.Gain;
	if( fineexp )     *fineexp     = cmd.data.In.GetExposure.FineExp;
	if( coarseexplo ) *coarseexplo = cmd.data.In.GetExposure.CoarseExpLo;
	if( coarseexphi ) *coarseexphi = cmd.data.In.GetExposure.CoarseExpHi;
	if( redcomp )     *redcomp     = cmd.data.In.GetExposure.RedComp;
	if( green1comp )  *green1comp  = cmd.data.In.GetExposure.Green1Comp;
	if( green2comp )  *green2comp  = cmd.data.In.GetExposure.Green2Comp;
	if( bluecomp )    *bluecomp    = cmd.data.In.GetExposure.BlueComp;
    }
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGetExposure" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define SET_SENSOR_MATRIX		0x13
#define COLOUR_BARS				0x1D

//-----------------------------------------------------------------------------

status_t CPiACam::IOReadVPRegs(
	uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3,
	uint8 *reg0, uint8 *reg1, uint8 *reg2, uint8 *reg3 )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = READ_VP_REGS;
    cmd.read = true;
    cmd.len = 8;
    cmd.cmd.ReadVPRegs.VPAddr0 = addr0;
    cmd.cmd.ReadVPRegs.VPAddr1 = addr1;
    cmd.cmd.ReadVPRegs.VPAddr2 = addr2;
    cmd.cmd.ReadVPRegs.VPAddr3 = addr3;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	if( reg0 ) *reg0 = cmd.data.In.ReadVPRegs.VPReg0;
	if( reg1 ) *reg1 = cmd.data.In.ReadVPRegs.VPReg1;
	if( reg2 ) *reg2 = cmd.data.In.ReadVPRegs.VPReg2;
	if( reg3 ) *reg3 = cmd.data.In.ReadVPRegs.VPReg3;
    }

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOReadVPRegs" );
#endif
	
    return status;
}

status_t CPiACam::IOWriteVPReg( uint8 addr, uint8 andmask, uint8 ormask )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = VP_CTRL_ADDR;
    cmd.proc = WRITE_VP_REG;
    cmd.read = false;
    cmd.cmd.WriteVPReg.VPAddr = addr;
    cmd.cmd.WriteVPReg.AndMask = andmask;
    cmd.cmd.WriteVPReg.OrMask = ormask;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOWriteVPReg" );
#endif
	
    return status;
}

//-----------------------------------------------------------------------------

