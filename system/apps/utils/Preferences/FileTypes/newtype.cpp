#include "mainwindow.h"
#include "messages.h"
#include "newtype.h"
#include "resources/FileTypes.h"

/* New filetype window */

NewTypeWin::NewTypeWin( os::Looper* pcParent, os::Rect cFrame ) : os::Window( cFrame, "newtype_window", 
																		MSG_NEWTYPEWND_TITLE, os::WND_NOT_V_RESIZABLE )

{
	m_pcParent = pcParent;
			
	/* Create main view */
	m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
	os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
	pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
	
	os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node", 0.0f );
	
	/* create input view */
	m_pcInput = new os::TextView( os::Rect(), "input", "Name", 
							os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
	m_pcInput->SetReadOnly( false );
	m_pcInput->SetMultiLine( false );
	m_pcInput->Set( MSG_NEWTYPEWND_MIMETYPE.c_str() );
	pcVRoot->AddChild( m_pcInput ); 
	pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
		
	
	/* create buttons */
	m_pcOk = new os::Button( os::Rect(), "ok", 
					MSG_NEWTYPEWND_OK, new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
			
	
	pcHNode->AddChild( new os::HLayoutSpacer( "" ) );
	pcHNode->AddChild( m_pcOk, 0.0f );
	pcVRoot->AddChild( pcHNode );
		
	m_pcView->SetRoot( pcVRoot );
	
	AddChild( m_pcView );
		
	m_pcInput->SetTabOrder( 0 );
	m_pcOk->SetTabOrder( 1 );
	
	SetDefaultButton( m_pcOk );
		
	m_pcInput->MakeFocus();
}
	

NewTypeWin::~NewTypeWin() {}
	
	
void NewTypeWin::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case 0:
		{
			os::String zMimeType = m_pcInput->GetBuffer()[0];
			os::Message cReply(	M_ADD_TYPE_CALLBACK );
			cReply.AddString( "mimetype", zMimeType );
			
			m_pcParent->PostMessage( &cReply, m_pcParent );
			Close();
			break;
		}
		default:
			break;
	}
	os::Window::HandleMessage( pcMessage );
}



