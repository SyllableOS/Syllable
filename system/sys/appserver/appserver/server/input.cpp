
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
#include <unistd.h>
#include <dirent.h>

#include <atheos/time.h>
#include <atheos/kernel.h>
#include <atheos/image.h>

#include "inputnode.h"
#include "server.h"
#include "layer.h"
#include "swindow.h"
#include "keyboard.h"
#include "bitmap.h"
#include "config.h"

#include <util/message.h>

using namespace os;

MessageQueue InputNode::s_cEventQueue;
Point InputNode::s_cMousePos( 0, 0 );
uint32 InputNode::s_nMouseButtons = 0;
atomic_t InputNode::s_nMouseMoveEventCount = ATOMIC_INIT(0);

static thread_id g_hEventThread = -1;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void InputNode :: _CalculateMouseMovement( Point& cDeltaMove )
{
	cDeltaMove.x = cDeltaMove.x * AppserverConfig::GetInstance()->GetMouseSpeed();
	cDeltaMove.y = cDeltaMove.y * AppserverConfig::GetInstance()->GetMouseSpeed();

	double acceleration = 1;
	acceleration = 1 + sqrt( cDeltaMove.x * cDeltaMove.x + cDeltaMove.y * cDeltaMove.y ) * AppserverConfig::GetInstance()->GetMouseAcceleration();
	
	if( cDeltaMove.x > 0 )
		cDeltaMove.x = floor( cDeltaMove.x * acceleration );
	else
		cDeltaMove.x = ceil( cDeltaMove.x * acceleration );
	if( cDeltaMove.y > 0 )
		cDeltaMove.y = floor( cDeltaMove.y * acceleration );
	else
		cDeltaMove.y = ceil( cDeltaMove.y * acceleration );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

InputNode::InputNode()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

InputNode::~InputNode()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void InputNode::SetMousePos( const Point & cPos )
{
//    if ( cPos != s_cMousePos ) {
	Message *pcEvent = new Message( M_MOUSE_MOVED );

	pcEvent->AddPoint( "delta_move", cPos - s_cMousePos );
	EnqueueEvent( pcEvent );
//    }
}

Point InputNode::GetMousePos()
{
	return ( s_cMousePos );
}

uint32 InputNode::GetMouseButtons()
{
	return ( s_nMouseButtons );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void InputNode::EnqueueEvent( Message * pcEvent )
{
	int32 nButton = 0;

	s_cEventQueue.Lock();

	pcEvent->AddInt32( "_qualifiers", GetQualifiers() );
	pcEvent->AddInt64( "_timestamp", get_system_time() );

	switch ( pcEvent->GetCode() )
	{
	case M_MOUSE_DOWN:
		if( pcEvent->FindInt32( "_button", &nButton ) == 0 )
		{
			if( AppserverConfig::GetInstance()->GetMouseSwapButtons() && (nButton == 1 || nButton == 2) )
			{
				/* Swap buttons 1 & 2 */
				nButton = 3 - nButton;
				pcEvent->RemoveName( "_button" );
				pcEvent->AddInt32( "_button", nButton );
			}
			s_nMouseButtons |= 1 << ( nButton - 1 );
		}
		break;
	case M_MOUSE_UP:
		if( pcEvent->FindInt32( "_button", &nButton ) == 0 )
		{
			if( AppserverConfig::GetInstance()->GetMouseSwapButtons() && (nButton == 1 || nButton == 2) )
			{
				/* Swap buttons 1 & 2 */
				nButton = 3 - nButton;
				pcEvent->RemoveName( "_button" );
				pcEvent->AddInt32( "_button", nButton );
			}
			s_nMouseButtons &= ~( 1 << ( nButton - 1 ) );
		}
		break;
	case M_MOUSE_MOVED:
		{
			Point cDeltaMove;

			if( pcEvent->FindPoint( "delta_move", &cDeltaMove ) == 0 )
			{
				_CalculateMouseMovement( cDeltaMove );
				s_cMousePos += cDeltaMove;
			}
			else
			{
				dbprintf( "InputNode::EnqueueEvent() M_MOUSE_MOVED event missing delta_move entry\n" );
			}
			pcEvent->AddInt32( "_button", s_nMouseButtons );	// To be removed
			pcEvent->AddInt32( "_buttons", s_nMouseButtons );
			atomic_inc( &s_nMouseMoveEventCount );
			break;
		}
    }
	if( s_cMousePos.x < 0 )
		s_cMousePos.x = 0;
	if( s_cMousePos.x >= g_pcScreenBitmap->m_nWidth )
		s_cMousePos.x = g_pcScreenBitmap->m_nWidth - 1;
	if( s_cMousePos.y < 0 )
		s_cMousePos.y = 0;
	if( s_cMousePos.y >= g_pcScreenBitmap->m_nHeight )
		s_cMousePos.y = g_pcScreenBitmap->m_nHeight - 1;


	pcEvent->AddPoint( "_scr_pos", s_cMousePos );

	s_cEventQueue.AddMessage( pcEvent );
	s_cEventQueue.Unlock();


//  resume_thread( g_hEventThread );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void InputNode::EventLoop()
{
	for( ;; )
	{
		Message *pcEvent;
		int nKeyCode = 0;

		for( ;; )
		{
			s_cEventQueue.Lock();
			pcEvent = s_cEventQueue.NextMessage();
			s_cEventQueue.Unlock();
			if( pcEvent != NULL && pcEvent->GetCode() == M_MOUSE_MOVED )
			{
				if( !atomic_dec_and_test( &s_nMouseMoveEventCount ) )
				{
					delete pcEvent;

					continue;
				}
				g_cLayerGate.Close();
				g_pcDispDrv->SetMousePos( static_cast < IPoint > ( s_cMousePos ) );
				g_cLayerGate.Open();
			} 
			break;
		}
		
		if( pcEvent != NULL )
		{
			if( pcEvent->GetCode() == M_KEY_DOWN || pcEvent->GetCode() == M_KEY_UP )
			{
				pcEvent->FindInt32( "_key", &nKeyCode );
				HandleKeyboard( true, nKeyCode );
			} else {
				AppServer::GetInstance()->ResetEventTime(  );
				SrvWindow::HandleInputEvent( pcEvent );
			}
			delete pcEvent;
		}
		else
		{
			HandleKeyboard( false, nKeyCode );
			snooze( 10000 );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 InputNode::EventLoopEntry( void *pData )
{
	EventLoop();
	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void InitInputSystem()
{
	char zInputDrvPath[1024] = "/system/drivers/appserver/input/";
	int nPathLen = strlen( zInputDrvPath );
	DIR *pDir = opendir( zInputDrvPath );
	bool bMouseFound = false;

	if( pDir != NULL )
	{
		struct dirent *psEntry;

		while( ( psEntry = readdir( pDir ) ) != NULL )
		{
			if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			int nError;

			dbprintf( "Probe input node %s\n", psEntry->d_name );
			zInputDrvPath[nPathLen] = '\0';
			strcat( zInputDrvPath, psEntry->d_name );
			int nLib = load_library( zInputDrvPath, 0 );

			if( nLib < 0 )
			{
				dbprintf( "failed to load input node %s\n", zInputDrvPath );
				continue;
			}
			in_init *pInitFunc;
			in_get_node_count *pGetNodeCount;
			in_get_input_node *pGetInputNode;

			nError = get_symbol_address( nLib, "init_input_node", -1, ( void ** )&pInitFunc );
			if( nError < 0 )
			{
				dbprintf( "Input node %s does not export entry point init_input_node()\n", psEntry->d_name );
				unload_library( nLib );
				continue;
			}
			nError = get_symbol_address( nLib, "get_node_count", -1, ( void ** )&pGetNodeCount );
			if( nError < 0 )
			{
				dbprintf( "Input node %s does not export entry point get_node_count()\n", psEntry->d_name );
				unload_library( nLib );
				continue;
			}
			nError = get_symbol_address( nLib, "get_input_node", -1, ( void ** )&pGetInputNode );
			if( nError < 0 )
			{
				dbprintf( "Input node %s does not export entry point get_input_node()\n", psEntry->d_name );
				unload_library( nLib );
				continue;
			}

			if( pInitFunc() == false )
			{
				dbprintf( "Input node %s failed to initialize\n", psEntry->d_name );
				unload_library( nLib );
				continue;
			}
			int nCount = pGetNodeCount();

			for( int i = 0; i < nCount; ++i )
			{
				InputNode *pcNode = pGetInputNode( i );

				if( pcNode != NULL )
				{
					if( pcNode->Start() )
					{
						if( pcNode->GetType() == InputNode::IN_MOUSE )
						{
							bMouseFound = true;
						}
					}
				}
			}
		}
	}
	else
	{
		dbprintf( "Error: failed to open input driver directory\n" );
	}

	if( bMouseFound == false )
	{
		dbprintf( "No mouse input node found!\n" );
	}

	g_hEventThread = spawn_thread( "input_thread", (void*)InputNode::EventLoopEntry, DISPLAY_PRIORITY, 0, NULL );
	resume_thread( g_hEventThread );
}
