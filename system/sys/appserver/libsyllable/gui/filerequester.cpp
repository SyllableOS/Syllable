/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003 - 2004  The Syllable Team
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

#include <gui/filerequester.h>
#include <gui/icondirview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/requesters.h>
#include <gui/layoutview.h>
#include <util/message.h>
#include <util/application.h>
#include <gui/dropdownmenu.h>
#include <gui/imagebutton.h>
#include <gui/image.h>
#include <util/locale.h>
#include <util/application.h>
#include <storage/memfile.h>
#include <string>
#include <stack>

#include "catalogs/libsyllable.h"

using namespace os;

static uint8 g_aBackImage[] = {
#include "pixmaps/back.h"
};

static uint8 g_aUpImage[] = {
#include "pixmaps/up.h"
};

static uint8 g_aHomeImage[] = {
#include "pixmaps/home.h"
};

class FileRequester::Private
{
public:

	Private()
	{
		m_bFirstReread = true;
		m_pcMessage = NULL;
		m_pcTarget = NULL;
		m_pcCatalog = NULL;
	}
	
	~Private()
	{
		if( m_pcCatalog ) {
			m_pcCatalog->Release();
		}
	}

	DirectoryIconData* GetFirstDirectory()
	{
		for( uint i = 0; i < m_pcDirView->GetIconCount(); i++ )
		{
			if( m_pcDirView->GetIconSelected( i ) )
			{
				IconData* pcIconData = m_pcDirView->GetIconData( i );
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( pcIconData );
			
				if( S_ISDIR( pcData->m_sStat.st_mode ) )	
					return pcData;			
			}
		}

		return NULL;
	}
	
	void PathChanged( Window* win )
	{
		String cTitlePath = m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_BROWSE_LOCATION_LABEL, "Searching in: " ) + m_pcDirView->GetPath();
		win->SetTitle( cTitlePath );

		if( m_pcTypeDrop->GetItemCount() >= 5 )
		{
			for( int i = 5; i <= m_pcTypeDrop->GetItemCount(); i++ )
				m_pcTypeDrop->DeleteItem( i );
		}
			
		std::string cDropString = m_pcDirView->GetPath() + ( std::string ) "/";

		if( cDropString != "//" )
		{
			m_pcTypeDrop->InsertItem( 5, cDropString.c_str() );
			m_pcTypeDrop->SetSelection( 5, false );
		}
		else
		{
			m_pcTypeDrop->InsertItem( 5, "/" );
			m_pcTypeDrop->SetSelection( 5, false );
		}
			
		m_cBackStack.push( m_pcDirView->GetPath() + "/" );
		if( m_cBackStack.size() > 1 )
			m_pcBackButton->SetEnable( true );
		else
			m_pcBackButton->SetEnable( false );
	}

	bool m_bFirstReread;

	Message *m_pcMessage;
	Messenger *m_pcTarget;

	file_req_mode_t m_nMode;
	uint32 m_nNodeType;
	bool m_bHideWhenDone;
	LayoutView* m_pcRoot;
	IconDirectoryView *m_pcDirView;
	TextView *m_pcPathView;
	Button *m_pcOkButton;
	Button *m_pcCancelButton;
	StringView *m_pcFileString;
	DropdownMenu *m_pcTypeDrop;
	ImageButton *m_pcUpButton;
	ImageButton *m_pcHomeButton;
	ImageButton *m_pcBackButton;
	std::stack<String> m_cBackStack;
	Catalog* m_pcCatalog;
};


