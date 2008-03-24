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
#include <gui/checkbox.h>
#include <gui/button.h>
#include <util/string.h>
#include <util/message.h>
#include <storage/path.h>
#include "messages.h"
#include "project.h"
#include "ProjectPrefs.h"
#include "resources/sIDE.h"

ProjectPrefs::ProjectPrefs( const os::Rect& cFrame, project* pcProject, os::Window* pcProjectWindow )
				:os::Window( cFrame, "side_project_prefs", MSG_PPREF_TITLE, os::WND_NOT_V_RESIZABLE )
{
	/* Create main view */
	m_pcView = new os::LayoutView( GetBounds(), "side_p_view" );
	
	os::VLayoutNode* pcVNode = new os::VLayoutNode( "side_p_v" );
	pcVNode->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	/* Add target */
	os::HLayoutNode* pcHNode = new os::HLayoutNode( "side_p_h" );
	m_pcTargetLabel = new os::StringView( os::Rect(), "side_target_label", MSG_PPREF_TARGET );
	pcHNode->AddChild( m_pcTargetLabel );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcTarget = new os::TextView( os::Rect(), "side_target", "", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcTarget->SetReadOnly( false );
	m_pcTarget->SetMultiLine( false );
	m_pcTarget->Set( pcProject->GetTarget().c_str() );
	m_pcTarget->SetMinPreferredSize( 0, 1 );
	pcHNode->AddChild( m_pcTarget );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVNode->AddChild( pcHNode );
	
	/* Add category */
	pcHNode = new os::HLayoutNode( "side_p_h" );
	m_pcCategoryLabel = new os::StringView( os::Rect(), "side_category_label", MSG_PPREF_CATEGORY );
	pcHNode->AddChild( m_pcCategoryLabel );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcCategory = new os::TextView( os::Rect(), "side_category", "", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcCategory->SetReadOnly( false );
	m_pcCategory->SetMultiLine( false );
	m_pcCategory->Set( pcProject->GetCategory().c_str() );
	m_pcCategory->SetMinPreferredSize( 0, 1 );
	pcHNode->AddChild( m_pcCategory );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVNode->AddChild( pcHNode );
	
	/* Add install path */
	pcHNode = new os::HLayoutNode( "side_p_h" );
	m_pcInstallPathLabel = new os::StringView( os::Rect(), "side_install_path_label", MSG_PPREF_INSTALL_PATH );
	pcHNode->AddChild( m_pcInstallPathLabel );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcInstallPath = new os::TextView( os::Rect(), "side_install_path", "", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcInstallPath->SetReadOnly( false );
	m_pcInstallPath->SetMultiLine( false );
	m_pcInstallPath->Set( pcProject->GetInstallPath().c_str() );
	m_pcInstallPath->SetMinPreferredSize( 0, 1 );
	pcHNode->AddChild( m_pcInstallPath );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVNode->AddChild( pcHNode );
	
	/* Add compiler flags */
	pcHNode = new os::HLayoutNode( "side_p_h" );
	m_pcCFlagsLabel = new os::StringView( os::Rect(), "side_cflags_label", MSG_PPREF_CFLAGS );
	pcHNode->AddChild( m_pcCFlagsLabel );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcCFlags = new os::TextView( os::Rect(), "side_cflags", "", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcCFlags->SetReadOnly( false );
	m_pcCFlags->SetMultiLine( false );
	m_pcCFlags->Set( pcProject->GetCFlags().c_str() );
	m_pcCFlags->SetMinPreferredSize( 0, 1 );
	pcHNode->AddChild( m_pcCFlags );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVNode->AddChild( pcHNode );
	
	/* Add linker flags */
	pcHNode = new os::HLayoutNode( "side_p_h" );
	m_pcLFlagsLabel = new os::StringView( os::Rect(), "side_lflags_label", MSG_PPREF_LFLAGS );
	pcHNode->AddChild( m_pcLFlagsLabel );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcLFlags = new os::TextView( os::Rect(), "side_lflags", "", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcLFlags->SetReadOnly( false );
	m_pcLFlags->SetMultiLine( false );
	m_pcLFlags->Set( pcProject->GetLFlags().c_str() );
	m_pcLFlags->SetMinPreferredSize( 0, 1 );
	pcHNode->AddChild( m_pcLFlags );
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVNode->AddChild( pcHNode );
	
	/* Add terminal flag */
	m_pcTerminalFlag = new os::CheckBox( os::Rect(), "side_terminal_flag", MSG_PPREF_TERMINAL, NULL );
	m_pcTerminalFlag->SetValue( pcProject->GetTerminalFlag() );
	pcVNode->AddChild( new os::VLayoutSpacer( "", 3.0f, 3.0f ) );
	pcVNode->AddChild( m_pcTerminalFlag );
	
	pcVNode->AddChild( new os::VLayoutSpacer( "" ) );
	
	pcVNode->SameWidth( "side_target_label", "side_category_label", "side_install_path_label", "side_cflags_label", "side_lflags_label", NULL );
	
	pcHNode = new os::HLayoutNode( "side_p_h" );
	/* Add Ok button */
	
	m_pcOk = new os::Button( os::Rect(), "side_ok", MSG_BUTTON_OK, new os::Message( M_BUTTON_OK ) );
	pcHNode->AddChild( m_pcOk );
	
	pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	/* Add Cancel button */
	
	m_pcCancel = new os::Button( os::Rect(), "side_cancel", MSG_BUTTON_CANCEL, new os::Message( M_BUTTON_CANCEL ) );
	pcHNode->AddChild( m_pcCancel );
	pcHNode->SameWidth( "side_ok", "side_cancel", NULL );
	
	pcVNode->AddChild( pcHNode );
	
	m_pcView->SetRoot( pcVNode );
	
	AddChild( m_pcView );
	
	m_pcProject = pcProject;
	m_pcProjectWindow = pcProjectWindow;
	
	
	
}

ProjectPrefs::~ProjectPrefs()
{
}

bool ProjectPrefs::OkToQuit()
{
	m_pcProjectWindow->PostMessage( new os::Message( M_PROJECT_PREFS_CLOSED )
											, m_pcProjectWindow );
	return( true );
}

void ProjectPrefs::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_BUTTON_OK:
			m_pcProject->SetTarget( m_pcTarget->GetBuffer()[0] );
			m_pcProject->SetCategory( m_pcCategory->GetBuffer()[0] );
			m_pcProject->SetCFlags( m_pcCFlags->GetBuffer()[0] );
			m_pcProject->SetLFlags( m_pcLFlags->GetBuffer()[0] );
			m_pcProject->SetTerminalFlag( m_pcTerminalFlag->GetValue().AsBool() );
			m_pcProject->SetInstallPath( m_pcInstallPath->GetBuffer()[0] );
			m_pcProjectWindow->PostMessage( new os::Message( M_PROJECT_PREFS_CLOSED )
													, m_pcProjectWindow );
			Close();
		break;
		case M_BUTTON_CANCEL:
			m_pcProjectWindow->PostMessage( new os::Message( M_PROJECT_PREFS_CLOSED )
													, m_pcProjectWindow );
			Close();
		break;
		default:
			os::Window::HandleMessage( pcMessage );
	}
}













