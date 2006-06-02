// sIDE		(C) 2001-2005 Arno Klenke
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <gui/window.h>
#include <gui/point.h>
#include <gui/menu.h>
#include <gui/dropdownmenu.h>
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <util/string.h>
#include <util/message.h>
#include <storage/path.h>
//#include <codeview/codeview.h>
#include "messages.h"
#include "resources/sIDE.h"
#include "project.h"
#include "AddWindow.h"

AddWindow::AddWindow( const os::Rect& cFrame, project* pcProject, std::vector<os::String> azPaths, os::Window* pcProjectWindow )
				:os::Window( cFrame, "side_add", MSG_ADDWND_TITLE, os::WND_NOT_RESIZABLE )
{
	m_azPaths = azPaths;
	
	/* Create main view */
	m_pcView = new os::LayoutView( GetBounds(), "side_a_view" );
	
	os::VLayoutNode* pcVRoot = new os::VLayoutNode( "side_a_v" );
	pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	
	os::HLayoutNode* pcHNode = new os::HLayoutNode( "side_a_h" );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* create input view */
	os::VLayoutNode* pcVNode = new os::VLayoutNode( "side_a_v" );
	m_pcInput = new os::TextView( os::Rect(), "side_input", MSG_ADDWND_NAME.c_str(), 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcInput->SetReadOnly( false );
	m_pcInput->SetMultiLine( false );
	if( !m_azPaths.empty() )
		m_pcInput->SetEnable( false );
	else
		m_pcInput->Set( MSG_ADDWND_FILE.c_str() );
	pcVNode->AddChild( m_pcInput ); 
	pcVNode->AddChild( new os::VLayoutSpacer( "" ) );
	pcHNode->AddChild( pcVNode );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* create buttons */
	pcVNode = new os::VLayoutNode( "side_a_v" );
	m_pcAdd = new os::Button( os::Rect( cFrame.Width() - 70, 5, cFrame.Width() - 10 , 25 ), "side_add", 
						MSG_BUTTON_OK, new os::Message( M_BUTTON_ADD ),  os::CF_FOLLOW_RIGHT );
	pcVNode->AddChild( m_pcAdd );
	
	m_pcCancel = new os::Button( os::Rect( cFrame.Width() - 70, 30, cFrame.Width() - 10, 50 ), "side_cancel", 
						MSG_BUTTON_CANCEL, new os::Message( M_BUTTON_CANCEL ), os::CF_FOLLOW_RIGHT );
	pcVNode->AddChild( m_pcCancel );
	pcVNode->AddChild( new os::VLayoutSpacer( "" ) );
	pcVNode->SameWidth( "side_add", "side_cancel", NULL );
	pcHNode->AddChild( pcVNode );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	pcVRoot->AddChild( pcHNode );
	pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
	
	/* create dropdown menus */
	pcHNode = new os::HLayoutNode( "side_a_h" );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	m_pcGroup = new os::DropdownMenu( os::Rect( 5, 60, 175, 75 ), "side_type" );
	pcHNode->AddChild( m_pcGroup );
	pcHNode->AddChild( new os::HLayoutSpacer( "" ) );
	
	m_pcType = new os::DropdownMenu( os::Rect( 180, 60, 245, 75 ), "side_type" );
	m_pcType->AppendItem( MSG_TYPE_HEADER );
	m_pcType->AppendItem( MSG_TYPE_C );
	m_pcType->AppendItem( MSG_TYPE_CPP );
	m_pcType->AppendItem( MSG_TYPE_LIB );
	m_pcType->AppendItem( MSG_TYPE_OTHER );
	m_pcType->AppendItem( MSG_TYPE_RES );
	m_pcType->AppendItem( MSG_TYPE_CATALOG );
	m_pcType->AppendItem( MSG_TYPE_IF );
	m_pcType->SetSelection( 0 );
	pcHNode->AddChild( m_pcType );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	
	pcVRoot->AddChild( pcHNode );
	pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	m_pcView->SetRoot( pcVRoot );
	
	AddChild( m_pcView );
	
	m_pcProject = pcProject;
	m_pcProjectWindow = pcProjectWindow;

	RefreshList();
	
}

AddWindow::~AddWindow()
{
}

bool AddWindow::OkToQuit()
{
	m_pcProjectWindow->PostMessage( new os::Message( M_ADD_CLOSED )
											, m_pcProjectWindow );
	return( true );
}

void AddWindow::RefreshList()
{
	m_pcGroup->Clear();
	for( uint i = 0; i < m_pcProject->GetGroupCount(); i++ )
	{
		m_pcGroup->AppendItem( m_pcProject->GetGroupName( i ).c_str() );
	} 
	
	m_pcGroup->SetSelection( 0 );
}


void AddWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		
		case M_BUTTON_ADD:
			if( m_pcGroup->GetSelection() > -1 && m_pcType->GetSelection() > -1 )
				if( m_azPaths.empty() ) {	
					m_pcProject->AddFile( m_pcGroup->GetSelection(), m_pcInput->GetBuffer()[0], 
											m_pcType->GetSelection() + 1 );
				} else {
					for( uint i = 0; i < m_azPaths.size(); i++ )
						m_pcProject->AddFile( m_pcGroup->GetSelection(), m_azPaths[i], 
											m_pcType->GetSelection() + 1 );

				}
			m_pcProjectWindow->PostMessage( new os::Message( M_ADD_CLOSED )
													, m_pcProjectWindow );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );
			
			Close();
		break;
		case M_BUTTON_CANCEL:
			m_pcProjectWindow->PostMessage( new os::Message( M_ADD_CLOSED )
													, m_pcProjectWindow );
			Close();
		break;
		case M_LIST_REFRESH:
			RefreshList();
		break;
		default:
			os::Window::HandleMessage( pcMessage );
	}
}























