// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
//-------------------------------------
//-------------------------------------
#include "../driver/cpia.h"
#include "cpiacam.h"
//-----------------------------------------------------------------------------

status_t CPiACam::IOGrabFrame( uint8 startline )
{
    cpiacmd cmd;
    memset( &cmd, 0, sizeof(cmd) );
    cmd.module = CAPTURE_ADDR;
    cmd.proc = GRAB_FRAME;
    cmd.cmd.GrabFrame.StreamStartLine = startline;

    status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
    PrintCameraStatus( "CPiACam::IOGrabFrame" );
#endif

    return status;
}

//-----------------------------------------------------------------------------

#define UPLOAD_FRAME			0x02

//-----------------------------------------------------------------------------

status_t CPiACam::IOSetGrabMode( uint8 continuous )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_GRAB_MODE;
	cmd.cmd.SetGrabMode.ContinuousGrab = continuous;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetGrabMode" );
#endif

	return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOInitStreamCap( uint8 skipframes, uint8 startline )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = INIT_STREAM_CAP;
	cmd.cmd.InitStreamCap.SkipFrames = skipframes;
	cmd.cmd.InitStreamCap.StreamStartLine = startline;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOInitStreamCap" );
#endif

	return status;
}

status_t CPiACam::IOFiniStreamCap()
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = FINI_STREAM_CAP;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOFiniStreamCap" );
#endif

	return status;
}

status_t CPiACam::IOStartStreamCap()
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = START_STREAM_CAP;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOStartStreamCap" );
#endif

	return status;
}

status_t CPiACam::IOStopStreamCap()
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = STOP_STREAM_CAP;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOStopStreamCap" );
#endif

	return status;
}

status_t CPiACam::IOSetFormat( uint8 videosize, uint8 subsampling, uint8 yuvorder )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_FORMAT;
	cmd.cmd.SetFormat.VideoSize = videosize;
	cmd.cmd.SetFormat.SubSampling = subsampling;
	cmd.cmd.SetFormat.YUVOrder = yuvorder;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetFormat" );
#endif

	return status;
}

status_t CPiACam::IOSetROI( uint8 colstart, uint8 colend, uint rowstart, uint rowend )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_ROI;
	cmd.cmd.SetROI.ColStart = colstart;
	cmd.cmd.SetROI.ColEnd = colend;
	cmd.cmd.SetROI.RowStart = rowstart;
	cmd.cmd.SetROI.RowEnd = rowend;

	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetROI" );
#endif

	return status;
}

status_t CPiACam::IOSetCompression( uint8 compressmode, uint8 decimation )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_COMPRESSION;
	cmd.cmd.SetCompression.CompMode = compressmode;
	cmd.cmd.SetCompression.Decimation = decimation;
	
	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetCompression" );
#endif

	return status;
}

status_t CPiACam::IOSetCompressionTarget( uint8 targetting, uint8 framerate, uint8 quality )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_COMPRESSION_TARGET;
	cmd.cmd.SetCompressionTarget.FRTargetting = targetting;
	cmd.cmd.SetCompressionTarget.TargetFR = framerate;
	cmd.cmd.SetCompressionTarget.TargetQ = quality;
	
	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetCompressionTarget" );
#endif

	return status;
}

status_t CPiACam::IOSetYUVThresh( uint8 ythresh, uint8 uvthresh )
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module = CAPTURE_ADDR;
	cmd.proc = SET_YUV_THRESH;
	cmd.cmd.SetYUVThresh.YThresh = ythresh;
	cmd.cmd.SetYUVThresh.UVThresh = uvthresh;
	
	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOSetYUVThresh" );
#endif

	return status;
}

//-----------------------------------------------------------------------------

#define SET_COMPRESSION_PARAMS  0x0D

//-----------------------------------------------------------------------------

status_t CPiACam::IODiscardFrame()
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module	= CAPTURE_ADDR;
	cmd.proc	= DISCARD_FRAME;
	cmd.read	= false;
	
	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IODiscardFrame" );
#endif

	return status;
}

//-----------------------------------------------------------------------------

status_t CPiACam::IOGrabReset()
{
	cpiacmd cmd;
	memset( &cmd, 0, sizeof(cmd) );
	cmd.module	= CAPTURE_ADDR;
	cmd.proc	= GRAB_RESET;
	cmd.read	= false;
	
	status_t status = ioctl( fFileHandle, CPIA_COMMAND, &cmd );

#ifdef SHOWSTATUS
	PrintCameraStatus( "CPiACam::IOGrabReset" );
#endif

	return status;
}

//-----------------------------------------------------------------------------