/** Constructor.
 * \par Description:
 * Filerequester constructure. 
 * \param nMode - Whether the filerequester is used to load or save files.
 * \param zName - The target of the messages.
 * \param pzStartPath - The start path. Default is the home directory of the user.
 * \param nNodeType - When NODE_DIR is given, also directories can be opened.
 * \param bMultiSelect - Whether multiple files can be selected.
 * \param pcMessage - A message which overrieds M_LOAD_REQUESTED/M_SAVE_REQUESTED.
 * \param pcFilter - A file filter. NOT SUPPORTED YET.
 * \param bModal - NOT SUPPORTED YET.
 * \param bHideWhenDone - NOT SUPPORTED YET.
 * \param cOKLabel - Overrides "Load" or "Save".
 * \param cCancelLabel - Overrides "Cancel".
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
FileRequester::FileRequester( file_req_mode_t nMode, Messenger * pcTarget, String cStartPath, uint32 nNodeType, bool bMultiSelect, Message * pcMessage, FileFilter * pcFilter, bool bModal, bool bHideWhenDone, String cOkLabel, String cCancelLabel ):Window( Rect( 0, 0, 1, 1 ), "file_requester", "", 0 )
{
	Lock();		// Why ??

	m = new Private;	

	m->m_pcCatalog = Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );
	if( !m->m_pcCatalog ) {
		m->m_pcCatalog = new Catalog;
	}

	if( cOkLabel.empty() )
	{
		if( nMode == LOAD_REQ )
		{
			cOkLabel = m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_LOAD_BUTTON, "Load" );
		}
		else
		{
			cOkLabel = m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_SAVE_BUTTON, "Save" );
		}
	}
	if( cCancelLabel.empty() )
	{
		cCancelLabel = m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_CANCEL_BUTTON, "Cancel" );
	}


	m->m_nMode = nMode;
	m->m_nNodeType = nNodeType;
	m->m_bHideWhenDone = bHideWhenDone;
	m->m_pcTarget = ( pcTarget != NULL ) ? pcTarget : new Messenger( Application::GetInstance() );
	m->m_pcMessage = pcMessage;

	std::string cPath;
	std::string cFile;

	if( cStartPath == "" )
	{
		const char *pzHome = getenv( "HOME" );

		if( pzHome != NULL )
		{
			cPath = pzHome;
			if( cPath[cPath.size() - 1] != '/' )
			{
				cPath += '/';
			}
		}
		else
		{
			cPath = "/tmp/";
		}
	}
	else
	{
		cPath = cStartPath;
		if( cPath.find( '/' ) == std::string::npos )
		{
			const char *pzHome = getenv( "HOME" );

			if( pzHome != NULL )
			{
				cPath = pzHome;
				if( cPath[cPath.size() - 1] != '/' )
				{
					cPath += '/';
				}
			}
			else
			{
				cPath = "/tmp/";
			}
			cFile = cStartPath;
		}
		else
		{
			cPath = cStartPath;
		}
	}
	if( cFile.empty() )
	{
		if( nNodeType == NODE_FILE )
		{
			struct stat sStat;

			if( cPath[cPath.size() - 1] != '/' || ( stat( cPath.c_str(  ), &sStat ) < 0 || S_ISREG( sStat.st_mode ) ) )
			{
				uint nSlash = cPath.rfind( '/' );

				if( nSlash != std::string::npos )
				{
					cFile = cPath.c_str() + nSlash + 1;
					cPath.resize( nSlash );
				}
				else
				{
					cFile = cPath;
					cPath = "";
				}
			}
		}
	}

	String cRootName = String( getenv( "HOME" ) ) + "/";
	
	m->m_pcRoot = new LayoutView( GetBounds(), "layout_root" );
	
	VLayoutNode* pcVRoot = new VLayoutNode( "vlayout_root" );
	pcVRoot->SetBorders( Rect( 10, 5, 10, 5 ) );
	
	HLayoutNode* pcHButtons = new HLayoutNode( "hlayout_buttons", 0.0f );
	
	m->m_pcBackButton = new ImageButton( Rect( 0, 0, 0, 0 ), "BackBut", "", new Message( ID_BACK_BUT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	MemFile* pcSource = new MemFile( g_aBackImage, sizeof( g_aBackImage ) );
	m->m_pcBackButton->SetImage( pcSource );
	m->m_pcBackButton->SetEnable( false );
	delete( pcSource );
	
	m->m_pcUpButton = new ImageButton( Rect(), "UpBut", "", new Message( ID_UP_BUT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	pcSource = new MemFile( g_aUpImage, sizeof( g_aUpImage ) );
	m->m_pcUpButton->SetImage( pcSource );
	delete( pcSource );

	m->m_pcHomeButton = new ImageButton( Rect( 0, 0, 0, 0 ), "HomeBut", "", new Message( ID_HOME_BUT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	pcSource = new MemFile( g_aHomeImage, sizeof( g_aHomeImage ) );
	m->m_pcHomeButton->SetImage( pcSource );
	delete( pcSource );

	m->m_pcTypeDrop = new DropdownMenu( Rect(), "Drop" );
	m->m_pcTypeDrop->SetMaxPreferredSize( 1000 );
	
	pcHButtons->AddChild( m->m_pcBackButton, 0.0f );
	pcHButtons->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHButtons->AddChild( m->m_pcUpButton, 0.0f );
	pcHButtons->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHButtons->AddChild( m->m_pcHomeButton, 0.0f );
	pcHButtons->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHButtons->AddChild( m->m_pcTypeDrop );

	m->m_pcDirView = new IconDirectoryView( Rect(), cPath );
	
	HLayoutNode* pcHBottom = new HLayoutNode( "hlayout_bottom", 0.0f );
	
	m->m_pcPathView = new TextView( Rect(), "path_edit", cFile.c_str() );
	m->m_pcCancelButton = new Button( Rect(), "cancel", cCancelLabel, new Message( ID_CANCEL ) );
	m->m_pcOkButton = new Button( Rect(), "ok", cOkLabel, new Message( ID_OK ) );
	m->m_pcFileString = new StringView( Rect(), "string", m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_FILE_NAME_LABEL, "File Name:" ) );
	
	pcHBottom->AddChild( m->m_pcFileString, 0.0f );
	pcHBottom->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHBottom->AddChild( m->m_pcPathView );
	pcHBottom->AddChild( new HLayoutSpacer( "", 10.0f, 10.0f ) );
	pcHBottom->AddChild( m->m_pcCancelButton, 0.0f );
	pcHBottom->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHBottom->AddChild( m->m_pcOkButton, 0.0f );

	
	pcVRoot->AddChild( pcHButtons );
	pcVRoot->AddChild( new VLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVRoot->AddChild( m->m_pcDirView );
	pcVRoot->AddChild( new VLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVRoot->AddChild( pcHBottom );
	
	m->m_pcRoot->SetRoot( pcVRoot );
	AddChild( m->m_pcRoot );
	
	m->m_pcTypeDrop->SetReadOnly( true );
	m->m_pcTypeDrop->AppendItem( cRootName );
	m->m_pcTypeDrop->AppendItem( cRootName + "Documents/" );
	m->m_pcTypeDrop->AppendItem( cRootName + "Trash/" );
	m->m_pcTypeDrop->AppendItem( "/Applications/" );
	m->m_pcTypeDrop->AppendItem( "/system/" );

	std::string cDropString = m->m_pcDirView->GetPath() + ( std::string ) "/";
	m->m_pcTypeDrop->InsertItem( 5, cDropString.c_str() );
	m->m_pcTypeDrop->SetSelection( 5, false );
	m->m_pcTypeDrop->SetSelectionMessage( new Message( ID_DROP_CHANGE ) );
	m->m_pcTypeDrop->SetTarget( this );

	m->m_pcDirView->SetAutoLaunch( false );
	m->m_pcDirView->SetMultiSelect( bMultiSelect );
	m->m_pcDirView->SetSelChangeMsg( new Message( ID_SEL_CHANGED ) );
	m->m_pcDirView->SetInvokeMsg( new Message( ID_INVOKED ) );
	m->m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
	
	String cTitlePath = m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_BROWSE_LOCATION_LABEL, "Searching in: " ) + m->m_pcDirView->GetPath();
	SetTitle( cTitlePath );
	
	m->m_cBackStack.push( cPath );

	Rect cFrame( 250, 150, 699, 400 );

	SetFrame( cFrame );
	SetDefaultButton( m->m_pcOkButton );
	AddShortcut( ShortcutKey( 27 ), new Message( ID_CANCEL ) );	// ESC

	m->m_pcDirView->SetView( IconView::VIEW_DETAILS );
	m->m_pcDirView->MakeFocus();
	
	Unlock();
}

FileRequester::~FileRequester()
{
	delete m;
}

void FileRequester::Show( bool bMakeVisible )
{
	if( bMakeVisible && m->m_bFirstReread )
	{
		m->m_bFirstReread = false;
		m->m_pcDirView->ReRead();
		
	}
	Window::Show( bMakeVisible );
}

bool FileRequester::OkToQuit( void )
{
	Message *pcMsg = new Message( M_FILE_REQUESTER_CANCELED );

	pcMsg->AddPointer( "source", this );
	m->m_pcTarget->SendMessage( pcMsg );
	Show( false );
	return ( false );
}

void FileRequester::FrameSized( const Point & cDelta )
{
}

void FileRequester::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{

	case ID_DROP_CHANGE:
		{
			uint32 nSwitch = m->m_pcTypeDrop->GetSelection();

			String cChangePath = m->m_pcTypeDrop->GetItem( nSwitch );
			SetPath( cChangePath );
			break;
		}

	case ID_PATH_CHANGED:
		{
			m->PathChanged( this );
			break;
		}
	case ID_BACK_BUT:
	{
		if( m->m_cBackStack.size() > 1 )
		{
			m->m_cBackStack.pop();
			String cNewPath = m->m_cBackStack.top();
			m->m_cBackStack.pop();
			//printf( "%s\n", cNewPath.c_str() );
			SetPath( cNewPath );
			m->PathChanged( this );		
		}
		break;
	}
	case ID_HOME_BUT:
		{
			std::string cRootName = getenv( "HOME" ) + ( std::string ) "/";
			SetPath( cRootName );
			m->PathChanged( this );
			break;
		}

	case ID_UP_BUT:
		{
			Path cNewPath = m->m_pcDirView->GetPath();
			cNewPath.Append( ".." );
			m->m_pcDirView->SetPath( cNewPath );
			m->m_pcDirView->MakeFocus();
			m->m_pcDirView->ReRead();
			m->PathChanged( this );
			break;
		}

	case ID_SEL_CHANGED:
		{
			for( uint i = 0; i < m->m_pcDirView->GetIconCount(); i++ )
			{
				if( m->m_pcDirView->GetIconSelected( i ) )
				{
					IconData* pcIconData = m->m_pcDirView->GetIconData( i );
					DirectoryIconData *pcData = static_cast < DirectoryIconData * >( pcIconData );
					if( ( m->m_nNodeType & NODE_DIR ) || S_ISDIR( pcData->m_sStat.st_mode ) == false )
					{
						m->m_pcPathView->Set( m->m_pcDirView->GetIconString( i, 0 ).c_str() );
					}
				}
			}
			break;
		}
	case ID_ALERT:		// User has ansvered the "Are you sure..." requester
		{
			int32 nButton;

			if( pcMessage->FindInt32( "which", &nButton ) != 0 )
			{
				dbprintf( "FileRequester::HandleMessage() message from alert " "requester does not contain a 'which' element!\n" );
				break;
			}
			if( nButton == 0 )
			{
				break;
			}

	      /*** FALL THROUGH ***/
		}
	case ID_OK:
	case ID_INVOKED:
		{			
			// Works only with a file requester dialog and not a directory requester dialog
			if( m->m_nNodeType == NODE_FILE )
			{
				// Check if one of the selected icons is directory
				DirectoryIconData *pcData = m->GetFirstDirectory();
				if( pcData != NULL )
				{
					// If it is a directory, change path to this directory
					Path cPath = m->m_pcDirView->GetPath();
					cPath.Append( pcData->m_zPath );
					SetPath( cPath.GetPath() + String( "/" ));	
					m->PathChanged( this );
					break;
				}
			}

			Message *pcMsg;

			if( m->m_pcMessage != NULL )
			{
				pcMsg = new Message( *m->m_pcMessage );
			}
			else
			{
				pcMsg = new Message( ( m->m_nMode == LOAD_REQ ) ? M_LOAD_REQUESTED : M_SAVE_REQUESTED );
			}
			pcMsg->AddPointer( "source", this );

			if( m->m_nMode == LOAD_REQ )
			{
				for( uint i = 0; i < m->m_pcDirView->GetIconCount(); i++ )
				{
					Path cDirPath( m->m_pcDirView->GetPath() );

					if( m->m_pcDirView->GetIconSelected( i ) )
					{
						IconData* pcIconData = m->m_pcDirView->GetIconData( i );
						DirectoryIconData *pcData = static_cast < DirectoryIconData * >( pcIconData );
						
						Path cPath = cDirPath;
						cPath.Append( pcData->m_zPath );
						pcMsg->AddString( "file/path", cPath.GetPath() );
					}
				}
				m->m_pcTarget->SendMessage( pcMsg );
				Show( false );
			}
			else
			{
				Path cPath( m->m_pcDirView->GetPath().c_str(  ) );

				cPath.Append( m->m_pcPathView->GetBuffer()[0].c_str(  ) );

				struct stat sStat;

				if( pcMessage->GetCode() != ID_ALERT && stat( cPath.GetPath(  ).c_str(), &sStat ) >= 0 )
				{
					String cMsg;
					
					cMsg.Format( m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_FILE_ALREADY_EXISTS_TEXT, "The file '%s' already exists\nDo you want to overwrite it?\n" ).c_str(), cPath.GetPath().c_str() );

					Alert *pcAlert = new Alert( m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_FILE_ALREADY_EXISTS_TITLE, "Warning:" ), cMsg.c_str(), Alert::ALERT_WARNING, 0,
							m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_FILE_ALREADY_EXISTS_NO_BUTTON, "No" ).c_str(),
							m->m_pcCatalog->GetString( ID_MSG_FILEREQUESTER_FILE_ALREADY_EXISTS_YES_BUTTON, "Yes" ).c_str(), NULL );

					pcAlert->CenterInWindow( this );
					pcAlert->Go( new Invoker( new Message( ID_ALERT ), this ) );
				}
				else
				{
					pcMsg->AddString( "file/path", cPath.GetPath() );
					m->m_pcTarget->SendMessage( pcMsg );
					Show( false );
				}
			}
			delete pcMsg;

			break;
		}
	case ID_CANCEL:
		{
			Message *pcMsg = new Message( M_FILE_REQUESTER_CANCELED );

			pcMsg->AddPointer( "source", this );
			m->m_pcTarget->SendMessage( pcMsg );
			Show( false );
		}
		break;
	default:
		Window::HandleMessage( pcMessage );
		break;
	}
}


