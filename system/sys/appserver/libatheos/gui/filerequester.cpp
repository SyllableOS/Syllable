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
#include <gui/dropdownmenu.h>
#include <gui/imagebutton.h>
#include <gui/image.h>
#include <string>

using namespace os;

uint8 back_image[]= {  
	#include "pixmaps/back.h"
};

uint8 up_image[]= {  
	#include "pixmaps/up.h"
};

uint8 home_image[]= {  
	#include "pixmaps/home.h"
};

uint8 forward_image[]= {  
	#include "pixmaps/forward.h"
};


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
			      const char*	pzCancelLabel ) : Window( Rect(0,0,1,1), "file_requester", "",WND_NOT_RESIZABLE )

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
    
    std::string cRootName = getenv("HOME") + (std::string) "/";
    
    m_pcDirView	     = new DirectoryView( GetBounds(), cPath );
    m_pcPathView     = new TextView( Rect(0,0,1,1), "path_edit", cFile.c_str() );
    m_pcCancelButton = new Button( Rect(0,0,1,1), "cancel", pzCancelLabel, new Message( ID_CANCEL ) );
    m_pcOkButton     = new Button( Rect(0,0,1,1), "ok", pzOkLabel, new Message( ID_OK ) );
    m_pcFileString   = new StringView(Rect(0,0,1,1),"string","File Name:");
    m_pcTypeString   = new StringView(Rect(0,0,1,1),"string_1","Look in:");
    m_pcTypeDrop     = new DropdownMenu(Rect(0,0,0,0),"Drop");
    
    m_pcUpButton = new ImageButton(Rect(0,0,0,0), "UpBut", "up",new Message(ID_UP_BUT), NULL);
   	BitmapImage* pImage = new BitmapImage(up_image, IPoint(16,16),CS_RGB32);
   	m_pcUpButton->SetImageFromImage(pImage); 

    
    m_pcHomeButton = new ImageButton(Rect(0,0,0,0), "HomeBut", "home",new Message(ID_HOME_BUT), NULL);
 	pImage = new BitmapImage(home_image, IPoint(16,16),CS_RGB32);
   	m_pcHomeButton->SetImageFromImage(pImage); 
    
    m_pcBackButton = new ImageButton(Rect(0,0,0,0), "BackBut", "back",new Message(ID_BACK_BUT), NULL);
 	pImage = new BitmapImage(back_image, IPoint(16,16),CS_RGB32);
   	m_pcBackButton->SetImageFromImage(pImage); 

    
	m_pcForwardButton = new ImageButton(Rect(0,0,0,0), "ForwardBut", "forward",new Message(ID_FORWARD_BUT), NULL);
	pImage = new BitmapImage(forward_image, IPoint(16,16),CS_RGB32);
   	m_pcForwardButton->SetImageFromImage(pImage); 
    
    AddChild(m_pcForwardButton); 
    AddChild(m_pcBackButton); 
    AddChild(m_pcUpButton); 
    AddChild(m_pcHomeButton);
    AddChild( m_pcDirView );
    AddChild( m_pcCancelButton );
    AddChild( m_pcOkButton );
    AddChild( m_pcTypeString);
    AddChild( m_pcTypeDrop);
    AddChild( m_pcPathView );
    AddChild( m_pcFileString);
    
    if (nMode == 1){
    m_pcTypeString->SetString("Save in:");
    
   }
   
	m_pcTypeDrop->SetReadOnly(true);
    m_pcTypeDrop->AppendItem("/atheos/");
    m_pcTypeDrop->AppendItem("/bin/");
    m_pcTypeDrop->AppendItem(cRootName.c_str());
    m_pcTypeDrop->AppendItem("/system/");
    m_pcTypeDrop->AppendItem("/usr/");
    
    
    std::string cDropString = m_pcDirView->GetPath() + (std::string)"/";
    m_pcTypeDrop->InsertItem(5,cDropString.c_str());
    m_pcTypeDrop->SetSelection(5);
    m_pcTypeDrop->SetSelectionMessage(new Message(ID_DROP_CHANGE));
	m_pcTypeDrop->SetTarget(this);
	
    m_pcDirView->SetMultiSelect( bMultiSelect );
    m_pcDirView->SetSelChangeMsg( new Message( ID_SEL_CHANGED ) );
    m_pcDirView->SetInvokeMsg( new Message( ID_INVOKED ) );
    m_pcDirView->SetDirChangeMsg( new Message( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    
    std::string cTitlePath = (std::string)"Searching in: " + m_pcDirView->GetPath(); 
    SetTitle( cTitlePath.c_str() );

    Rect cFrame( 250, 150, 699, 400 );
    SetFrame( cFrame );
    SetDefaultButton(m_pcOkButton);
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

	Point cSize = m_pcPathView->GetPreferredSize( false );
	
    Rect cOkRect = cBounds;
	cOkRect.bottom -= 5;
    cOkRect.top = cOkRect.bottom - cSize1.y;
    cOkRect.right -= 70;
    cOkRect.left = cOkRect.right - cSize1.x;
  
    Rect cCancelRect = cBounds;
    cCancelRect.bottom -= 5;
    cCancelRect.top = cCancelRect.bottom + - cSize2.y;
    cCancelRect.right -= 10;
    cCancelRect.left = cCancelRect.right - cSize2.x;
  
	Rect cFileFrame = cBounds;
    cFileFrame.bottom =  cOkRect.bottom;
    cFileFrame.top = cFileFrame.bottom - cSize.y;
    cFileFrame.left = 10;
    cFileFrame.right = 75;
    
    Rect cPathFrame = cBounds;
	cPathFrame.bottom = cOkRect.bottom;
    cPathFrame.top = cPathFrame.bottom - cSize.y;
    cPathFrame.left  = cFileFrame.right + 10;
    cPathFrame.right -= 130;
    
    Rect cFile2Frame = cBounds;
    cFile2Frame.bottom =  cCancelRect.bottom;
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
    cTypeFrame.top =  10;
    cTypeFrame.left = 10;
    cTypeFrame.right = cTypeFrame.left + 55;
    
    Rect cDropFrame = cBounds;
    cDropFrame.bottom =  cTypeFrame.bottom -2;
    cDropFrame.top = cTypeFrame.top - 10;
    cDropFrame.left = cTypeFrame.right + 5;
    cDropFrame.right -= 100;
    
    Rect cOkFrame = cBounds;
    cOkFrame.bottom = cPathFrame.bottom;
    cOkFrame.top = cOkFrame.bottom - cSize.y;
    cOkFrame.left = cPathFrame.right + 10;
    cOkFrame.right = 20;
    
    

    m_pcPathView->SetFrame( cFile2Frame );
    m_pcDirView->SetFrame( cDirFrame );
    m_pcOkButton->SetFrame(cOkRect);
    m_pcCancelButton->SetFrame( cCancelRect );
    m_pcFileString->SetFrame( cFileFrame + Point(0,-2));
    m_pcTypeString->SetFrame(cTypeFrame);
    m_pcTypeDrop->SetFrame(cDropFrame + Point(0,5));
    m_pcBackButton->SetFrame(Rect(351, 3,367, 23));
    m_pcForwardButton->SetFrame(Rect(369, 3, 384, 23));
   	m_pcUpButton->SetFrame(Rect(386,3,404,23));
   	m_pcHomeButton->SetFrame(Rect(406,3,422,23));
   	
   	m_pcTypeDrop->SetTabOrder(0);
    m_pcDirView->SetTabOrder(1);
    m_pcPathView->SetTabOrder(2);
    m_pcOkButton->SetTabOrder(3);
    m_pcCancelButton->SetTabOrder(4);
    SetFocusChild(m_pcPathView);
    
}

bool FileRequester::OkToQuit(void)
{
	Message* pcMsg = new Message(M_FILE_REQUESTER_CANCELED);
	m_pcTarget->SendMessage(pcMsg);
	Show(false);
	return (false);
}

void FileRequester::FrameSized( const Point& cDelta )
{
    Layout();
}

void FileRequester::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
    	
    case ID_DROP_CHANGE:
    	{
    		uint32 nSwitch = m_pcTypeDrop->GetSelection();
    		std::string c_ChangePath = m_pcTypeDrop->GetItem(nSwitch);
    	    SetPath(c_ChangePath);
    		m_pcDirView->Clear();
    		m_pcDirView->ReRead();
    		
    		PostMessage(new Message(ID_PATH_CHANGED),this);
    	break;
    	} 
    		
	case ID_PATH_CHANGED:
		{
    	std::string cTitlePath = (std::string)"Searching in: " + m_pcDirView->GetPath(); 
    	SetTitle( cTitlePath.c_str() );
    	
    	if (m_pcTypeDrop->GetItemCount() >=5)
	 	 		{
	 	 			for (uint i=5; i<=m_pcTypeDrop->GetItemCount(); i++)
	 	 				m_pcTypeDrop->DeleteItem(i);
	 	 		} 		
    	std::string cDropString = m_pcDirView->GetPath() + (std::string)"/";
    	
    	if (cDropString != "//"){
    		m_pcTypeDrop->InsertItem(5,cDropString.c_str());
    		m_pcTypeDrop->SetSelection(5);
    	}
    	
    	else{
    		m_pcTypeDrop->InsertItem(5,"/");
    		m_pcTypeDrop->SetSelection(5);
		}
		    	
	    break;
	    }
	    
	 case ID_HOME_BUT:
	 	{
	 		std::string cRootName = getenv("HOME") + (std::string) "/";
	 		SetPath(cRootName);
	 		m_pcDirView->Clear();
	 	 	m_pcDirView->ReRead();	
	 	 	PostMessage(new Message(ID_PATH_CHANGED),this);
	 		break;
	 	}
	 	
	 case ID_UP_BUT:
	 	{
	 		 FileRow* m_pcFileRow = m_pcDirView->GetFile(0);
	 	 	if (m_pcFileRow->GetName()  == ".."){
	 	 		m_pcDirView->Invoked(0,0);
	 	 		m_pcDirView->Clear();
	 	 		m_pcDirView->ReRead();
	 	 		if (m_pcTypeDrop->GetItemCount() >=5)
	 	 		{
	 	 			for (uint i=5; i<=m_pcTypeDrop->GetItemCount(); i++)
	 	 				m_pcTypeDrop->DeleteItem(i);
	 	 		}
	 	 		
	 	 		std::string cDropString = m_pcDirView->GetPath() + (std::string)"/";
	 	 	
	 	 		if (cDropString != "//"){
    				m_pcTypeDrop->InsertItem(5,cDropString.c_str());
    				m_pcTypeDrop->SetSelection(5);
    		}
    	
    			else{
    				m_pcTypeDrop->InsertItem(5,"/");
    				m_pcTypeDrop->SetSelection(5);
			}
	 	 	}
	 	 	break;
	 	}
	 
	case ID_SEL_CHANGED:
	{
	    int nSel = m_pcDirView->GetFirstSelected();
	    if ( nSel >= 0 ) {
		FileRow* pcFile = m_pcDirView->GetFile( nSel );
		if ( (m_nNodeType & NODE_DIR) || S_ISDIR( pcFile->GetFileStat().st_mode ) == false ) {
		    m_pcPathView->Set( pcFile->GetName().c_str() );
		    
		}
	    }
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
		    std::string cMsg = "The file '" +  (std::string)cPath.GetPath() + "' already exists\nDo you want to overwrite it?\n";
	  
		    Alert* pcAlert = new Alert( "Warning:", cMsg.c_str(), Alert::ALERT_WARNING,0, "No", "Yes", NULL );
		    pcAlert->CenterInWindow(this);
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
		{
			Message* pcMsg = new Message(M_FILE_REQUESTER_CANCELED);
			m_pcTarget->SendMessage(pcMsg);
	    	Show( false );
	    }
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
















