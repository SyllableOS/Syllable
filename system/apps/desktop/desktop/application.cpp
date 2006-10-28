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
	SetCatalog("desktop.catalog");

	m_pcDesktop = new Desktop();
	m_pcDesktop->Show();
	m_pcDesktop->MakeFocus();
	
	/* Make our port public */
	SetPublic( true );
}

bool App::OkToQuit()
{
	return( false );
}

void App::HandleMessage( os::Message* pcMessage )
{
	//dbprintf( "Got message!\n" );
	switch( pcMessage->GetCode() )
	{
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
		case M_GET_DESKTOP_FONT_SHADOW:
		{
			os::Message cReply( 0 );
			if( pcMessage->IsSourceWaiting() )
			{
				cReply.AddBool( "desktop_font_shadow", m_pcDesktop->GetDesktopFontShadow() );
				pcMessage->SendReply( &cReply );
			}
			break;
		}
		case M_SET_DESKTOP_FONT_SHADOW:
		{
			bool bEnabled;
			if( pcMessage->FindBool( "desktop_font_shadow", &bEnabled ) == 0 ) {
				m_pcDesktop->SetDesktopFontShadow( bEnabled );
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

		case M_REFRESH:
		{
			m_pcDesktop->PostMessage(new os::Message(M_REFRESH),m_pcDesktop);
			break;
		}

		default:
			os::Application::HandleMessage( pcMessage );
	}
}