/** Sets a new path
 * \par Description:
 * Sets a new path.
 * \param cNewPath - The new path.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void FileRequester::SetPath( const String & cNewPath )
{
	std::string cPath = cNewPath;
	std::string cFile;


	if( cPath.find( '/' ) == std::string::npos )
	{
		const char *pzHome = getenv( "HOME" );

		if( pzHome != NULL )
		{
			cPath = pzHome;
			if( cPath[cPath.size() - 1] != '/' )
			{
				cPath += '/';
			}
		}
		else
		{
			cPath = "/tmp/";
		}
		cFile = cNewPath;
	}
	if( cFile.empty() )
	{
		if( m->m_nNodeType == NODE_FILE )
		{
			struct stat sStat;

			if( cPath[cPath.size() - 1] != '/' || ( stat( cPath.c_str(  ), &sStat ) < 0 || S_ISREG( sStat.st_mode ) ) )
			{
				uint nSlash = cPath.rfind( '/' );

				if( nSlash != std::string::npos )
				{
					cFile = cPath.c_str() + nSlash + 1;
					cPath.resize( nSlash );
				}
				else
				{
					cFile = cPath;
					cPath = "";
				}
			}
		}
	}

	m->m_pcPathView->Set( cFile.c_str(), false );
	if( String( cPath ) == GetPath() + "/" )
		return;
	m->m_pcDirView->SetPath( cPath );
	m->m_pcDirView->MakeFocus();
	m->m_pcDirView->ReRead();
}


/** Returns the current path.
 * \par Description:
 * Returns the current path.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
String FileRequester::GetPath() const
{
	return ( m->m_pcDirView->GetPath() );
}
