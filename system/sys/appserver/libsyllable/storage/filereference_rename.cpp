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
#include <storage/filereference.h>
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


struct RenameFileParams_s
{
	RenameFileParams_s( String cPath, const Messenger & cViewTarget, Message* pcMsg )
	:m_cPath( cPath ), m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg )
	{
		
	}
	String m_cPath;
	Messenger m_cViewTarget;
	Message* m_pcMsg;
	
};

class RenameFileWin : public os::Window
{
public:
	RenameFileWin( RenameFileParams_s* psParams, os::Rect cFrame ) : os::Window( cFrame, "rename_window", 
																			"Rename file", os::WND_NOT_V_RESIZABLE )
	{
		m_pcCatalog = os::Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );

		m_psParams = psParams;
		
		os::String cTitle;
		cTitle.Format( GS( ID_MSG_RENAMEFILE_WINDOW_TITLE, "Rename %s" ).c_str(),
			os::Path( psParams->m_cPath ).GetLeaf().c_str() );
			
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
		
		m_pcInput->Set( os::Path( psParams->m_cPath ).GetLeaf().c_str() );
		pcVRoot->AddChild( m_pcInput ); 
		pcVRoot->AddChild( new os::VLayoutSpacer( "" ) );
		
	
		/* create buttons */
		m_pcOk = new os::Button( os::Rect(), "ok", 
						GS( ID_MSG_RENAMEFILE_WINDOW_OK_BUTTON, "Ok" ), new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	
		
		pcHNode->AddChild( new os::HLayoutSpacer( "" ) );
		pcHNode->AddChild( m_pcOk )->LimitMaxSize( m_pcOk->GetPreferredSize( false ) );
		pcVRoot->AddChild( pcHNode );
		
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcOk->SetTabOrder( 0 );
		m_pcInput->SetTabOrder( 0 );
		
		SetDefaultButton( m_pcOk );
		
		m_pcOk->MakeFocus();
		
		
	}
	

	~RenameFileWin() { 
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
				os::String cPath;
				cPath = m_psParams->m_cPath;
				
				try {
					
					
					//printf( "%s %s %s\n", os::Path( cPath ).GetDir().GetPath().c_str(),
					//					os::Path( cPath ).GetLeaf().c_str(),
						//				m_pcInput->GetBuffer()[0].c_str() );
										
					if( os::Path( cPath ).GetLeaf() == m_pcInput->GetBuffer()[0] )
					{
						if( m_psParams->m_pcMsg )
						{
							m_psParams->m_pcMsg->AddBool( "success", true );
							m_psParams->m_cViewTarget.SendMessage( m_psParams->m_pcMsg );
						}
				
						PostMessage( os::M_QUIT );
						break;
					}
				
					try
					{
						FileReference cRef( cPath );
						if( cRef.Rename( m_pcInput->GetBuffer()[0] ) != 0 )
							bError = true;
					} catch( ... )
					{
						bError = true;
					}
					
				}
				catch( ... ) {
					bError = true;
				}
				
				if( bError ) {
					os::String zBuffer;
					zBuffer.Format( GS( ID_MSG_RENAMEFILE_ERROR_TEXT, "%s could not be renamed!" ).c_str(), os::Path( cPath ).GetLeaf().c_str() );
					os::Alert* pcAlert = new os::Alert( GS( ID_MSG_RENAMEFILE_ERROR_TITLE, "Rename" ), zBuffer, os::Alert::ALERT_WARNING, 0, GS( ID_MSG_RENAMEFILE_ERROR_CLOSE, "Ok" ).c_str(), NULL );
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
	RenameFileParams_s* m_psParams;
	os::LayoutView*		m_pcView;
	os::Button*			m_pcOk;
	os::TextView*		m_pcInput;
	os::Catalog*		m_pcCatalog;
};


/** Create a dialog which can be used to rename a directory or file.
 * \par Description:
 *	The created window contains an input field where the user can
 *  type the new name of the directory/file. A message will be sent to the
 *  supplied message target if the file/directory has been renamed.
 * \par Note:
 *	Don’t forget to place the window at the right position and show it.
 * \param cMsgTarget - The target that will receive the message.
 * \param pcMsg - The message that will be sent.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

Window* FileReference::RenameDialog( const Messenger & cMsgTarget, Message* pcMsg )
{
	os::String cPath;
	GetPath( &cPath );
	RenameFileParams_s *psParams = new RenameFileParams_s( cPath, cMsgTarget, pcMsg );
	RenameFileWin* pcWin = new RenameFileWin( psParams, os::Rect( 0, 0, 250, 70 ) );
	
	return( pcWin );
}
