/*
 *  The Syllable application server
 *	USB mouse appserver driver
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
 *
 */


 
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include <inputnode.h>

#include <appserver/protocol.h>
#include <util/message.h>
#include <atheos/kernel.h>

using namespace os;

static int g_nSerialDevice = -1;

class USBMouseDriver : public InputNode
{
public:
    USBMouseDriver();
    ~USBMouseDriver();

    virtual bool Start();
    virtual int  GetType() { return( IN_MOUSE ); }
  
private:
    static int32 EventLoopEntry( void* pData );
    void EventLoop();
    // (xxl) Added vertical and horizontal scroll parameters
    // Currently only the vertical scroll is implemented
    void DispatchEvent( int nDeltaX, int nDeltaY, uint32 nButtons, int nVScroll, int nHScroll );
    void SwitchToProtocol( int nProto );

    thread_id m_hThread;
    int	    m_nMouseDevice;
};



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

USBMouseDriver::USBMouseDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

USBMouseDriver::~USBMouseDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void USBMouseDriver::DispatchEvent( int nDeltaX, int nDeltaY, uint32 nButtons, int nVScroll, int nHScroll )
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
        // (xxl) Middle mouse button
	if ( nButtonFlg & 0x04 ) {
	    if ( nButtons & 0x04 ) {
		pcEvent = new Message( M_MOUSE_DOWN );
	    } else {
		pcEvent = new Message( M_MOUSE_UP );
	    }
	    pcEvent->AddInt32( "_button", 3 );
	    pcEvent->AddInt32( "_buttons", 3 ); // To be removed
	    EnqueueEvent( pcEvent );
	}
    }
    if ( nDeltaX != 0 || nDeltaY != 0 ) {
	Message* pcEvent = new Message( M_MOUSE_MOVED );
	pcEvent->AddPoint( "delta_move", cDeltaMove );
	EnqueueEvent( pcEvent );
    }
    // (xxl) Vertical and/or horizontal scroll
    if( nVScroll !=0 || nHScroll != 0 ) {
       Point cScroll( nHScroll, nVScroll );
       // send a specific scroll message: M_MOUSE_SCROLL
       Message* pcEvent = new Message( M_WHEEL_MOVED );
       // the "delta_move" key contains the scroll amount
       // as a os::Point structure ("x" for horizontal and
       // "y" for vertical). Usually the scroll amount is
       // either -1, 0 or 1, but I noticed that sometimes
       // the mouse send other values as well (-2 and 2 are
       // the most common).
       pcEvent->AddPoint( "delta_move", cScroll );
       EnqueueEvent( pcEvent );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void USBMouseDriver::EventLoop()
{
    char anBuf[4];
    int  nIndex = 0;
    uint32 nButtons = 0;
  
    for (;;)
    {
    
	if ( read( g_nSerialDevice, &anBuf, 4 ) != 4 ) {
	    dbprintf( "Error: USBMouseDriver::EventLoop() failed to read from serial device\n" );
	    continue;
	}
	    int x = (int)((float)anBuf[1] * 1.5);
	    int y = (int)((float)anBuf[2] * 1.5);
	    int nVScroll = 0;
	    
	    nButtons = 0;
	    if ( anBuf[0] & 0x01 ) {
		nButtons |= 0x01;
	    }
	    if ( anBuf[0] & 0x02 ) {
		nButtons |= 0x02;
	    }
	      // (xxl) check the middle button
	    if ( anBuf[0] & 0x04 ) {
	        nButtons |= 0x04;
	    }
	   
		nVScroll = anBuf[3];
	    DispatchEvent( x, y, nButtons, -nVScroll, 0 );
	    nIndex = 0;
	    //snooze( 1000 );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 USBMouseDriver::EventLoopEntry( void* pData )
{
    USBMouseDriver* pcThis = (USBMouseDriver*) pData;
  
    pcThis->EventLoop();
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool USBMouseDriver::Start()
{
    thread_id hEventThread;
    hEventThread = spawn_thread( "usbmouse_event_thread", (void*)EventLoopEntry, 120, 0, this );
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
    g_nSerialDevice = open( "/dev/input/usb_mouse", O_RDWR );
    if ( g_nSerialDevice < 0 ) {
	dbprintf( "No USB mouse device node found\n" );
	return( false );
    }
    dbprintf( "Found USB mouse device node\n" );
   
    
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
	dbprintf( "USB mouse driver: get_input_node() called with invalid index %d\n", nIndex );
	return( NULL );
    }
    return( new USBMouseDriver() );
}
