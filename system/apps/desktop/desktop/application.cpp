/*  Syllable Desktop
 *  Copyright (C) 2003 Arno Klenke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
 

#include "desktop.h"
#include "application.h"
#include "messages.h"
#include <iostream>

App::App() : os::Application( "application/syllable-Desktop" )
{
	m_pcDesktop = new Desktop();
	m_pcDesktop->Show();
	m_pcDesktop->MakeFocus();
	
	/* Make our port public */
	SetPublic( true );
}

void App::HandleMessage( os::Message* pcMessage )
{
	//dbprintf( "Got message!\n" );
	switch( pcMessage->GetCode() )
	{
		case M_GET_SETTINGS:
		{
			/* Called by another application to get the desktop settings */
			if( !pcMessage->IsSourceWaiting() )
				break;
			os::Message cMsg( M_GET_SETTINGS );
			cMsg.AddString( "background_image", m_pcDesktop->GetBackground() );
			cMsg.AddBool( "single_click", m_pcDesktop->GetSingleClick() );
			pcMessage->SendReply( &cMsg );
			break;
		}
		case M_SET_SETTINGS:
		{
			/* Called by another application to set the desktop settings */
			os::Message cMsg( M_SET_SETTINGS );
			os::String zImage;
			if( pcMessage->FindString( "background_image", &zImage.str() ) == 0 )
			{
				m_pcDesktop->SetBackground( zImage );
			}
			bool bSingleClick;
			if( pcMessage->FindBool( "single_click", &bSingleClick ) == 0 )
			{
				m_pcDesktop->SetSingleClick( bSingleClick );
			}
			break;
		}
		case M_GET_SINGLE_CLICK:
		{
			os::Message cReply( 0 );
			if( pcMessage->IsSourceWaiting() )
			{
				cReply.AddBool( "single_click", m_pcDesktop->GetSingleClick() );
				pcMessage->SendReply( &cReply );
			}
			break;
		}
		case M_SET_SINGLE_CLICK:
		{
			bool bEnabled;
			if( pcMessage->FindBool( "single_click", &bEnabled ) == 0 ) {
				m_pcDesktop->SetSingleClick( bEnabled );
			}
			break;
		}
		case M_GET_BACKGROUND:
		{
			os::Message cReply( 0 );
			if( pcMessage->IsSourceWaiting() )
			{
				cReply.AddString( "background_image", m_pcDesktop->GetBackground() );
				pcMessage->SendReply( &cReply );
			}
			break;
		}
		case M_SET_BACKGROUND:
		{
			os::String zBackground;
			if( pcMessage->FindString( "background_image", &zBackground ) == 0 ) {
				m_pcDesktop->SetBackground( zBackground );
			}
			break;
		}
		default:
			os::Application::HandleMessage( pcMessage );
	}
}












