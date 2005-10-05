/*
 *  The Syllable application server
 *  Standard keyboard appserver driver
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2005 Arno Klenke
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

class KeyboardDriver : public InputNode
{
public:
    KeyboardDriver();
    ~KeyboardDriver();

    virtual bool Start();
    virtual int  GetType() { return( IN_KEYBOARD ); }
  
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

KeyboardDriver::KeyboardDriver()
{
	m_nDevice = open( "/dev/keybd", O_RDONLY );
	
	/* Reset keyboard */
	ioctl( m_nDevice, IOCTL_KBD_LEDRST );
	switch ( g_psKeymap->m_nLockSetting )
	{
	case 0x00:		// None
		break;

	case 0x01:		// Caps
		{
			ioctl( m_nDevice, IOCTL_KBD_CAPLOC );
			break;
		}

	case 0x02:		// Scroll
		{
			ioctl( m_nDevice, IOCTL_KBD_SCRLOC );
			break;
		}

	case 0x04:		// Num
		{
			ioctl( m_nDevice, IOCTL_KBD_NUMLOC );
			break;
		}

	default:
		dbprintf( "Unknown lock key 0x%2X\n", ( int )g_psKeymap->m_nLockSetting );
		break;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

KeyboardDriver::~KeyboardDriver()
{
	close( m_nDevice );
}


void KeyboardDriver::SetLED( int nKeyCode )
{
	switch ( nKeyCode )
	{
	case 0x3b:
		{
			ioctl( m_nDevice, IOCTL_KBD_CAPLOC );
			break;
		}

	case 0x22:
		{
			ioctl( m_nDevice, IOCTL_KBD_NUMLOC );
			break;
		}

	case 0x0f:
		{
			ioctl( m_nDevice, IOCTL_KBD_SCRLOC );
			break;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardDriver::DispatchEvent( int nKeyCode )
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

void KeyboardDriver::EventLoop()
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

int32 KeyboardDriver::EventLoopEntry( void* pData )
{
    KeyboardDriver* pcThis = (KeyboardDriver*) pData;
  
    pcThis->EventLoop();
    return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool KeyboardDriver::Start()
{
    thread_id hEventThread;
    hEventThread = spawn_thread( "keyboard_event_thread", (void*)EventLoopEntry, 120, 0, this );
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
    return( new KeyboardDriver() );
}







