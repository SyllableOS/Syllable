/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#include <strstream>

using namespace os;

FileRequester::FileRequester( file_req_mode_t	nMode,
			      Messenger*	pcTarget,
			      const char*	pzPath,
			      uint32		nNodeType,
			      bool		bMultiSelect,
			      Message*		pcMessage,
			      FileFilter*	pcFilter,
			      bool		bModal,
			      bool		bHideWhenDone,
			      const char*	pzOkLabel,
			      const char*	pzCancelLabel ) : Window( Rect(0,0,1,1), "file_requester", "" )

{
    Lock();

    if ( pzOkLabel == NULL ) {
	if ( nMode == LOAD_REQ ) {
	    pzOkLabel = "Load";
	} else {
	    pzOkLabel = "Save";
	}
    }
    if ( pzCancelLabel == NULL ) {
	pzCancelLabel = "Cancel";
    }

  
    m_nMode         = nMode;
    m_nNodeType	    = nNodeType;
    m_bHideWhenDone = bHideWhenDone;
    m_pcTarget      = (pcTarget != NULL ) ? pcTarget : new Messenger( Application::GetInstance() );
    m_pcMessage     = pcMessage;

    std::string cPath;
    std::string cFile;

    if ( pzPath == NULL ) {
	const char* pzHome = getenv( "HOME" );
	if ( pzHome != NULL ) {
	    cPath = pzHome;
	    if ( cPath[cPath.size()-1] != '/' ) {
		cPath += '/';
	    }
	} else {
	    cPath = "/tmp/";
	}
    } else {
	cPath = pzPath;
	if ( cPath.find( '/' ) == std::string::npos ) {
	    const char* pzHome = getenv( "HOME" );
	    if ( pzHome != NULL ) {
		cPath = pzHome;
		if ( cPath[cPath.size()-1] != '/' ) {
		    cPath += '/';
		}
	    } else {
		cPath = "/tmp/";
	    }
	    cFile = pzPath;
	} else {
	    cPath = pzPath;
	}
    }
    if ( cFile.empty() ) {
	if ( nNodeType == NODE_FILE ) {
	    struct ::stat sStat;
	    if ( cPath[cPath.size()-1] != '/' || (stat( cPath.c_str(), &sStat ) < 0 || S_ISREG( sStat.st_mode )) ) {
		uint nSlash = cPath.rfind( '/' );
		if ( nSlash != std::string::npos ) {
		    cFile = cPath.c_str() + nSlash + 1;
		    cPath.resize( nSlash );
		} else {
		    cFile = cPath;
		    cPath = "";
		}
	    }
	}
    }
    m_pcDirView	     = new DirectoryView( GetBounds(), cPath );
    m_pcPathView     = new TextView( Rect(0,0,1,1), "path_edit", cFile.c_str() );
    m_pcCancelButton = new Button( Rect(0,0,1,1), "cancel", pzCancelLabel, new Message( ID_CANCEL ) );
    m_pcOkButton     = new Button( Rect(0,0,1,1), "ok", pzOkLabel, new Message( ID_OK ) );

    AddChild( m_pcDirView );
    AddChild( m_pcPathView );
    AddChild( m_pcCancelButton );
    AddChild( m_pcOkButton );

    m_pcDirView->SetMultiSelect( bMultiSelect );
    m_pcDirView->SetSelChangeMsg( new Message( ID_SEL_CHANGED ) );
    m_pcDirView->SetInvokeMsg( new Message( ID_INVOKED ) );
    m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    SetTitle( m_pcDirView->GetPath().c_str() );

    Rect cFrame( 250, 150, 699, 499 );
    SetFrame( cFrame );
    Unlock();
}

void FileRequester::Layout()
{
    Rect cBounds = GetBounds();

    Point cSize1 = m_pcOkButton->GetPreferredSize( false );
    Point cSize2 = m_pcCancelButton->GetPreferredSize( false );

    if ( cSize2.x > cSize1.x ) {
	cSize1 = cSize2;
    }

    Rect cOkRect = cBounds;

    cOkRect.bottom -= 5;
    cOkRect.top = cOkRect.bottom - cSize1.y;
    cOkRect.right -= 10;
    cOkRect.left = cOkRect.right - cSize1.x;
  
    Rect cCancelRect = cOkRect;
    cCancelRect.left -= cSize1.x + 15;
    cCancelRect.right -= cSize1.x + 15;

    Point cSize = m_pcPathView->GetPreferredSize( false );
    Rect cPathFrame = cBounds;

    cPathFrame.bottom = cOkRect.top - 10;
    cPathFrame.top = cPathFrame.bottom - cSize.y;
    cPathFrame.left += 10;
    cPathFrame.right -= 10;
  
    Rect cDirFrame = cBounds;
    cDirFrame.bottom = cPathFrame.top - 10;
    cDirFrame.top += 10;
    cDirFrame.left += 10;
    cDirFrame.right -= 10;
  
    m_pcPathView->SetFrame( cPathFrame );
    m_pcDirView->SetFrame( cDirFrame );
    m_pcOkButton->SetFrame( cOkRect );
    m_pcCancelButton->SetFrame( cCancelRect );
}


