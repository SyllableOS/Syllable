/*
 *  The Syllable application server
 *  USB keyboard appserver driver
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2005 - 2006 Arno Klenke
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
#include <string>

#include <inputnode.h>
#include <keyboard.h>

#include <appserver/protocol.h>
#include <util/message.h>
#include <atheos/kernel.h>
#include <sys/ioctl.h>

using namespace os;

class USBKeyboardDriver : public InputNode
{
public:
    USBKeyboardDriver();
    ~USBKeyboardDriver();

    virtual bool Start();
    virtual int  GetType() { return( IN_KEYBOARD ); }
	bool m_bIsInitialized;
private:
	void SetLED( int nKeyCode );
    static int32 EventLoopEntry( void* pData );
    void EventLoop();
    void DispatchEvent( int nKeyCode );

    thread_id m_hThread;
    int	    m_nDevice;
};



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

USBKeyboardDriver::USBKeyboardDriver()
{
	m_bIsInitialized = false;
	m_nDevice = open( "/dev/input/usb_keyboard", O_RDONLY );
	if( m_nDevice >= 0 )
		m_bIsInitialized = true;

	/* TODO: LEDS */	
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

USBKeyboardDriver::~USBKeyboardDriver()
{
	if( m_nDevice >= 0 )
		close( m_nDevice );
}


void USBKeyboardDriver::SetLED( int nKeyCode )
{
	/* TODO: */
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void USBKeyboardDriver::DispatchEvent( int nKeyCode )
{
	SetLED( nKeyCode );
	Message* pcEvent = new Message( nKeyCode & 0x80 ? M_KEY_UP : M_KEY_DOWN );
	pcEvent->AddInt32( "_key", nKeyCode );
	EnqueueEvent( pcEvent );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void USBKeyboardDriver::EventLoop()
{
    uint8 nKeyCode;
   
    for (;;)
    {
    	snooze( 10000 );
		if ( read( m_nDevice, &nKeyCode, 1 ) == 1 ) {
		    DispatchEvent( nKeyCode );
    	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 USBKeyboardDriver::EventLoopEntry( void* pData )
{
    USBKeyboardDriver* pcThis = (USBKeyboardDriver*) pData;
  
    pcThis->EventLoop();
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool USBKeyboardDriver::Start()
{
    thread_id hEventThread;
    hEventThread = spawn_thread( "usbkeyboard_event_thread", (void*)EventLoopEntry, 120, 0, this );
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
	int nFd = open( "/dev/input/usb_keyboard", O_RDONLY );
	if( nFd >= 0 )
	{
		close( nFd );
		return( true );
	}
    return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern "C" int uninit_input_node()
{
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
	dbprintf( "Keyboard driver: get_input_node() called with invalid index %d\n", nIndex );
	return( NULL );
    }
    USBKeyboardDriver* pcDriver = new USBKeyboardDriver();
    if( pcDriver->m_bIsInitialized )
    	return( pcDriver );
    delete( pcDriver );
    return( NULL );
}











