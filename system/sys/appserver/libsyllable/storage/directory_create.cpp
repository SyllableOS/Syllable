/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <gui/requesters.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/string.h>
#include <util/application.h>
#include <storage/path.h>
#include <storage/symlink.h>
#include <storage/directory.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cassert>
#include <sys/stat.h>
#include <string>

#include "catalogs/libsyllable.h"

#define GS( x, def )		( m_pcCatalog ? m_pcCatalog->GetString( x, def ) : def )

using namespace os;


struct NewDirParams_s
{
	NewDirParams_s( Directory* pcDir, const Messenger & cViewTarget, Message* pcMsg, String cDefaultName )
	: m_pcDir( pcDir ), m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg ), m_cDefaultName( cDefaultName )
	{
	}
	Directory* m_pcDir;
	Messenger m_cViewTarget;
	Message* m_pcMsg;
	String m_cDefaultName;
	
};

class NewDirWin : public os::Window
{
public:
	NewDirWin( NewDirParams_s* psParams, os::Rect cFrame ) : os::Window( cFrame, "newdir_window", 
																			"New directory", os::WND_NOT_V_RESIZABLE )
	{
		m_psParams = psParams;

		m_pcCatalog = os::Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );

		SetTitle( GS( ID_MSG_NEWDIR_WINDOW_TITLE, "New directory" ) );

		/* Create main view */
		m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
		os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
		pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
	
		os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node" );
	
		/* create input view */
		m_pcInput = new os::TextView( os::Rect(), "input", "Name", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
		m_pcInput->SetReadOnly( false );
		m_pcInput->SetMultiLine( false );
		m_pcInput->Set( psParams->m_cDefaultName.c_str() );
		pcVRoot->AddChild( m_pcInput ); 
		pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
		
	
		/* create buttons */
		m_pcOk = new os::Button( os::Rect(), "ok", 
						GS( ID_MSG_NEWDIR_WINDOW_OK_BUTTON, "Ok" ), new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	
		
		pcHNode->AddChild( new os::HLayoutSpacer( "" ) );
		pcHNode->AddChild( m_pcOk )->LimitMaxSize( m_pcOk->GetPreferredSize( false ) );
		pcVRoot->AddChild( pcHNode );
		
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcInput->SetTabOrder( 0 );
		m_pcOk->SetTabOrder( 1 );
		
		SetDefaultButton( m_pcOk );
		
		m_pcInput->MakeFocus();
		
	}
	

	~NewDirWin() {
		delete( m_psParams->m_pcMsg );
		delete( m_psParams );
		if( m_pcCatalog )
			m_pcCatalog->Release();
	}
	
	void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			case 0:
			{
				bool bError = false;
				try {
					os::Directory cNewDir;
					if( m_psParams->m_pcDir->CreateDirectory( m_pcInput->GetBuffer()[0], &cNewDir, S_IRWXU|S_IRWXG|S_IRWXO ) != 0 ) 
						bError = true;
				}
				catch( ... ) {
					bError = true;
				}
				
				if( bError ) {
					os::String zBuffer;
					zBuffer.Format( GS( ID_MSG_NEWDIR_ERROR_TEXT, "The directory %s could not be created!" ).c_str(),  m_pcInput->GetBuffer()[0].c_str() );
					os::Alert* pcAlert = new os::Alert( GS( ID_MSG_NEWDIR_ERROR_TITLE, "New directory" ), zBuffer, os::Alert::ALERT_WARNING, 0, GS( ID_MSG_NEWDIR_ERROR_CLOSE, "Ok" ).c_str(), NULL );
					pcAlert->Go( new os::Invoker( 0 ) );
				}
				
				if( m_psParams->m_pcMsg )
				{
					m_psParams->m_pcMsg->AddBool( "success", !bError );
					m_psParams->m_cViewTarget.SendMessage( m_psParams->m_pcMsg );
				}
				
				PostMessage( os::M_QUIT );
				break;
			}
			default:
				break;
		}
		os::Window::HandleMessage( pcMessage );
	}
	
private:
	NewDirParams_s* m_psParams;
	os::LayoutView*		m_pcView;
	os::Button*			m_pcOk;
	os::TextView*		m_pcInput;
	os::Catalog*		m_pcCatalog;
};


/** Create a dialog which can be used to create a new directory..
 * \par Description:
 *	The created window contains an input field where the user can
 *  type the name of the new directory. If a new directory has been created, 
 *	a message will be sent to the supplied message target.
 * \par Note:
 *	Don’t forget to place the window at the right position and show it.
 * \param cMsgTarget - The target that will receive the message.
 * \param pcCreateMsg - The message that will be sent.
 * \param cDefaultName - The default name of the new directory.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

Window* Directory::CreateDirectoryDialog( const Messenger & cMsgTarget, Message* pcCreateMsg, String cDefaultName )
{
	os::String cPath;
	GetPath( &cPath );
	NewDirParams_s *psParams = new NewDirParams_s( this, cMsgTarget, pcCreateMsg, cDefaultName );
	NewDirWin* pcWin = new NewDirWin( psParams, os::Rect( 0, 0, 250, 70 ) );
	
	return( pcWin );
}