void FileRequester::FrameSized( const Point& cDelta )
{
    Layout();
}

void FileRequester::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_PATH_CHANGED:
	    SetTitle( m_pcDirView->GetPath().c_str() );
	    break;
	case ID_SEL_CHANGED:
	{
	    int nSel = m_pcDirView->GetFirstSelected();
	    if ( nSel >= 0 ) {
		FileRow* pcFile = m_pcDirView->GetFile( nSel );
		if ( (m_nNodeType & NODE_DIR) || S_ISDIR( pcFile->GetFileStat().st_mode ) == false ) {
		    m_pcPathView->Set( pcFile->GetName().c_str() );
		}
	    }
//	    else {
//		m_pcPathView->Set( "" );
//	    }
	    break;
	}
	case ID_ALERT: // User has ansvered the "Are you sure..." requester
	{
	    int32 nButton;
	    if ( pcMessage->FindInt32( "which", &nButton ) != 0 ) {
		dbprintf( "FileRequester::HandleMessage() message from alert "
			  "requester does not contain a 'which' element!\n" );
		break;
	    }
	    if ( nButton == 0 ) {
		break;
	    }
	      /*** FALL THROUGH ***/
	}
	case ID_OK:
	case ID_INVOKED:
	{
	    Message* pcMsg;
	    if ( m_pcMessage != NULL ) {
		pcMsg = new Message( *m_pcMessage );
	    } else {
		pcMsg = new Message( (m_nMode == LOAD_REQ) ? M_LOAD_REQUESTED : M_SAVE_REQUESTED );
	    }
	    pcMsg->AddPointer( "source", this );

	    if ( m_nMode == LOAD_REQ ) {
		for ( int i = m_pcDirView->GetFirstSelected() ; i <= m_pcDirView->GetLastSelected() ; ++i ) {
		    if ( m_pcDirView->IsSelected( i ) == false ) {
			continue;
		    }
		    Path cPath( m_pcDirView->GetPath().c_str() );
		    cPath.Append( m_pcDirView->GetFile(i)->GetName().c_str() );
		    pcMsg->AddString( "file/path", cPath.GetPath() );
		}
		m_pcTarget->SendMessage( pcMsg );
		Show( false );
	    } else {
		Path cPath( m_pcDirView->GetPath().c_str() );
		cPath.Append( m_pcPathView->GetBuffer()[0].c_str() );

		struct ::stat sStat;
		if ( pcMessage->GetCode() != ID_ALERT && ::stat( cPath.GetPath(), &sStat ) >= 0 ) {
		    std::strstream cMsg;
		    cMsg << "The file " << cPath.GetPath() << " already exists\n"
			 << "Do you want to overwrite it?\n";
	  
		    Alert* pcAlert = new Alert( "Alert:", cMsg.str(), 0, "No", "Yes", NULL );
		    pcAlert->Go( new Invoker( new Message( ID_ALERT ), this ) );
		} else {
		    pcMsg->AddString( "file/path", cPath.GetPath() );
		    m_pcTarget->SendMessage( pcMsg );
		    Show( false );
		}
	    }
	    delete pcMsg;
	    break;
	}
	case ID_CANCEL:
	    Show( false );
	    break;
	default:
	    Window::HandleMessage( pcMessage );
	    break;
    }
}

void FileRequester::SetPath( const std::string& a_cPath )
{
    std::string cPath = a_cPath;
    std::string cFile;


    if ( cPath.find( '/' ) == std::string::npos ) {
	const char* pzHome = getenv( "HOME" );
	if ( pzHome != NULL ) {
	    cPath = pzHome;
	    if ( cPath[cPath.size()-1] != '/' ) {
		cPath += '/';
	    }
	} else {
	    cPath = "/tmp/";
	}
	cFile = a_cPath;
    }
    if ( cFile.empty() ) {
	if ( m_nNodeType == NODE_FILE ) {
	    struct ::stat sStat;
	    if ( cPath[cPath.size()-1] != '/' || (stat( cPath.c_str(), &sStat ) < 0 || S_ISREG( sStat.st_mode )) ) {
		uint nSlash = cPath.rfind( '/' );
		if ( nSlash != std::string::npos ) {
		    cFile = cPath.c_str() + nSlash + 1;
		    cPath.resize( nSlash );
		} else {
		    cFile = cPath;
		    cPath = "";
		}
	    }
	}
    }
    m_pcPathView->Set( cFile.c_str(), false );
    m_pcDirView->SetPath( cPath );
}

std::string FileRequester::GetPath() const
{
    return( m_pcDirView->GetPath() );
}
