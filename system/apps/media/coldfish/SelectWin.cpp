/*  ColdFish Music Player
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include "messages.h"
#include "SelectWin.h"

enum {
	CF_GUI_PLAYLIST_SELECTOR = 100,
	CF_GUI_PLAYLIST_OPEN
};

SelectWin::SelectWin( os::Rect cFrame )
		: os::Window( cFrame, "cf_select_win", "Select Playlist - ColdFish", 
						os::WND_NOT_V_RESIZABLE )
{
	/* Create file input field */
	m_pcFileLabel = new os::StringView( os::Rect( 0, 0, 1, 1 ), "file_label", "File" );
	m_pcFileLabel->SetFrame( os::Rect( 5, 10, m_pcFileLabel->GetPreferredSize( false ).x + 5, 
									m_pcFileLabel->GetPreferredSize( false ).y + 10 ) );
	
	m_pcFileInput = new os::TextView( os::Rect( 0, 0, 1, 1 ), "file_input", "", os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP |
																			os::CF_FOLLOW_RIGHT );
	m_pcFileInput->SetFrame( os::Rect( m_pcFileLabel->GetPreferredSize( false ).x + 10, 5, 180,
								 m_pcFileInput->GetPreferredSize( false ).y + 5 ) );
	m_pcFileInput->SetMultiLine( false );
	
	m_pcFileButton = new os::Button( os::Rect( 182, 5, 200, m_pcFileInput->GetPreferredSize( false ).y + 5 ),
							"file_button", "..", new os::Message( CF_GUI_PLAYLIST_SELECTOR ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP );
	
	/* Create open button */
	m_pcOpenButton = new os::Button( os::Rect( 0, 0, 1, 1 ),
							"open_button", "Open", new os::Message( CF_GUI_PLAYLIST_OPEN ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	m_pcOpenButton->SetFrame( os::Rect( 160, 30, 200, m_pcOpenButton->GetPreferredSize( false ).y + 30 ) );

	/* Create information */
	m_pcInfo = new os::StringView( os::Rect( 0, 0, 1, 1 ), "info", "Note: If you try to open a non existent",
												os::ALIGN_CENTER, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP );
	m_pcInfo->SetFrame( os::Rect( 0, m_pcOpenButton->GetPreferredSize( false ).y + 40, 
												GetBounds().Width(), m_pcOpenButton->GetPreferredSize( false ).y + 
												m_pcInfo->GetPreferredSize( false ).y + 40 ) );
	
	m_pcInfo2 = new os::StringView( os::Rect( 0, 0, 1, 1 ), "info", "playlist then it will be created",
												os::ALIGN_CENTER, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP );
	m_pcInfo2->SetFrame( m_pcInfo->GetFrame() + os::Point( 0, m_pcInfo->GetPreferredSize( false ).y ) );
	
	/* Create file selector */
	m_pcFileDialog = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( this ), NULL, os::FileRequester::NODE_FILE, false );
	m_pcFileDialog->Lock();
	m_pcFileDialog->Start();
	m_pcFileDialog->Unlock();
	
	AddChild( m_pcFileLabel );
	AddChild( m_pcFileInput );
	AddChild( m_pcFileButton );
	AddChild( m_pcOpenButton );
	AddChild( m_pcInfo );
	AddChild( m_pcInfo2 );
	
	SetFocusChild( m_pcFileInput );
	
}

SelectWin::~SelectWin()
{
	m_pcFileDialog->Close();
}

void SelectWin::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case CF_GUI_PLAYLIST_OPEN:
		{
			os::Message* pcMsg = new os::Message( CF_GUI_LIST_SELECTED );
			pcMsg->AddString( "file/path", m_pcFileInput->GetBuffer()[0] );
			os::Application::GetInstance()->PostMessage( pcMsg, os::Application::GetInstance() );
			PostMessage( os::M_QUIT );
		}
		break;
		case CF_GUI_PLAYLIST_SELECTOR:
			m_pcFileDialog->Show();
			m_pcFileDialog->MakeFocus( true );
		break;
		case os::M_LOAD_REQUESTED:
		{
			os::String zFile;
			if( pcMessage->FindString( "file/path", &zFile.str() ) == 0 ) {
				m_pcFileInput->Clear();
				m_pcFileInput->Insert( zFile.c_str() );
			}
		}
		default:
			os::Window::HandleMessage( pcMessage );
	}
}

