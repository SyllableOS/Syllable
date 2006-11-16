//  AView (C)opyright 2005 Kristian Van Der Vliet
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

#include <posix/errno.h>
#include <util/application.h>
#include <util/settings.h>
#include <gui/scrollbar.h>
#include <gui/filerequester.h>
#include <gui/requesters.h>
#include <gui/guidefines.h>

#include <iostream>

#include "appwindow.h"
#include "iview.h"
#include "messages.h"

#include "resources/AView.h"

AViewWindow::AViewWindow( const Rect &cFrame, bool bFitToImage, bool bFitToWindow, char **ppzArgList, int nArgCount ) : Window( cFrame, "aview_ng", MSG_MAINWND_TITLE )
{
	m_pcImage = NULL;
	m_bFitToImage = bFitToImage;
	m_bFitToWindow = bFitToWindow;
	m_nScaleBy = 100;

	Rect cMenuFrame, cIViewFrame, cStatusBarFrame;
	cMenuFrame = cIViewFrame = cStatusBarFrame = GetBounds();
	cMenuFrame.bottom = 18;

	// Menus
	m_pcMenuBar = new Menu( cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcApplicationMenu = new Menu( Rect( 0, 0, 1, 1 ), MSG_MAINWND_MENU_APPLICATION, ITEMS_IN_COLUMN );
	m_pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new Message( ID_MENU_APP_QUIT ), "CTRL+Q" );
	m_pcApplicationMenu->AddItem( new MenuSeparator() );
	m_pcApplicationMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new Message( ID_MENU_APP_ABOUT ) );
	m_pcMenuBar->AddItem( m_pcApplicationMenu );

	m_pcFileMenu = new Menu( Rect( 0, 0, 1, 1 ), MSG_MAINWND_MENU_FILE, ITEMS_IN_COLUMN );
	m_pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_OPEN, new Message( ID_MENU_FILE_OPEN ), "CTRL+O" );
	m_pcMenuBar->AddItem( m_pcFileMenu );

	m_pcViewMenu = new Menu( Rect( 0, 0, 1, 1 ), MSG_MAINWND_MENU_VIEW, ITEMS_IN_COLUMN );

	m_pcViewFitImageItem = new CheckMenu( MSG_MAINWND_MENU_VIEW_FITTOIMAGE, new Message( ID_MENU_VIEW_FITTOIMG ), m_bFitToImage );
	m_pcViewMenu->AddItem( m_pcViewFitImageItem );
	m_pcViewFitWindowItem = new CheckMenu( MSG_MAINWND_MENU_VIEW_FITTOWINDOW, new Message( ID_MENU_VIEW_FITTOWND ), m_bFitToWindow );
	m_pcViewMenu->AddItem( m_pcViewFitWindowItem );

	m_pcViewMenu->AddItem( new MenuSeparator() );

	// The navigation menu items are created specially as we need to be able to access
	// the objects after they are created here
	m_pcViewPrevItem = new MenuItem( MSG_MAINWND_MENU_VIEW_PREVIOUS, new Message( ID_MENU_VIEW_PREV ) );
	m_pcViewMenu->AddItem( m_pcViewPrevItem );
	m_pcViewNextItem = new MenuItem( MSG_MAINWND_MENU_VIEW_NEXT, new Message( ID_MENU_VIEW_NEXT ) );
	m_pcViewMenu->AddItem( m_pcViewNextItem );
	m_pcViewMenu->AddItem( new MenuSeparator() );
	m_pcViewFirstItem = new MenuItem( MSG_MAINWND_MENU_VIEW_FIRST, new Message( ID_MENU_VIEW_FIRST ) );
	m_pcViewMenu->AddItem( m_pcViewFirstItem );
	m_pcViewLastItem = new MenuItem( MSG_MAINWND_MENU_VIEW_LAST, new Message( ID_MENU_VIEW_LAST ) );
	m_pcViewMenu->AddItem( m_pcViewLastItem );

	m_pcMenuBar->AddItem( m_pcViewMenu );

	m_pcResizeMenu = new Menu( Rect( 0, 0, 1, 1 ), MSG_MAINWND_MENU_RESIZE, ITEMS_IN_COLUMN );

	m_pcResize10pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_10, new Message( ID_MENU_RESIZE_10 ), false );
	m_pcResizeMenu->AddItem( m_pcResize10pcItem );
	m_pcResize25pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_25, new Message( ID_MENU_RESIZE_25 ), false );
	m_pcResizeMenu->AddItem( m_pcResize25pcItem );
	m_pcResize50pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_50, new Message( ID_MENU_RESIZE_50 ), false );
	m_pcResizeMenu->AddItem( m_pcResize50pcItem );
	m_pcResize75pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_75, new Message( ID_MENU_RESIZE_75 ), false );
	m_pcResizeMenu->AddItem( m_pcResize75pcItem );
	m_pcResize100pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_100, new Message( ID_MENU_RESIZE_100 ), true );
	m_pcResizeMenu->AddItem( m_pcResize100pcItem );
	m_pcResize150pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_150, new Message( ID_MENU_RESIZE_150 ), false );
	m_pcResizeMenu->AddItem( m_pcResize150pcItem );
	m_pcResize200pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_200, new Message( ID_MENU_RESIZE_200 ), false );
	m_pcResizeMenu->AddItem( m_pcResize200pcItem );
	m_pcResize400pcItem = new CheckMenu( MSG_MAINWND_MENU_RESIZE_400, new Message( ID_MENU_RESIZE_400 ), false );
	m_pcResizeMenu->AddItem( m_pcResize400pcItem );

	m_pcMenuBar->AddItem( m_pcResizeMenu );

	m_vMenuHeight = m_pcMenuBar->GetPreferredSize( true ).y - 1.0f;
	cMenuFrame.bottom = m_vMenuHeight;
	m_pcMenuBar->SetFrame( cMenuFrame );
	m_pcMenuBar->SetTargetForItems( this );
	AddChild( m_pcMenuBar );

	// Toolbar and Toolbar buttons
	m_pcToolBar = new ToolBar( Rect( 0, m_vMenuHeight + 1, GetBounds().Width(), AW_TOOLBAR_HEIGHT + m_vMenuHeight ), "aview_ng_toolbar" );

	m_pcOpenButton = new ImageButton( Rect(), "open", MSG_MAINWND_MENU_FILE_OPEN, new Message( ID_EVENT_OPEN ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	SetButtonImageFromResource( m_pcOpenButton, "fileopen.png" );
	m_pcToolBar->AddChild( m_pcOpenButton, ToolBar::TB_FIXED_WIDTH );

	m_pcToolBar->AddSeparator( "aview_ng_separator" );

	m_pcFirstButton = new ImageButton( Rect(), "first", MSG_MAINWND_MENU_VIEW_FIRST, new Message( ID_EVENT_FIRST ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	SetButtonImageFromResource( m_pcFirstButton, "first.png" );
	m_pcToolBar->AddChild( m_pcFirstButton, ToolBar::TB_FIXED_WIDTH );

	m_pcPrevButton = new ImageButton( Rect(), "previous", MSG_MAINWND_MENU_VIEW_PREVIOUS, new Message( ID_EVENT_PREV ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	SetButtonImageFromResource( m_pcPrevButton, "previous.png" );
	m_pcToolBar->AddChild( m_pcPrevButton, ToolBar::TB_FIXED_WIDTH );

	m_pcNextButton = new ImageButton( Rect(), "next", MSG_MAINWND_MENU_VIEW_NEXT, new Message( ID_EVENT_NEXT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	SetButtonImageFromResource( m_pcNextButton, "next.png" );
	m_pcToolBar->AddChild( m_pcNextButton, ToolBar::TB_FIXED_WIDTH );

	m_pcLastButton = new ImageButton( Rect(), "last", MSG_MAINWND_MENU_VIEW_LAST, new Message( ID_EVENT_LAST ), NULL, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	SetButtonImageFromResource( m_pcLastButton, "last.png" );
	m_pcToolBar->AddChild( m_pcLastButton, ToolBar::TB_FIXED_WIDTH );

	AddChild( m_pcToolBar );

	// Image View
	cIViewFrame.top = m_vMenuHeight + AW_TOOLBAR_HEIGHT;
	cIViewFrame.bottom -= AW_STATUSBAR_HEIGHT;

	// Shift the IView down 5 pixels
	cIViewFrame.top += 5;
	cIViewFrame.bottom += 5;

	m_pcIView = new IView( cIViewFrame, "aview_ng_iview", this );
	m_pcIView->SetMode( m_bFitToImage, m_bFitToWindow );
	m_pcIView->Hide();
	AddChild( m_pcIView );

	SetFocusChild( m_pcIView );
	SetDefaultWheelView( m_pcIView );

	// Status bar
	cStatusBarFrame.top = cIViewFrame.bottom + 1;
	cStatusBarFrame.right = cStatusBarFrame.Width();
	cStatusBarFrame.bottom = GetBounds().Height() + 1;
	cStatusBarFrame.left = 0;

	m_pcStatusBar = new StatusBar( cStatusBarFrame, "status_bar", 2 );
	m_pcStatusBar->ConfigurePanel( 0, StatusBar::CONSTANT, 50 );
	m_pcStatusBar->ConfigurePanel( 1, StatusBar::FILL, 0 );
	AddChild( m_pcStatusBar );

	// Open file requester
	m_pcLoadRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "" );

	// Set a Dock icon
	BitmapImage *pcApplicationIcon = LoadImageFromResource( "icon24x24.png" );
	SetIcon( pcApplicationIcon->LockBitmap() );

	// Ensure the window isn't resized out of sight
	m_pcMinWindowSize = new Point( 300, 275 );
	SetSizeLimits( *m_pcMinWindowSize, Point( cFrame.Width(), cFrame.Height() ) );

	// The first item of the argument list passed to us will always be the executable name,
	// so we skip the first item
	for( int nCurrent = 1; nCurrent < nArgCount; nCurrent++ )
		m_vzFileList.push_back( std::string( ppzArgList[ nCurrent ] ) );
	m_nCurrentIndex = 0;
}

AViewWindow::~AViewWindow()
{
	RemoveChild( m_pcMenuBar );
	delete( m_pcMenuBar );

	RemoveChild( m_pcToolBar );
	delete( m_pcToolBar );

	RemoveChild( m_pcIView );

	if( m_pcImage != NULL )
		delete( m_pcImage );

	delete( m_pcIView );

	RemoveChild( m_pcStatusBar );
	delete( m_pcStatusBar );

	delete( m_pcMinWindowSize );
}

void AViewWindow::HandleMessage( Message *pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case ID_MENU_APP_QUIT:
		{
			OkToQuit();
			break;
		}

		case ID_MENU_APP_ABOUT:
		{
			BitmapImage *pcAlertIcon = LoadImageFromResource( "icon48x48.png" );

			m_pcAlertInvoker = new Invoker( new Message( ID_ALERT_OK ) );
			Alert* pcAbout = new Alert( MSG_ABOUTWND_TITLE, MSG_ABOUTWND_TEXT, pcAlertIcon->LockBitmap(), 0x00, MSG_ABOUTWND_OK.c_str(), NULL);
			pcAbout->CenterInWindow( this );
			pcAbout->Go( m_pcAlertInvoker );
			break;
		}

		case ID_ALERT_OK:
		{
			delete( m_pcAlertInvoker );
			break;
		}

		case ID_MENU_FILE_OPEN:
		case ID_EVENT_OPEN:
		{
			m_pcLoadRequester->Show();
			break;
		}

		case M_LOAD_REQUESTED:
		{
			const char* pzFilename;
			int nIndex = 0;

			m_vzFileList.clear();

			while( pcMessage->FindString( "file/path", &pzFilename, nIndex++ ) == 0 )
				m_vzFileList.push_back( std::string( pzFilename ) );

			if( m_vzFileList.size() > 0 )
			{
				Load( 0 );
				DisableNav( NAV_PREV );
			}
			else
				HideIView();

			if( m_vzFileList.size() > 1 )
				EnableNav( NAV_NEXT );
			else
				DisableNav( NAV_NEXT );
			
			break;
		}

		case ID_MENU_VIEW_FITTOIMG:
		{
			pcMessage->FindBool( "status",&m_bFitToImage );
			if( m_bFitToImage )
			{
				m_bFitToWindow = false;
				m_pcViewFitWindowItem->SetChecked( false );
			}
			m_pcIView->SetMode( m_bFitToImage, m_bFitToWindow );
			ResetWindow();
			break;
		}

		case ID_MENU_VIEW_FITTOWND:
		{
			pcMessage->FindBool( "status",&m_bFitToWindow );
			if( m_bFitToWindow )
			{
				m_bFitToImage = false;
				m_pcViewFitImageItem->SetChecked( false );
			}
			m_pcIView->SetMode( m_bFitToImage, m_bFitToWindow );
			ResetWindow();
			break;
		}

		case ID_MENU_RESIZE_10:
		{
			DoResize( 10 );
			break;
		}

		case ID_MENU_RESIZE_25:
		{
			DoResize( 25 );
			break;
		}

		case ID_MENU_RESIZE_50:
		{
			DoResize( 50 );
			break;
		}

		case ID_MENU_RESIZE_75:
		{
			DoResize( 75 );
			break;
		}

		case ID_MENU_RESIZE_100:
		{
			DoResize( 100 );
			break;
		}

		case ID_MENU_RESIZE_150:
		{
			DoResize( 150 );
			break;
		}

		case ID_MENU_RESIZE_200:
		{
			DoResize( 200 );
			break;
		}

		case ID_MENU_RESIZE_400:
		{
			DoResize( 400 );
			break;
		}

		case ID_MENU_VIEW_PREV:
		case ID_EVENT_PREV:
		{
			// The while() loop will keep calling Load() until we get a valid file loaded
			// or run out of files to try
			while( ( m_nCurrentIndex > 0 ) && ( Load( --m_nCurrentIndex ) == EIO ) )
				;

			// Disable the previous navigation if we're at the start of the list
			if( 0 == m_nCurrentIndex )
				DisableNav( NAV_PREV );

			// Re-enable the next navigation if we're not at the end of the list
			if( m_nCurrentIndex < m_vzFileList.size() );
				EnableNav( NAV_NEXT );

			break;
		}

		case ID_MENU_VIEW_NEXT:
		case ID_EVENT_NEXT:
		{
			// The same loop as above, but this one counts up
			while( ( m_nCurrentIndex < m_vzFileList.size() - 1 ) && ( Load( ++m_nCurrentIndex ) == EIO ) )
				;

			// Disable the next navigation if we're at the end of the list
			if( m_nCurrentIndex == m_vzFileList.size() - 1 )
				DisableNav( NAV_NEXT );

			// Re-enable the previous navigation if we're not at the start of the list
			if( m_nCurrentIndex > 0 )
				EnableNav( NAV_PREV );

			break;
		}

		case ID_MENU_VIEW_FIRST:
		case ID_EVENT_FIRST:
		{
			if( m_vzFileList.size() > 0 )
			{
				Load( 0 );

				// This will always be the first in the list so
				// disable previous and enable next navigation
				DisableNav( NAV_PREV );
				EnableNav( NAV_NEXT );
			}
			break;
		}

		case ID_MENU_VIEW_LAST:
		case ID_EVENT_LAST:
		{
			if( m_vzFileList.size() > 0 )
			{
				Load( m_vzFileList.size() - 1 );

				// This will always be the last in the list so
				// disable next and enable previous navigation
				EnableNav( NAV_PREV );
				DisableNav( NAV_NEXT );
			}
			break;
		}

		default:
			Window::HandleMessage( pcMessage );
	}
}

bool AViewWindow::OkToQuit( void )
{
	// Save application settings
	Settings cSettings;

	cSettings.AddRect( "window_position", GetFrame() );
	cSettings.AddBool( "fit_to_image", m_bFitToImage );
	cSettings.AddBool( "fit_to_window", m_bFitToWindow );

	cSettings.Save();

	// Instruct the application to quit
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

void AViewWindow::FrameSized( const Point &cDelta )
{
	if( m_bFitToWindow )
	{
		Point cImageSize;

		cImageSize = m_pcImage->GetSize();
		cImageSize += cDelta;
		m_pcImage->SetSize( cImageSize );
	}
}

status_t AViewWindow::Load( uint nIndex )
{
	File *pcImageFile;
	status_t nError = EOK;

	if( nIndex > m_vzFileList.size() - 1 )
		return EINVAL;

	// Set m_nCurrentIndex wether we succeed or not, otherwise the navigation
	// will get into an inconsistant state
	m_nCurrentIndex = nIndex;
	m_sCurrentFilename = m_vzFileList[nIndex];

	try
	{
		pcImageFile = new File( m_vzFileList[nIndex] );
	}
	catch( std::exception &e )
	{
		std::cerr << m_vzFileList[nIndex] << ": " << e.what() << std::endl;
		HideIView();

		return EIO;
	}

	if( m_pcImage != NULL )
		delete( m_pcImage );
	
	m_pcImage = new BitmapImage( pcImageFile );

	if( m_pcImage )
	{
		char zStatusText[64];

		m_cOriginalSize = m_pcImage->GetSize();

		// Display the new Image
		m_pcIView->SetImage( m_pcImage );

		Scale();

		sprintf( zStatusText, MSG_STATUSBAR_OF.c_str(), m_nCurrentIndex + 1, m_vzFileList.size() );
		m_pcStatusBar->SetText( zStatusText, 0 );
		m_pcStatusBar->SetText( m_sCurrentFilename, 1 );
	}
	else
	{
		HideIView();
		nError = EINVAL;
	}

	delete( pcImageFile );

	return nError;
}

void AViewWindow::DisableNav( nav_id nId )
{
	if( NAV_PREV == nId )
	{
		m_pcFirstButton->SetEnable( false );
		m_pcPrevButton->SetEnable( false );

		m_pcViewFirstItem->SetEnable( false );
		m_pcViewPrevItem->SetEnable( false );
	}
	else
	{
		m_pcNextButton->SetEnable( false );
		m_pcLastButton->SetEnable( false );

		m_pcViewNextItem->SetEnable( false );
		m_pcViewLastItem->SetEnable( false );
	}
}

void AViewWindow::EnableNav( nav_id nId )
{
	if( NAV_PREV == nId )
	{
		m_pcFirstButton->SetEnable( true );
		m_pcPrevButton->SetEnable( true );

		m_pcViewFirstItem->SetEnable( true );
		m_pcViewPrevItem->SetEnable( true );
	}
	else
	{
		m_pcNextButton->SetEnable( true );
		m_pcLastButton->SetEnable( true );

		m_pcViewNextItem->SetEnable( true );
		m_pcViewLastItem->SetEnable( true );
	}
}

void AViewWindow::HideIView( void )
{
	if( m_pcIView->IsVisible() )
		m_pcIView->Hide();
	m_pcStatusBar->SetText( "", 0 );
	m_pcStatusBar->SetText( "", 1 );
}

void AViewWindow::ResetWindow( void )
{
	if( m_bFitToWindow )
	{
		Rect cIViewBounds = m_pcIView->GetBounds();
		Point cImageSize;

		cImageSize.x = cIViewBounds.Width() - IV_SCROLL_WIDTH;
		cImageSize.y = cIViewBounds.Height() - IV_SCROLL_HEIGHT;

		m_pcImage->SetSize( cImageSize );

		SetSizeLimits( *m_pcMinWindowSize,Point( 4096, 4096 ) );
	}
	else
	{
		// Ensure the Window isn't larger than the Image.  Always resize the
		// Window if the user has enabled Fit To Image.  Make sure the Window
		// is not resized any smaller than the minimum size limit.
		Rect cFrame = GetFrame();
		Point cImageSize = m_pcImage->GetSize();
		cImageSize.x += IV_SCROLL_WIDTH;
		cImageSize.y += m_vMenuHeight + AW_TOOLBAR_HEIGHT + IV_SCROLL_HEIGHT + AW_STATUSBAR_HEIGHT;

		if( cImageSize.x < m_pcMinWindowSize->x )
			cImageSize.x = m_pcMinWindowSize->x;
		if( cImageSize.y < m_pcMinWindowSize->y )
			cImageSize.y = m_pcMinWindowSize->y;

		if( ( cFrame.Width() > cImageSize.x ) || ( cFrame.Height() > cImageSize.y ) || m_bFitToImage )
			ResizeTo( cImageSize );

		// Ensure the Window can not be made larger than the Image being displayed
		SetSizeLimits( *m_pcMinWindowSize, cImageSize );
	}

	m_pcIView->Update( m_pcImage->GetSize() );

	if( !m_pcIView->IsVisible() )
		m_pcIView->Show();
}

void AViewWindow::DoResize( int nScaleBy )
{
	m_nScaleBy = nScaleBy;
	ResetResizeMenu();

	m_bFitToWindow = false;
	m_pcViewFitWindowItem->SetChecked( false );
	m_bFitToImage = true;
	m_pcViewFitImageItem->SetChecked( true );
	m_pcIView->SetMode( m_bFitToImage, m_bFitToWindow );

	Scale();
}

void AViewWindow::Scale( void )
{
	Point cCurrentSize, cNewSize;

	cCurrentSize = m_pcImage->GetSize();

	// Scale
	cNewSize.x = ( m_cOriginalSize.x / 100 ) * m_nScaleBy;
	cNewSize.y = ( m_cOriginalSize.y / 100 ) * m_nScaleBy;

	if( ( cNewSize.x != cCurrentSize.x ) || ( cNewSize.y != cCurrentSize.y ) )
		m_pcImage->SetSize( cNewSize );

	ResetWindow();
}

void AViewWindow::ResetResizeMenu( void )
{
	// This is horrible but I can't be bothered to clean it up
	m_pcResize10pcItem->SetChecked( false );
	m_pcResize25pcItem->SetChecked( false );
	m_pcResize50pcItem->SetChecked( false );
	m_pcResize75pcItem->SetChecked( false );
	m_pcResize100pcItem->SetChecked( false );
	m_pcResize150pcItem->SetChecked( false );
	m_pcResize200pcItem->SetChecked( false );
	m_pcResize400pcItem->SetChecked( false );

	switch( m_nScaleBy )
	{
		case 10:
		{
			m_pcResize10pcItem->SetChecked( true );
			break;
		}
		case 25:
		{
			m_pcResize25pcItem->SetChecked( true );
			break;
		}
		case 50:
		{
			m_pcResize50pcItem->SetChecked( true );
			break;
		}
		case 75:
		{
			m_pcResize75pcItem->SetChecked( true );
			break;
		}
		case 100:
		{
			m_pcResize100pcItem->SetChecked( true );
			break;
		}
		case 150:
		{
			m_pcResize150pcItem->SetChecked( true );
			break;
		}
		case 200:
		{
			m_pcResize200pcItem->SetChecked( true );
			break;
		}
		case 400:
		{
			m_pcResize400pcItem->SetChecked( true );
			break;
		}
	}
}

BitmapImage* AViewWindow::LoadImageFromResource( String zResource )
{
	BitmapImage *pcImage = new BitmapImage();
	Resources cRes( get_image_id() );		
	ResStream *pcStream = cRes.GetResourceStream( zResource );
	pcImage->Load( pcStream );
	delete ( pcStream );

	return pcImage;
}

void AViewWindow::SetButtonImageFromResource( ImageButton* pcButton, String zResource )
{
	pcButton->SetImage( LoadImageFromResource( zResource ) );
}

