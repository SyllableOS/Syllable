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

status_t CPiACam::IOReadVCRegs(
	uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3,
	uint8 *reg0, uint8 *reg1, uint8 *reg2, uint8 *reg3 )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = READ_VC_REGS;
    cmd.read = true;
    cmd.len = 8;
    cmd.cmd.ReadVCRegs.Addr0 = addr0;
    cmd.cmd.ReadVCRegs.Addr0 = addr1;
    cmd.cmd.ReadVCRegs.Addr0 = addr2;
    cmd.cmd.ReadVCRegs.Addr0 = addr3;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	if( reg0 ) *reg0 = cmd.data.In.ReadVCRegs.Reg0;
	if( reg1 ) *reg1 = cmd.data.In.ReadVCRegs.Reg1;
	if( reg2 ) *reg2 = cmd.data.In.ReadVCRegs.Reg2;
	if( reg3 ) *reg3 = cmd.data.In.ReadVCRegs.Reg3;
    }
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOReadVCRegs" );
#endif

    return status;
}

status_t CPiACam::IOWriteVCReg( uint8 addr, uint8 andmask, uint8 ormask )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = WRITE_VC_REG;
    cmd.read = false;
    cmd.cmd.WriteVCReg.Addr = addr;
    cmd.cmd.WriteVCReg.AndMask = andmask;
    cmd.cmd.WriteVCReg.OrMask = ormask;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOWriteVCReg" );
#endif

    return status;
}

status_t CPiACam::IOReadMCPorts( uint8 *port0, uint8 *port1, uint8 *port2, uint8 *port3 )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = READ_MC_PORTS;
    cmd.read = true;
    cmd.len = 8;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	if( port0 ) *port0 = cmd.data.In.ReadMCPorts.Port0;
	if( port1 ) *port1 = cmd.data.In.ReadMCPorts.Port1;
	if( port2 ) *port2 = cmd.data.In.ReadMCPorts.Port2;
	if( port3 ) *port3 = cmd.data.In.ReadMCPorts.Port3;
    }

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOReadMCPorts" );
#endif

    return status;
}

status_t CPiACam::IOWriteMCPort( uint8 port, uint andmask, uint8 ormask )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = WRITE_MC_PORT;
    cmd.read = false;
    cmd.cmd.WriteMCPort.Port = port;
    cmd.cmd.WriteMCPort.AndMask = andmask;
    cmd.cmd.WriteMCPort.OrMask = ormask;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
	
#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOWriteMCPort" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define SET_BAUD_RATE			0x05

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetECPTiming( uint8 slowtiming )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = SET_ECP_TIMING;
    cmd.read = false;
    cmd.cmd.SetECPTiming.SlowTiming = slowtiming;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOSetECPTiming" );
#endif

    return status;
}

status_t CPiACam::IOReadIDATA(
	uint8 addr0, uint8 addr1, uint8 addr2, uint8 addr3,
	uint8 *data0, uint8 *data1, uint8 *data2, uint8 *data3 )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = READ_IDATA;
    cmd.read = true;
    cmd.len = 8;
    cmd.cmd.ReadIDATA.Addr0 = addr0;
    cmd.cmd.ReadIDATA.Addr1 = addr1;
    cmd.cmd.ReadIDATA.Addr2 = addr2;
    cmd.cmd.ReadIDATA.Addr3 = addr3;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );
    if( status >= 0 )
    {
	if( data0 ) *data0 = cmd.data.In.ReadIDATA.Byte0;
	if( data1 ) *data1 = cmd.data.In.ReadIDATA.Byte1;
	if( data2 ) *data2 = cmd.data.In.ReadIDATA.Byte2;
	if( data3 ) *data3 = cmd.data.In.ReadIDATA.Byte3;
    }

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOReadIDATA" );
#endif
	
    return status;
}

status_t CPiACam::IOWriteIDATA( uint8 addr, uint8 andmask, uint8 ormask )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = SYSTEM_ADDR;
    cmd.proc = WRITE_IDATA;
    cmd.read = false;
    cmd.cmd.WriteIDATA.Addr = addr;
    cmd.cmd.WriteIDATA.AndMask = andmask;
    cmd.cmd.WriteIDATA.OrMask = ormask;
	
    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOWriteIDATA" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define GENERIC_CALL			0x09
#define SERIAL_START			0x0A
#define SERIAL_STOP				0x0B
#define SERIAL_WRITE			0x0C
#define SERIAL_READ				0x0D

//-----------------------------------------------------------------------------

