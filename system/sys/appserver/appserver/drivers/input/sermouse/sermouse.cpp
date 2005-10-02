/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
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
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include <atheos/kernel.h>

#include <inputnode.h>

#include <appserver/protocol.h>
#include <util/message.h>

using namespace os;

static int g_nSerialDevice = -1;

class SerMouseDriver : public InputNode
{
public:
    SerMouseDriver();
    ~SerMouseDriver();

    virtual bool Start();
    virtual int  GetType() { return( IN_MOUSE ); }
  
private:
    static int32 EventLoopEntry( void* pData );
    void EventLoop();
    void DispatchEvent( int nDeltaX, int nDeltaY, uint32 nButtons );
  
    thread_id m_hThread;
    int	    m_nMouseDevice;
};



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SerMouseDriver::SerMouseDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SerMouseDriver::~SerMouseDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SerMouseDriver::DispatchEvent( int nDeltaX, int nDeltaY, uint32 nButtons )
{
    Point cDeltaMove( nDeltaX, nDeltaY );
    static uint32 nLastButtons = 0;
    uint32 	nButtonFlg;
  
    nButtonFlg	= nButtons ^ nLastButtons;
    nLastButtons	= nButtons;

    if ( nButtonFlg != 0 ) {
	Message* pcEvent;
    
	if ( nButtonFlg & 0x01 ) {
	    if ( nButtons & 0x01 ) {
		pcEvent = new Message( M_MOUSE_DOWN );
	    } else {
		pcEvent = new Message( M_MOUSE_UP );
	    }
	    pcEvent->AddInt32( "_button", 1 );
	    pcEvent->AddInt32( "_buttons", 1 ); // To be removed
	    EnqueueEvent( pcEvent );
	}
	if ( nButtonFlg & 0x02 ) {
	    if ( nButtons & 0x02 ) {
		pcEvent = new Message( M_MOUSE_DOWN );
	    } else {
		pcEvent = new Message( M_MOUSE_UP );
	    }
	    pcEvent->AddInt32( "_button", 2 );
	    pcEvent->AddInt32( "_buttons", 2 ); // To be removed
	    EnqueueEvent( pcEvent );
	}
    }
    if ( nDeltaX != 0 || nDeltaY != 0 ) {
	Message* pcEvent = new Message( M_MOUSE_MOVED );
	pcEvent->AddPoint( "delta_move", cDeltaMove );
	EnqueueEvent( pcEvent );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SerMouseDriver::EventLoop()
{
    char anBuf[4];
    int  nIndex = 0;
  
    for (;;)
    {
	uint8 nByte;
    
	if ( read( g_nSerialDevice, &nByte, 1 ) != 1 ) {
	    dbprintf( "Error: SerMouseDriver::EventLoop() failed to read from serial device\n" );
	    continue;
	}
	if ( nByte & 0x40 ) {
	    nIndex = 0;
	}
	if ( nIndex >= 4 ) {
	    continue;
	}
	anBuf[nIndex++] = nByte;

	if ( nIndex == 3 ) {
	    int x;
	    int y;
	    uint32 nButtons = 0;
      
	    x = (anBuf[1] & 0x3f) | ((anBuf[0] & 0x03) << 6 );
	    y = (anBuf[2] & 0x3f) | ((anBuf[0] & 0x0c) << 4 );
	    if ( x & 0x80 ) {
		x |= 0xffffff00;
	    }
	    if ( y & 0x80 ) {
		y |= 0xffffff00;
	    }
	    if ( (anBuf[0] & 0x10) ) {
		nButtons |= 0x02;
	    }
	    if ( (anBuf[0] & 0x20) ) {
		nButtons |= 0x01;
	    }
	    DispatchEvent( x, y, nButtons );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 SerMouseDriver::EventLoopEntry( void* pData )
{
    SerMouseDriver* pcThis = (SerMouseDriver*) pData;
  
    pcThis->EventLoop();
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool SerMouseDriver::Start()
{
    thread_id hEventThread;
    hEventThread = spawn_thread( "sermouse_event_thread", (void*)EventLoopEntry, 120, 0, this );
    resume_thread( hEventThread );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" bool init_input_node()
{
    g_nSerialDevice = open( "/dev/misc/com/1", O_RDWR | O_NONBLOCK );
    if ( g_nSerialDevice < 0 ) {
	dbprintf( "Error: serial mode driver failed to open serial device: %s\n", strerror( errno ) );
	return( false );
    }
    uint32 nValue;
      // DTR should always be set.
    nValue = TIOCM_DTR;
    ioctl( g_nSerialDevice, TIOCMBIS, &nValue );

    uint8 nByte;
      // Empty the input buffer.
    while( read( g_nSerialDevice, &nByte, 1 ) == 1 ) 
	  /*** EMPTY ***/;
      // Toggle the RTS line. This should cause the mouse to send an "M"
    nValue = TIOCM_RTS;
    ioctl( g_nSerialDevice, TIOCMBIS, &nValue );
    snooze( 100000 );
    ioctl( g_nSerialDevice, TIOCMBIC, &nValue );
    snooze( 100000 );
    ioctl( g_nSerialDevice, TIOCMBIS, &nValue );
    snooze( 100000 );

    bool bDetected = false;
  
    while( read( g_nSerialDevice, &nByte, 1 ) == 1 ) {
	if ( (nByte & 0x7f) == 'M' ) {
	    bDetected = true;
	}
    }
    close( g_nSerialDevice );

    if ( bDetected == false ) {
	return( false );
    }
  
    dbprintf( "Found serial mouse on port 2\n" );
      // Re-open the device in blocking mode
    g_nSerialDevice = open( "/dev/misc/com/1", O_RDWR );
    if ( g_nSerialDevice < 0 ) {
	dbprintf( "Error: serial mode driver failed to re-open serial device: %s\n", strerror( errno ) );
	return( false );
    }
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" int uninit_input_node()
{
    if ( g_nSerialDevice >= 0 ) {
	close( g_nSerialDevice );
    }
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" int get_node_count()
{
    return( 1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" InputNode* get_input_node( int nIndex )
{
    if ( nIndex != 0 ) {
	dbprintf( "Serial mouse driver: get_input_node() called with invalid index %d\n", nIndex );
	return( NULL );
    }
    return( new SerMouseDriver() );
}
