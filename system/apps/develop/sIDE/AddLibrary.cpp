// sIDE		(C) 2001-2006 Arno Klenke
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
#include <gui/radiobutton.h>
#include <gui/separator.h>
#include <gui/listview.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <util/string.h>
#include <util/message.h>
#include <storage/path.h>
#include "messages.h"
#include "resources/sIDE.h"

#include "project.h"
#include "AddLibrary.h"

enum
{
	M_AL_SELECT = 1000,
	M_AL_CUSTOM,
	M_AL_OK
};

AddLibraryWindow::AddLibraryWindow( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow )
				:os::Window( cFrame, "side_add_library", "Add library", os::WND_NOT_RESIZABLE )
{
	
	/* Create main view */
	m_pcView = new os::LayoutView( GetBounds(), "side_a_view" );
	
	#include "AddLibraryLayout.cpp"
	
	m_pcRoot->SameWidth( "SelectRadio", "CustomRadio", "GroupString", NULL );
	
	m_pcProject = pcProject;
	m_pcProjectWindow = pcProjectWindow;
	
	RefreshList();
	
	m_pcView->SetRoot( m_pcRoot );
	m_pcCustomEdit->SetEnable( false );
	
	AddChild( m_pcView );
}

AddLibraryWindow::~AddLibraryWindow()
{
}

bool AddLibraryWindow::OkToQuit()
{
	m_pcProjectWindow->PostMessage( new os::Message( M_ADD_LIBRARY_CLOSED )
											, m_pcProjectWindow );
	return( true );
}


void AddLibraryWindow::RefreshList()
{
	m_pcGroupBox->Clear();
	for( uint i = 0; i < m_pcProject->GetGroupCount(); i++ )
	{
		m_pcGroupBox->AppendItem( m_pcProject->GetGroupName( i ).c_str() );
	} 
	
	m_pcGroupBox->SetSelection( 0 );
}


void AddLibraryWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_AL_SELECT:
			m_pcCustomEdit->SetEnable( false );
			m_pcSelectBox->SetEnable( true );
		break;
		case M_AL_CUSTOM:
			m_pcSelectBox->SetEnable( false );
			m_pcCustomEdit->SetEnable( true );
		break;
		case M_AL_OK:
		{
			os::String cLib;
			/* Get library name */
			if( m_pcCustomEdit->IsEnabled() )
				cLib = m_pcCustomEdit->GetBuffer()[0];
			else
				cLib = m_pcSelectBox->GetCurrentString();
				
			if( cLib == "" )
				break;
			if( cLib.substr( 0, 3 ) == "lib" )
				cLib = cLib.substr( 3, cLib.Length() - 3 );
				
			/* Add */
			if( m_pcGroupBox->GetSelection() > -1 )
			{
				m_pcProject->AddFile( m_pcGroupBox->GetSelection(), cLib, 
											TYPE_LIB );
			}
			m_pcProjectWindow->PostMessage( new os::Message( M_ADD_LIBRARY_CLOSED )
											, m_pcProjectWindow );
			m_pcProjectWindow->PostMessage( new os::Message( M_LIST_REFRESH )
												, m_pcProjectWindow );

			PostMessage( os::M_TERMINATE );
		}
		break;
		case M_LIST_REFRESH:
			RefreshList();
		break;
		default:
			os::Window::HandleMessage( pcMessage );
	}
}








