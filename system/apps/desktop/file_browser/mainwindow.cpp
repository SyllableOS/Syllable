/*  Syllable FileBrowser
 *  Copyright (C) 2003 Arno Klenke
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
 


#include <iostream>
#include "mainwindow.h"
#include "messages.h"

void SetButtonImageFromResource( os::ImageButton* pcButton, os::String zResource )
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zResource );
	pcButton->SetImage( pcStream );
	delete( pcStream );
}

MainWindow::MainWindow( os::String zPath ) : os::Window( os::Rect( 0, 0, 300, 300 ), "main_wnd", "FileBrowser" )
{
	
	SetTitle( zPath );
	CenterInScreen();
	
	/* Create toolbar */
	m_pcToolBar = new os::ToolBar( os::Rect( 0, 0, GetBounds().Width(), 35 ), "" );
	
	os::HLayoutNode* pcNode = new os::HLayoutNode( "h_root" );
	pcNode->SetBorders( os::Rect( 0, 4, 5, 4 ) );
	
	os::ImageButton* pcBreaker = new os::ImageButton(os::Rect(), "breaker", "",NULL,NULL, os::ImageButton::IB_TEXT_BOTTOM,false,false,false);
    SetButtonImageFromResource( pcBreaker, "breaker.png" );
    pcNode->AddChild( pcBreaker, 0.0f );
    
    m_pcBackButton = new os::ImageButton( os::Rect(), "back", "",new os::Message( M_BACK )
									,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    m_pcBackButton->SetEnable( false );
    SetButtonImageFromResource( m_pcBackButton, "back.png" );
	pcNode->AddChild( m_pcBackButton, 0.0f );
	os::ImageButton* pcUp = new os::ImageButton( os::Rect(), "up", "",new os::Message( M_UP )
									,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
    SetButtonImageFromResource( pcUp, "up.png" );
	pcNode->AddChild( pcUp, 0.0f );
	
	os::ImageButton* pcReload = new os::ImageButton( os::Rect(), "reload", "",new os::Message( M_RELOAD )
									,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( pcReload, "reload.png" );
	pcNode->AddChild( pcReload, 0.0f );
	
	os::ImageButton* pcHome = new os::ImageButton( os::Rect(), "home", "",new os::Message( M_HOME )
									,NULL, os::ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( pcHome, "home.png" );
	pcNode->AddChild( pcHome, 0.0f );
	pcNode->AddChild( new os::HLayoutSpacer( "" ) );
	
	
	/* Create popupmenu */
	os::Menu* pcPopupMenu = new os::Menu( os::Rect(), "", os::ITEMS_IN_COLUMN );
	pcPopupMenu->AddItem( "Icons", new os::Message( M_VIEW_ICONS ) );
	pcPopupMenu->AddItem( "List", new os::Message( M_VIEW_LIST ) );
	pcPopupMenu->AddItem( "Details", new os::Message( M_VIEW_DETAILS ) );
	pcPopupMenu->AddItem( new os::MenuSeparator() );
	pcPopupMenu->AddItem( "Save", new os::Message( M_SAVE_WINDOW ) );
	pcPopupMenu->SetTargetForItems( this );
	
	
	os::BitmapImage* pcView = new os::BitmapImage();
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( "view.png" );
	pcView->Load( pcStream );
	delete( pcStream );
	
	m_pcViewMenu = new os::PopupMenu( os::Rect( GetBounds().Width() - 70, 2,  GetBounds().Width() - 5, 32 ), "", "View", pcPopupMenu, pcView,
						os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_RIGHT );
	pcNode->AddChild( m_pcViewMenu );
	
	/* Create icon view */
	m_pcView = new os::IconDirectoryView( os::Rect( 0, 36, GetBounds().Width(), GetBounds().Height() ), zPath, os::CF_FOLLOW_ALL );
	m_pcView->SetDirChangeMsg( new os::Message( M_CHANGE_DIR ) );
	m_pcView->SetDirectoryLocked( true );
	os::BitmapImage* pcDirIcon = static_cast<os::BitmapImage*>(m_pcView->GetDirIcon());
	pcDirIcon->SetSize( os::Point( 24, 24 ) );
	SetIcon( pcDirIcon->LockBitmap() );
	delete( pcDirIcon );
	m_cBackStack.push( zPath );
	
	m_pcToolBar->SetRoot( pcNode );
	AddChild( m_pcToolBar );
	AddChild( m_pcView );
	m_pcView->MakeFocus();	
	
	LoadWindow( false );
	m_pcView->ReRead();
}

MainWindow::~MainWindow()
{
	m_pcView->Clear();
}


void MainWindow::LoadWindow( bool bStyleOnly )
{
	/* Load the window size and style from the attributes */
	char zWindowString[255];
	os::String cCurrentPath = m_pcView->GetPath();
	memset( zWindowString, 0, 255 );
	
	try
	{
		os::FSNode cNode( cCurrentPath );
		if( !bStyleOnly )
		{
			if( cNode.ReadAttr( "os::WindowPosition", ATTR_TYPE_STRING, 
					zWindowString, 0, 255 ) > 0 )
			{
				float vX = 0;
				float vY = 0;
				float vX2 = 0;
				float vY2 = 0;
			
				sscanf( zWindowString, "%f %f %f %f", &vX, &vY, &vX2, &vY2 );
				//printf( "%s %f %f %f %f\n", zWindowString, vX, vY, vX2, vY2 );
			
				if( vX < 0 ) vX = 0;
				if( vY < 0 ) vY = 0;
				if( vX2 < vX ) vX2 = vX + 100;
				if( vY2 < vY ) vY2 = vY + 100;
			
				SetFrame( os::Rect( vX, vY, vX2, vY2 ) );
			}
		}
		int32 nStyle;
		if( cNode.ReadAttr( "os::WindowStyle", ATTR_TYPE_INT32, 
					&nStyle, 0, sizeof( int32 ) ) == sizeof( int32 ) )
		{
			if( nStyle < 0 || nStyle > 2 )
				nStyle = 0;
			m_pcView->SetView( (os::IconView::view_type)nStyle );
		}
	} catch( ... )
	{
	}
}

void MainWindow::SaveWindow()
{
	/* Save the window size and style as an attribute */
	char zWindowString[255];
	os::String cCurrentPath = m_pcView->GetPath();
	
	sprintf( zWindowString, "%f %f %f %f", GetFrame().left, GetFrame().top, 
			GetFrame().right, GetFrame().bottom );
	try
	{
		os::FSNode cNode( cCurrentPath );
		cNode.WriteAttr( "os::WindowPosition", O_TRUNC, ATTR_TYPE_STRING, 
				zWindowString, 0, strlen( zWindowString ) );
		int32 nStyle = m_pcView->GetView();
		cNode.WriteAttr( "os::WindowStyle", O_TRUNC, ATTR_TYPE_INT32, 
				&nStyle, 0, sizeof( int32 ) );
	} catch( ... )
	{
	}
}

void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_CHANGE_DIR:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath.str() ) != 0 )
				break;
			
				
			/* We get two M_CHANGE_DIR messages. First when the new directory is opened and
			then because of our SetPath() call. We ignore the second case here */
			if( zPath == m_pcView->GetPath() )
				break;
			
			m_pcView->SetPath( zPath );
			m_pcView->Clear();
			LoadWindow( true );
			m_pcView->ReRead();
			
			/* Get the new directory icon */
			os::BitmapImage* pcImage = static_cast<os::BitmapImage*>(m_pcView->GetDirIcon());
			if( pcImage )
			{
				pcImage->SetSize( os::Point( 24, 24 ) );
				SetIcon( pcImage->LockBitmap() );
				delete( pcImage );
			}
			SetTitle( m_pcView->GetPath() );
			m_cBackStack.push( m_pcView->GetPath() );
			if( m_cBackStack.size() > 1 )
				m_pcBackButton->SetEnable( true );
			else
				m_pcBackButton->SetEnable( false );
			
			break;
		}
		case M_BACK:
		{
			if( m_cBackStack.size() > 1 )
			{
				m_cBackStack.pop();
				os::String cNewPath = m_cBackStack.top();
				m_cBackStack.pop();
				os::Message* pcMsg = new os::Message( M_CHANGE_DIR );
				pcMsg->AddString( "file/path", cNewPath );
				PostMessage( pcMsg, this );
			}
			break;
		}
		case M_UP:
		{
			os::Message* pcMsg = new os::Message( M_CHANGE_DIR );
			os::Path cPath( m_pcView->GetPath().c_str() );
			pcMsg->AddString( "file/path", cPath.GetDir().GetPath() );
			PostMessage( pcMsg, this );
			break;
		}
		case M_HOME:
		{
			const char *pzHome = getenv( "HOME" );
			os::Message* pcMsg = new os::Message( M_CHANGE_DIR );
			pcMsg->AddString( "file/path", pzHome );
			PostMessage( pcMsg, this );
			break;
		}
		case M_RELOAD:
		{
			m_pcView->ReRead();
			break;
		}
		case M_VIEW_ICONS:
		{
			m_pcView->SetView( os::IconDirectoryView::VIEW_ICONS );
			m_pcView->ReRead();
			break;
		}
		case M_VIEW_LIST:
		{
			m_pcView->SetView( os::IconDirectoryView::VIEW_LIST );
			m_pcView->ReRead();
			break;
		}
		case M_VIEW_DETAILS:
		{
			m_pcView->SetView( os::IconDirectoryView::VIEW_DETAILS );
			m_pcView->ReRead();
			break;
		}
		case M_SAVE_WINDOW:
		{
			SaveWindow();
			break;
		}
		case M_APP_QUIT:
		{
			OkToQuit();
			break;
		}
		
		break;
	}
}
bool MainWindow::OkToQuit()
{
	if( m_pcView->JobsPending() )
		return( false );
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}













