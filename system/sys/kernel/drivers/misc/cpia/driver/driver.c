// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <posix/errno.h>
//-------------------------------------
#include <macros.h>
#include <atheos/kernel.h>
#include <atheos/atomic.h>
#include <atheos/time.h>
#include <posix/signal.h>
//-------------------------------------
#include "cpia.h"
#include "cpialink.h"
#include "pport.h"
#include "driver.h"
//-----------------------------------------------------------------------------

struct isa_module_info *isa = NULL;

#define MAXCAMS 8

typedef struct
{
    int		ioport;
    int		dmachannel;	
    int		interrupt;	

    int		inuse;

    PPORT	pport;
} cpiappc2;

//-----------------------------------------------------------------------------

static status_t cpia_open( void  *node, uint32 flags, void **cookie );
static status_t cpia_close( void *node, void *cookie );

static status_t cpia_control( void *node, void *cookie, uint32 msg, void *buf, size_t len );

static int cpia_read( void *node, void *cookie, off_t pos, void *buf, size_t len );

status_t OpenCam( cpiappc2 *cam );
void CloseCam( cpiappc2 *cam );

//-----------------------------------------------------------------------------

static DeviceOperations_s cpia_devices =
{
    cpia_open, 		/* called to open the device */
    cpia_close,		/* called to close the device */
    cpia_control, 	/* called to control the device */
    cpia_read,		/* reads from the device */
    NULL,		/* writes to the device */
};

//------------------------------------------------------------------------------

status_t device_init( int device_id )
{
    status_t status;

    cpiappc2 *cam = kmalloc( sizeof(cpiappc2), MEMF_KERNEL );
    cam->ioport = 0x378;
    cam->dmachannel = 3;
//    cam->dmachannel = -1;
    cam->interrupt = 7;
//    cam->interrupt = -1;
    cam->inuse = 0;

    status = OpenCam( cam );
    if( status >= 0 )
    {
	CloseCam( cam );
	status = create_device_node( device_id, "video/cpia/0", &cpia_devices, cam );
	if( status < 0 )
	    kfree( cam );
    }
    else
    {
	if( cam )
	    kfree( cam );
    }

    return 0;
}

status_t device_uninit( int device_id )
{
      // FIXME: the cookie is leaked.
    return 0 ;
}

//------------------------------------------------------------------------------

static status_t cpia_open( void  *node, uint32 flags, void **cookie )
{
    int status;
    int inuse;
    cpiappc2 *cam = (cpiappc2*)node;

    TOUCH(flags);

    FUNC(( "cpia:open()\n" ));
	
    inuse = atomic_add( &cam->inuse, 1 );
    if( inuse )
    {
	WARNING(( "cpia:open(): aready open\n" ));
	atomic_add( &cam->inuse, -1 );
	return -EBUSY;
    }

    status = OpenCam( cam );
    if( status < 0 )
    {
	ERROR(( "cpia:open(): could not find hardware!!?!\n" ));
	cam->inuse = 0;
	return -1;
    }

    return 0;
}

static status_t cpia_close( void *node, void *cookie )
{
    cpiappc2 *cam = (cpiappc2*)node;

    FUNC(( "cpia:close()\n" ));

    CloseCam( cam );
    cam->inuse = 0;

    return 0;
}

static status_t cpia_control( void *node, void *cookie, uint32 msg, void *buf, size_t len )
{
    status_t status;
    cpiappc2 *cam = (cpiappc2*)node;

    TOUCH(len);
	
    FUNC(( "cpia:control()\n" ));

//	cpia_cmd *cmd = (cpia_cmd*)buf;

    switch( msg )
    {
	case CPIA_COMMAND:
	{
	    cpiacmd *cmd;
			
	    if( buf == NULL )
	    {
		ERROR(( "cpia:control():illegal parameter (buf==NULL)\n" ));
		return -EINVAL;
	    }
			
	    cmd = (cpiacmd*)buf;
	    INFO(( "cpia:ctrl: module:%d proc:%d read:%d len:%d\n", cmd->module, cmd->proc, cmd->read, cmd->len ));
#if 1
	    if( (cmd->module&MODULE_ADDR_BITS) != cmd->module )
	    {
		ERROR(( "cpia:ctrl: illegal module: %02X\n", cmd->module ));
		return -EINVAL;
	    }
	    if( (cmd->proc&PROC_ADDR_BITS) != cmd->proc )
	    {
		ERROR(( "cpia:ctrl: illegal proc: %02X\n", cmd->proc ));
		return -EINVAL;
	    }
	    if( cmd->read!=0 && cmd->read!=1 )
	    {
		ERROR(( "cpia:ctrl: illegal read: %02X\n", cmd->read ));
		return -EINVAL;
	    }
	    if( cmd->len!=0 && cmd->len!=8 )
	    {
		ERROR(( "cpia:ctrl: illegal len: %02X\n", cmd->len ));
		return -EINVAL;
	    }
#endif
	    status = CLINK_TransferMsg( &cam->pport, cmd->module, cmd->proc, (uint8*)&cmd->cmd, cmd->read, cmd->len, (uint8*)&cmd->data );
	    if( !status )
	    {
		ERROR(( "cpia:control(): could not transfer message\n" ));
		return -1;
	    }
			
	    return 0;
	}
		
	case CPIA_RESET:
	{
	    status = CLINK_ForceWatchdogReset( &cam->pport );
	    if( !status )
	    {
		ERROR(( "cpia:control(): could not force reset\n" ));
		return -1;
	    }

	    return 0;
	}
		
	case CPIA_WAITFORFRAME:
	{
	    cpiacmd cmd;
	    bigtime_t timeout;
			
	    memset( &cmd, 0, sizeof(cmd) );
	    cmd.module = CPIA_ADDR;
	    cmd.proc = GET_CAMERA_STATUS;
	    cmd.read = true;
	    cmd.len = 8;

	    timeout = get_system_time() + 5*1000000; // we wait for a max of 5 secs.
	    while( 1 )
	    {
		status_t status = CLINK_TransferMsg( &cam->pport, cmd.module, cmd.proc, (uint8*)&cmd.cmd, cmd.read, cmd.len, (uint8*)&cmd.data );
		if( !status )
		{
		    ERROR(( "cpia:control(): error transfering message while waiting for frame\n" ));
		    return -1;
		}
			
		if( cmd.data.In.GetCameraStatus.StreamState == STREAM_READY ) break;
		if( cmd.data.In.GetCameraStatus.StreamState != STREAM_NOT_READY )
		{
		    ERROR(( "cpia:control(): got unexpected stream state: %d\n", cmd.data.In.GetCameraStatus.StreamState ));
		    return -1;
		}
		snooze( 20000 );

		if( get_system_time() > timeout )
		{
		    ERROR(( "cpia:control(): timeout while waiting for frame\n" ));
		    return -1;
		}

		if( is_signals_pending() )
		{
		    ERROR(( "cpia:control(): got signal while waiting for frame\n" ));
		    return -EINTR;
		}
	    }
	    return 0;
	}

	case CPIA_STOPSTREAM:
	case CPIA_STARTSTREAM:
	      // dummies on ppc hardware.
	    return 0;

    }
    return -EINVAL;
}

