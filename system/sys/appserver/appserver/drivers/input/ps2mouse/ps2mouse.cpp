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
 *
 *  2001-08-11 - Intellimouse mouse-wheel support added by a patch
 *  from Catalin Climov <xxl@climov.com>
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

// (xxl) The PS/2 protocol (regular PS/2 mice)
#define PS2_PROTOCOL 1
// (xxl) The IMPS/2 protocol (IntelliMouse)
#define IMPS2_PROTOCOL 2
// (xxl) The number of "out of sync"'s needed in order to switch
// the mouse protocol. 2 seems a good default
#define SYNC_THRESHOLD 2

static int g_nSerialDevice = -1;

class PS2MouseDriver : public InputNode
{
public:
    PS2MouseDriver();
    ~PS2MouseDriver();

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
    // (xxl) protocol type (PS2_PROTOCOL or IMPS2_PROTOCOL)
    int     m_nProtocol;
    // (xxl) the number of bytes in the protocol:
    // 3 for PS/2 and 4 for IMPS/2
    int     m_nProtoByteCount;
    // (xxl) "out of sync" countdown
    int     m_nSyncCount;
};



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

PS2MouseDriver::PS2MouseDriver()
  : m_nProtocol( IMPS2_PROTOCOL ), m_nProtoByteCount( 4 ), m_nSyncCount( SYNC_THRESHOLD )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

PS2MouseDriver::~PS2MouseDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void PS2MouseDriver::DispatchEvent( int nDeltaX, int nDeltaY, uint32 nButtons, int nVScroll, int nHScroll )
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

void PS2MouseDriver::EventLoop()
{
    char anBuf[4];
    int  nIndex = 0;
  
    for (;;)
    {
	uint8 nByte;
    
	if ( read( g_nSerialDevice, &nByte, 1 ) != 1 ) {
	    dbprintf( "Error: PS2MouseDriver::EventLoop() failed to read from serial device\n" );
	    continue;
	}

	if ( nIndex == 0 ) {
	      // (xxl) The driver chokes on this test if you enable the
	      // middle mouse button.
	      //if ( (nByte & 0x08) == 0 || (nByte & 0x04) != 0 ) {
	    if ( (nByte & 0x08) == 0 ) {
		  // (xxl) try to switch the mouse protocol on every
		  // SYNC_THRESHOLD "out of sync"'s
		  // not very smart, but good enough
		m_nSyncCount--;
		if( m_nSyncCount == 0 ) {
		    int nProto = PS2_PROTOCOL + IMPS2_PROTOCOL - m_nProtocol;
		    dbprintf( "Mouse out of sync. Switching protocol to: %d\n", nProto );
		    SwitchToProtocol( nProto );
		}
		continue; // Out of sync.
	    }
	}
	anBuf[nIndex++] = nByte;
	  // (xxl) The IntelliMouse uses a 4-byte protocol, as opposite to the
	  // regular ps2 mouse, which uses 3-byte.
	  // the 4th byte contains scroll information
	  // please note that this driver is (yet) incompatible with the
	  // regular ps2 mice - I plan to add some sort of autodection ASAP
	  // and make it work with both ps2 and imps2 (IntelliMouse)
	if ( nIndex >= m_nProtoByteCount ) {
	    int x = anBuf[1];
	    int y = anBuf[2];
	    int nVScroll = 0;
	    
	    if ( anBuf[0] & 0x10 ) {
		x |= 0xffffff00;
	    }
	    if ( anBuf[0] & 0x20 ) {
		y |= 0xffffff00;
	    }
	    uint32 nButtons = 0;
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
	    if ( m_nProtoByteCount > 3 ) {
		  // (xxl) scroll wheel - vertical
		if ( anBuf[3] != 0 ) {
		      // if the most significant bit is set, we're most likely
		      // dealing with a negative value
//		    if( anBuf[3] & 0x80 )
			  // zzz - check if this is how to transform
			  // 0..255 to -127..127
//			nVScroll = anBuf[3] - 256;
//		    else
			nVScroll = anBuf[3];
		}
	    }
	    else
		nVScroll = 0;
	    DispatchEvent( x, -y, nButtons, nVScroll, 0 );
	    nIndex = 0;
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 PS2MouseDriver::EventLoopEntry( void* pData )
{
    PS2MouseDriver* pcThis = (PS2MouseDriver*) pData;
  
    pcThis->EventLoop();
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool PS2MouseDriver::Start()
{
    thread_id hEventThread;
    hEventThread = spawn_thread( "ps2mouse_event_thread", EventLoopEntry, 120, 0, this );
    resume_thread( hEventThread );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void PS2MouseDriver::SwitchToProtocol( int nProto )
{
    m_nSyncCount = SYNC_THRESHOLD;
    switch( nProto )
    {
	case PS2_PROTOCOL:
	    m_nProtocol = nProto;
	    m_nProtoByteCount = 3;
	    break;
	case IMPS2_PROTOCOL:
	    m_nProtocol = nProto;
	    m_nProtoByteCount = 4;
	    break;
	default:
	    dbprintf( "Unknown mouse protocol requested: %d\n", nProto );
	    break;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" bool init_input_node()
{
    // (xxl) imps2 initialization sequence
    static unsigned char s1[] = { 243, 200, 243, 100, 243, 80, };
    static unsigned char s2[] = { 246, 230, 244, 243, 100, 232, 3, };

    g_nSerialDevice = open( "/dev/misc/ps2aux", O_RDWR );
    if ( g_nSerialDevice < 0 ) {
	dbprintf( "No PS2 pointing device found\n" );
	return( false );
    }
    dbprintf( "Found PS2 pointing device. Init mouse...\n" );
   
    // (xxl) try to switch the mouse to the imps2 mode (enable the scroll wheel)
    write( g_nSerialDevice, s1, sizeof(s1) );
    snooze( 30000 );
    write( g_nSerialDevice, s2, sizeof(s2) );
    snooze( 30000 );
    tcflush( g_nSerialDevice, TCIFLUSH );
    
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
	dbprintf( "PS2 mouse driver: get_input_node() called with invalid index %d\n", nIndex );
	return( NULL );
    }
    return( new PS2MouseDriver() );
}

