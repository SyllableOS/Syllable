
/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003  The Syllable Team
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
#include <gui/directoryview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/requesters.h>
#include <util/message.h>
#include <util/application.h>
#include <gui/dropdownmenu.h>
#include <gui/imagebutton.h>
#include <gui/image.h>
#include <string>

using namespace os;

uint8 back_image[] = {
#include "pixmaps/back.h"
};

uint8 up_image[] = {
#include "pixmaps/up.h"
};

uint8 home_image[] = {
#include "pixmaps/home.h"
};

uint8 forward_image[] = {
#include "pixmaps/forward.h"
};

class FileRequester::Private
{
      public:

	Private()
	{
		m_pcMessage = NULL;
		m_pcTarget = NULL;
	}

	Message *m_pcMessage;
	Messenger *m_pcTarget;

	file_req_mode_t m_nMode;
	uint32 m_nNodeType;
	bool m_bHideWhenDone;
	DirectoryView *m_pcDirView;
	TextView *m_pcPathView;
	Button *m_pcOkButton;
	Button *m_pcCancelButton;
	StringView *m_pcFileString;
	StringView *m_pcTypeString;
	DropdownMenu *m_pcTypeDrop;
	ImageButton *m_pcUpButton;
	ImageButton *m_pcHomeButton;
	ImageButton *m_pcForwardButton;
	ImageButton *m_pcBackButton;
};