static int cpia_read( void *node, void *cookie, off_t pos, void *buf, size_t len )
{
	int status;
	uint32 actual = 0;
//	bigtime_t starttime,endtime;
//	int bps;
	cpiappc2 *cam = (cpiappc2*)node;

	TOUCH(pos);

	FUNC(( "cpia:read()\n" ));

//	starttime = system_time();

//	status = CLINK_UploadStreamData( &cam->pport, 1, *len, (uint8*)buf, &actual, 1024 );
	status = CLINK_UploadStreamData( &cam->pport, 1, len, (uint8*)buf, &actual, 1024*4 ); // NOTE: the dma buffer is only 4k!
//	status = CLINK_UploadStreamData( &cam->pport, 1, *len, (uint8*)buf, &actual, (*len)/4 );

//	endtime = system_time()+1; // make sure that we don't divide by zero!

	if( !status )
		ERROR(( "cpia:read(): could not read data\n" ));
	
//	bps = actual*1000000LL / (endtime-starttime);
//	INFO(( "%d.%d K/s\n", bps/1024, bps%1024 ));

	return status ? actual : -1;
}

//------------------------------------------------------------------------------

status_t OpenCam( cpiappc2 *cam )
{
    status_t status;
    char deviceid[MAX_1284_DEVICE_ID];

//	cpu_status state = disable_interrupts();

    INFO(( "cpia:OpenCam(): probing for camera on port %04X\n", cam->ioport ));

    if( !DetectPortType(cam->ioport, cam->dmachannel, cam->interrupt, &cam->pport) )
    {
	INFO(( "cpia:OpenCam(): no par-port found\n" ));
	goto error1;
    }

    WARNING(( "cpia:OpenCam(): port found, type=%s%s\n",
	     cam->pport.PortType&ECP_PORT_TYPE?"ECP/":"",
	     cam->pport.PortType&COMPAT_PORT_TYPE?"4 Bit":"" ));

    status = InitialisePPort( &cam->pport );
    if( !status )
    {
	INFO(( "cpia:OpenCam(): could not initalise port\n" ));
	goto error1;
    }

    status = CLINK_SetPortInfo( &cam->pport, cam->pport.PortType, 800 );
    if( !status )
    {
	INFO(( "cpia:OpenCam(): could not set timeout\n" ));
	goto error1;
    }

    status = CLINK_SendWakeup( &cam->pport );
    if( !status )
    {
	INFO(( "cpia:OpenCam(): send wakeup failed\n" ));
	goto error1;
    }

    status = CLINK_Get1284DeviceID( &cam->pport, deviceid, sizeof(deviceid) );
    if( !status )
    {
	INFO(( "cpia:OpenCam(): could not get 1284 device id\n" ));
	goto error1;
    }

    INFO(( "cpia:OpenCam(): 1284 deviceid: \"%s\"\n", deviceid ));

    if( strstr(deviceid, "PPC2 Camera") == NULL )
    {
	INFO(( "cpia:OpenCam(): found a 1284 device, but not a camera\n" ));
	goto error1;
    }

      // jahuu, we found a camera...
//	restore_interrupts( state );
    return 0;

error1:
    CloseCam( cam );
//error0:
//	restore_interrupts( state );
    return -1;
}

void CloseCam( cpiappc2 *cam )
{
    ClosePPort( &cam->pport );
}

