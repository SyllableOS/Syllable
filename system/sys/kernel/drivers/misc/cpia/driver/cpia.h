// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
#ifndef DAMN_CPIADEVICEDRIVER_CPIA_H
#define DAMN_CPIADEVICEDRIVER_CPIA_H
//-----------------------------------------------------------------------------
//-------------------------------------
#include <atheos/filesystem.h>
#include <atheos/types.h>
#include <atheos/device.h>
//-------------------------------------
#include "cmd.h"
//-----------------------------------------------------------------------------

#ifdef	__cplusplus
extern "C" {
#endif

    enum
    {
	CPIA_COMMAND = IOCTL_USER,
	CPIA_RESET,
	CPIA_STARTSTREAM,
	CPIA_STOPSTREAM,
	CPIA_WAITFORFRAME
    };

    typedef struct cpiacmd
    {
	uint8 		module;
	uint8 		proc;

	CMD_DATA	cmd;
	
	int   		read; // 0=write, 1=read
	int   		len;
	DATA_BUFFER	data;
    } cpiacmd;

#ifdef	__cplusplus
}
#endif

#endif