FileRequester::FileRequester( file_req_mode_t nMode, Messenger * pcTarget, const char *pzPath, uint32 nNodeType, bool bMultiSelect, Message * pcMessage, FileFilter * pcFilter, bool bModal, bool bHideWhenDone, const char *pzOkLabel, const char *pzCancelLabel ):Window( Rect( 0, 0, 1, 1 ), "file_requester", "", WND_NOT_RESIZABLE )
{
	Lock();		// Why ??

	m = new Private;

	if( pzOkLabel == NULL )
	{
		if( nMode == LOAD_REQ )
		{
			pzOkLabel = "Load";
		}
		else
		{
			pzOkLabel = "Save";
		}
	}
	if( pzCancelLabel == NULL )
	{
		pzCancelLabel = "Cancel";
	}


	m->m_nMode = nMode;
	m->m_nNodeType = nNodeType;
	m->m_bHideWhenDone = bHideWhenDone;
	m->m_pcTarget = ( pcTarget != NULL ) ? pcTarget : new Messenger( Application::GetInstance() );
	m->m_pcMessage = pcMessage;

	std::string cPath;
	std::string cFile;

	if( pzPath == NULL )
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
		cPath = pzPath;
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
			cFile = pzPath;
		}
		else
		{
			cPath = pzPath;
		}
	}
	if( cFile.empty() )
	{
		if( nNodeType == NODE_FILE )
		{
			struct::stat sStat;

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

	std::string cRootName = getenv( "HOME" ) + ( std::string ) "/";

	m->m_pcDirView = new DirectoryView( GetBounds(), cPath );
	m->m_pcPathView = new TextView( Rect( 0, 0, 1, 1 ), "path_edit", cFile.c_str() );
	m->m_pcCancelButton = new Button( Rect( 0, 0, 1, 1 ), "cancel", pzCancelLabel, new Message( ID_CANCEL ) );
	m->m_pcOkButton = new Button( Rect( 0, 0, 1, 1 ), "ok", pzOkLabel, new Message( ID_OK ) );
	m->m_pcFileString = new StringView( Rect( 0, 0, 1, 1 ), "string", "File Name:" );
	m->m_pcTypeString = new StringView( Rect( 0, 0, 1, 1 ), "string_1", "Look in:" );
	m->m_pcTypeDrop = new DropdownMenu( Rect( 0, 0, 0, 0 ), "Drop" );

	m->m_pcUpButton = new ImageButton( Rect( 0, 0, 0, 0 ), "UpBut", "up", new Message( ID_UP_BUT ), NULL );
	BitmapImage *pImage = new BitmapImage( up_image, IPoint( 16, 16 ), CS_RGB32 );

	m->m_pcUpButton->SetImageFromImage( pImage );


	m->m_pcHomeButton = new ImageButton( Rect( 0, 0, 0, 0 ), "HomeBut", "home", new Message( ID_HOME_BUT ), NULL );
	pImage = new BitmapImage( home_image, IPoint( 16, 16 ), CS_RGB32 );
	m->m_pcHomeButton->SetImageFromImage( pImage );

	m->m_pcBackButton = new ImageButton( Rect( 0, 0, 0, 0 ), "BackBut", "back", new Message( ID_BACK_BUT ), NULL );
	pImage = new BitmapImage( back_image, IPoint( 16, 16 ), CS_RGB32 );
	m->m_pcBackButton->SetImageFromImage( pImage );


	m->m_pcForwardButton = new ImageButton( Rect( 0, 0, 0, 0 ), "ForwardBut", "forward", new Message( ID_FORWARD_BUT ), NULL );
	pImage = new BitmapImage( forward_image, IPoint( 16, 16 ), CS_RGB32 );
	m->m_pcForwardButton->SetImageFromImage( pImage );

	AddChild( m->m_pcForwardButton );
	AddChild( m->m_pcBackButton );
	AddChild( m->m_pcUpButton );
	AddChild( m->m_pcHomeButton );
	AddChild( m->m_pcDirView );
	AddChild( m->m_pcCancelButton );
	AddChild( m->m_pcOkButton );
	AddChild( m->m_pcTypeString );
	AddChild( m->m_pcTypeDrop );
	AddChild( m->m_pcPathView );
	AddChild( m->m_pcFileString );

	if( nMode == 1 )
	{
		m->m_pcTypeString->SetString( "Save in:" );

	}

	m->m_pcTypeDrop->SetReadOnly( true );
	m->m_pcTypeDrop->AppendItem( "/atheos/" );
	m->m_pcTypeDrop->AppendItem( "/bin/" );
	m->m_pcTypeDrop->AppendItem( cRootName.c_str() );
	m->m_pcTypeDrop->AppendItem( "/system/" );
	m->m_pcTypeDrop->AppendItem( "/usr/" );


	std::string cDropString = m->m_pcDirView->GetPath() + ( std::string ) "/";
	m->m_pcTypeDrop->InsertItem( 5, cDropString.c_str() );
	m->m_pcTypeDrop->SetSelection( 5 );
	m->m_pcTypeDrop->SetSelectionMessage( new Message( ID_DROP_CHANGE ) );
	m->m_pcTypeDrop->SetTarget( this );

	m->m_pcDirView->SetMultiSelect( bMultiSelect );
	m->m_pcDirView->SetSelChangeMsg( new Message( ID_SEL_CHANGED ) );
	m->m_pcDirView->SetInvokeMsg( new Message( ID_INVOKED ) );
	m->m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
	m->m_pcDirView->MakeFocus();

	std::string cTitlePath = ( std::string ) "Searching in: " + m->m_pcDirView->GetPath();
	SetTitle( cTitlePath.c_str() );

	Rect cFrame( 250, 150, 699, 400 );

	SetFrame( cFrame );
	SetDefaultButton( m->m_pcOkButton );
	Unlock();



}

FileRequester::~FileRequester()
{
	delete m;
}

void FileRequester::Layout()
{
	Rect cBounds = GetBounds();

	Point cSize1 = m->m_pcOkButton->GetPreferredSize( false );
	Point cSize2 = m->m_pcCancelButton->GetPreferredSize( false );

	if( cSize2.x > cSize1.x )
	{
		cSize1 = cSize2;
	}

	Point cSize = m->m_pcPathView->GetPreferredSize( false );

	Rect cOkRect = cBounds;

	cOkRect.bottom -= 5;
	cOkRect.top = cOkRect.bottom - cSize1.y;
	cOkRect.right -= 70;
	cOkRect.left = cOkRect.right - cSize1.x;

	Rect cCancelRect = cBounds;

	cCancelRect.bottom -= 5;
	cCancelRect.top = cCancelRect.bottom + -cSize2.y;
	cCancelRect.right -= 10;
	cCancelRect.left = cCancelRect.right - cSize2.x;

	Rect cFileFrame = cBounds;

	cFileFrame.bottom = cOkRect.bottom;
	cFileFrame.top = cFileFrame.bottom - cSize.y;
	cFileFrame.left = 10;
	cFileFrame.right = 75;

	Rect cPathFrame = cBounds;

	cPathFrame.bottom = cOkRect.bottom;
	cPathFrame.top = cPathFrame.bottom - cSize.y;
	cPathFrame.left = cFileFrame.right + 10;
	cPathFrame.right -= 130;

	Rect cFile2Frame = cBounds;

	cFile2Frame.bottom = cCancelRect.bottom;
	cFile2Frame.top = cCancelRect.top;
	cFile2Frame.left = cFileFrame.right + 15;
	cFile2Frame.right -= 160;

	Rect cDirFrame = cBounds;

	cDirFrame.bottom = cPathFrame.top - 10;
	cDirFrame.top += 27;
	cDirFrame.left += 10;
	cDirFrame.right -= 10;

	Rect cTypeFrame = cBounds;

	cTypeFrame.bottom = cDirFrame.top - 5;
	cTypeFrame.top = 10;
	cTypeFrame.left = 10;
	cTypeFrame.right = cTypeFrame.left + 55;

	Rect cDropFrame = cBounds;

	cDropFrame.bottom = cTypeFrame.bottom - 2;
	cDropFrame.top = cTypeFrame.top - 10;
	cDropFrame.left = cTypeFrame.right + 5;
	cDropFrame.right -= 100;

	Rect cOkFrame = cBounds;

	cOkFrame.bottom = cPathFrame.bottom;
	cOkFrame.top = cOkFrame.bottom - cSize.y;
	cOkFrame.left = cPathFrame.right + 10;
	cOkFrame.right = 20;



	m->m_pcPathView->SetFrame( cFile2Frame );
	m->m_pcDirView->SetFrame( cDirFrame );
	m->m_pcOkButton->SetFrame( cOkRect );
	m->m_pcCancelButton->SetFrame( cCancelRect );
	m->m_pcFileString->SetFrame( cFileFrame + Point( 0, -2 ) );
	m->m_pcTypeString->SetFrame( cTypeFrame );
	m->m_pcTypeDrop->SetFrame( cDropFrame + Point( 0, 5 ) );
	m->m_pcBackButton->SetFrame( Rect( 351, 3, 367, 23 ) );
	m->m_pcForwardButton->SetFrame( Rect( 369, 3, 384, 23 ) );
	m->m_pcUpButton->SetFrame( Rect( 386, 3, 404, 23 ) );
	m->m_pcHomeButton->SetFrame( Rect( 406, 3, 422, 23 ) );

	m->m_pcTypeDrop->SetTabOrder( 0 );
	m->m_pcDirView->SetTabOrder( 1 );
	m->m_pcPathView->SetTabOrder( 2 );
	m->m_pcOkButton->SetTabOrder( 3 );
	m->m_pcCancelButton->SetTabOrder( 4 );
	SetFocusChild( m->m_pcPathView );

}

bool FileRequester::OkToQuit( void )
{
	Message *pcMsg = new Message( M_FILE_REQUESTER_CANCELED );

	m->m_pcTarget->SendMessage( pcMsg );
	Show( false );
	return ( false );
}

void FileRequester::FrameSized( const Point & cDelta )
{
	Layout();
}

void FileRequester::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{

	case ID_DROP_CHANGE:
		{
			uint32 nSwitch = m->m_pcTypeDrop->GetSelection();

			std::string c_ChangePath = m->m_pcTypeDrop->GetItem( nSwitch );
			SetPath( c_ChangePath );
			m->m_pcDirView->Clear();
			m->m_pcDirView->ReRead();

			PostMessage( new Message( ID_PATH_CHANGED ), this );
			break;
		}

	case ID_PATH_CHANGED:
		{
			std::string cTitlePath = ( std::string ) "Searching in: " + m->m_pcDirView->GetPath();
			SetTitle( cTitlePath.c_str() );

			if( m->m_pcTypeDrop->GetItemCount() >= 5 )
			{
				for( int i = 5; i <= m->m_pcTypeDrop->GetItemCount(); i++ )
					m->m_pcTypeDrop->DeleteItem( i );
			}
			std::string cDropString = m->m_pcDirView->GetPath() + ( std::string ) "/";

			if( cDropString != "//" )
			{
				m->m_pcTypeDrop->InsertItem( 5, cDropString.c_str() );
				m->m_pcTypeDrop->SetSelection( 5 );
			}

			else
			{
				m->m_pcTypeDrop->InsertItem( 5, "/" );
				m->m_pcTypeDrop->SetSelection( 5 );
			}

			break;
		}

	case ID_HOME_BUT:
		{
			std::string cRootName = getenv( "HOME" ) + ( std::string ) "/";
			SetPath( cRootName );
			m->m_pcDirView->Clear();
			m->m_pcDirView->ReRead();
			PostMessage( new Message( ID_PATH_CHANGED ), this );
			break;
		}

	case ID_UP_BUT:
		{
			FileRow *pcFileRow = m->m_pcDirView->GetFile( 0 );

			if( pcFileRow->GetName() == ".." )
			{
				m->m_pcDirView->Invoked( 0, 0 );
				m->m_pcDirView->Clear();
				m->m_pcDirView->ReRead();
				if( m->m_pcTypeDrop->GetItemCount() >= 5 )
				{
					for( int i = 5; i <= m->m_pcTypeDrop->GetItemCount(); i++ )
						m->m_pcTypeDrop->DeleteItem( i );
				}

				std::string cDropString = m->m_pcDirView->GetPath() + ( std::string ) "/";

				if( cDropString != "//" )
				{
					m->m_pcTypeDrop->InsertItem( 5, cDropString.c_str() );
					m->m_pcTypeDrop->SetSelection( 5 );
				}

				else
				{
					m->m_pcTypeDrop->InsertItem( 5, "/" );
					m->m_pcTypeDrop->SetSelection( 5 );
				}
			}
			break;
		}

	case ID_SEL_CHANGED:
		{
			int nSel = m->m_pcDirView->GetFirstSelected();

			if( nSel >= 0 )
			{
				FileRow *pcFile = m->m_pcDirView->GetFile( nSel );

				if( ( m->m_nNodeType & NODE_DIR ) || S_ISDIR( pcFile->GetFileStat().st_mode ) == false )
				{
					m->m_pcPathView->Set( pcFile->GetName().c_str(  ) );

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
				for( int i = m->m_pcDirView->GetFirstSelected(); i <= m->m_pcDirView->GetLastSelected(  ); ++i )
				{
					if( m->m_pcDirView->IsSelected( i ) == false )
					{
						continue;
					}
					Path cPath( m->m_pcDirView->GetPath().c_str(  ) );

					cPath.Append( m->m_pcDirView->GetFile( i )->GetName().c_str(  ) );
					pcMsg->AddString( "file/path", cPath.GetPath() );

				}
				m->m_pcTarget->SendMessage( pcMsg );
				Show( false );
			}
			else
			{
				Path cPath( m->m_pcDirView->GetPath().c_str(  ) );

				cPath.Append( m->m_pcPathView->GetBuffer()[0].c_str(  ) );

				struct::stat sStat;

				if( pcMessage->GetCode() != ID_ALERT &&::stat( cPath.GetPath(  ), &sStat ) >= 0 )
				{
					std::string cMsg = "The file '" + ( std::string ) cPath.GetPath() + "' already exists\nDo you want to overwrite it?\n";

					Alert *pcAlert = new Alert( "Warning:", cMsg.c_str(), Alert::ALERT_WARNING, 0, "No", "Yes", NULL );

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

			pcMsg->AddPointer( "source", pcMsg );
			m->m_pcTarget->SendMessage( pcMsg );
			Show( false );
		}
		break;
	default:
		Window::HandleMessage( pcMessage );
		break;
	}
}

void FileRequester::SetPath( const std::string & a_cPath )
{
	std::string cPath = a_cPath;
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
		cFile = a_cPath;
	}
	if( cFile.empty() )
	{
		if( m->m_nNodeType == NODE_FILE )
		{
			struct::stat sStat;

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
	m->m_pcDirView->SetPath( cPath );
}

std::string FileRequester::GetPath() const
{
	return ( m->m_pcDirView->GetPath() );
}
