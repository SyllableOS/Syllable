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
#include <gui/textview.h>
#include <gui/listview.h>
#include <gui/button.h>
#include <util/string.h>
#include <util/message.h>
#include <storage/path.h>
#include "resources/sIDE.h"
#include "messages.h"
#include "project.h"
#include "GroupWindow.h"

GroupWindow::GroupWindow( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow )
				:os::Window( cFrame, "side_groups", MSG_GROUPS_TITLE )
{
	
	/* Create main view */
	m_pcView = new os::LayoutView( GetBounds(), "side_a_view" );
	
	/* Include GUI */
	#include "GroupWindowLayout.cpp"
	
	m_pcList->InsertColumn( MSG_GROUPS_LIST.c_str(), (int)m_pcList->GetFrame().Width() );
	m_pcList->SetTarget( this );
	m_pcList->SetSelChangeMsg( new os::Message( M_SELECT ) );
	
	m_pcProject = pcProject;
	m_pcProjectWindow = pcProjectWindow;
	
	RefreshList();
	
	m_pcView->SetRoot( m_pcRoot );
	AddChild( m_pcView );
	
	SetDefaultButton( m_pcOk );
}

GroupWindow::~GroupWindow()
{
	os::ListViewRow *pcOldRow;
	for( uint r = 0; r < m_pcList->GetRowCount(); r++ )
	{
		pcOldRow = m_pcList->RemoveRow( 0 );
		delete( pcOldRow );
	}
}

bool GroupWindow::OkToQuit()
{
	m_pcProjectWindow->PostMessage( new os::Message( M_GROUP_CLOSED )
											, m_pcProjectWindow );
	return( true );
}

void GroupWindow::RefreshList()
{
	os::ListViewRow *pcOldRow;
	int nOld = m_pcList->GetFirstSelected();
	for( uint r = 0; r < m_pcList->GetRowCount(); r++ )
	{
		pcOldRow = m_pcList->RemoveRow( 0 );
		delete( pcOldRow );
	}
	m_pcList->Clear();
	os::ListViewStringRow *pcNewRow;
	for( uint i = 0; i < m_pcProject->GetGroupCount(); i++ )
	{
		pcNewRow = new os::ListViewStringRow();
		pcNewRow->AppendString( m_pcProject->GetGroupName( i ) );
		m_pcList->InsertRow( pcNewRow );
	}
	
	if( nOld >= 0 && nOld < (int)m_pcProject->GetGroupCount() )
		m_pcList->Select( nOld );
}

void GroupWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_BUTTON_OK:
			m_pcProjectWindow->PostMessage( new os::Message( M_GROUP_CLOSED )
													, m_pcProjectWindow );
			Close();
		break;
		case M_BUTTON_ADD:
			m_pcProject->AddGroup( m_pcInput->GetBuffer()[0] );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );
		break;
		case M_BUTTON_REMOVE:
			m_pcProject->RemoveGroup( m_pcList->GetFirstSelected() );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );
		break;
		case M_BUTTON_UP:
			if( m_pcList->GetFirstSelected() < 1 )
				break;
			m_pcProject->MoveGroup( m_pcList->GetFirstSelected(), m_pcList->GetFirstSelected() - 1 );
			m_pcList->Select( m_pcList->GetFirstSelected() - 1 );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );
		break;
		case M_BUTTON_DOWN:
			if( m_pcList->GetFirstSelected() < 0 )
				break;
			if( m_pcList->GetFirstSelected() == (int)m_pcProject->GetGroupCount() - 1 )
				break;
			m_pcProject->MoveGroup( m_pcList->GetFirstSelected(), m_pcList->GetFirstSelected() + 1 );
			m_pcList->Select( m_pcList->GetFirstSelected() + 1 );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );
		break;
		case M_LIST_REFRESH:
			RefreshList();
		break;
		case M_SELECT:
		{
			if( m_pcList->GetFirstSelected() < 0 )
				break;
			os::ListViewStringRow *pcRow = static_cast<os::ListViewStringRow*>( m_pcList->GetRow( m_pcList->GetFirstSelected() ) );
			m_pcInput->Set( pcRow->GetString( 0 ).c_str() );
		}
		break;
		default:
			os::Window::HandleMessage( pcMessage );
	}
}























