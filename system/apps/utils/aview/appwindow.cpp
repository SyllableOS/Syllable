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
#include <gui/checkmenu.h>
#include <gui/guidefines.h>

#include <iostream>

#include "appwindow.h"
#include "iview.h"
#include "messages.h"

AViewWindow::AViewWindow( const Rect &cFrame, bool bFitToImage, char **ppzArgList, int nArgCount ) : Window( cFrame, "aview_ng", AV_TITLE )
{
	m_pcImage = NULL;
	m_bFitToImage = bFitToImage;

	Rect cMenuFrame, cIViewFrame, cStatusBarFrame;
	cMenuFrame = cIViewFrame = cStatusBarFrame = GetBounds();
	cMenuFrame.bottom = 18;

	// Menus
	m_pcMenuBar = new Menu( cMenuFrame, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	m_pcApplicationMenu = new Menu( Rect( 0, 0, 1, 1 ), "Application", ITEMS_IN_COLUMN );
	m_pcApplicationMenu->AddItem( "Quit", new Message( ID_MENU_APP_QUIT ), "CTRL+Q" );
	m_pcApplicationMenu->AddItem( new MenuSeparator() );
	m_pcApplicationMenu->AddItem( "About", new Message( ID_MENU_APP_ABOUT ) );
	m_pcMenuBar->AddItem( m_pcApplicationMenu );

	m_pcFileMenu = new Menu( Rect( 0, 0, 1, 1 ), "File", ITEMS_IN_COLUMN );
	m_pcFileMenu->AddItem( "Open", new Message( ID_MENU_FILE_OPEN ), "CTRL+O" );
	m_pcMenuBar->AddItem( m_pcFileMenu );

	m_pcViewMenu = new Menu( Rect( 0, 0, 1, 1 ), "View", ITEMS_IN_COLUMN );
	m_pcViewMenu->AddItem( new CheckMenu( "Fit to image" , new Message( ID_MENU_VIEW_FITTO ), m_bFitToImage ) );
	m_pcViewMenu->AddItem( new MenuSeparator() );

	// The navigation menu items are created specially as we need to be able to access
	// the objects after they are created here
	m_pcViewPrevItem = new MenuItem( "Previous", new Message( ID_MENU_VIEW_PREV ) );
	m_pcViewMenu->AddItem( m_pcViewPrevItem );
	m_pcViewNextItem = new MenuItem( "Next", new Message( ID_MENU_VIEW_NEXT ) );
	m_pcViewMenu->AddItem( m_pcViewNextItem );
	m_pcViewMenu->AddItem( new MenuSeparator() );
	m_pcViewFirstItem = new MenuItem( "First", new Message( ID_MENU_VIEW_FIRST ) );
	m_pcViewMenu->AddItem( m_pcViewFirstItem );
	m_pcViewLastItem = new MenuItem( "Last", new Message( ID_MENU_VIEW_LAST ) );
	m_pcViewMenu->AddItem( m_pcViewLastItem );

	m_pcMenuBar->AddItem( m_pcViewMenu );

	m_vMenuHeight = m_pcMenuBar->GetPreferredSize( true ).y - 1.0f;
	cMenuFrame.bottom = m_vMenuHeight;
	m_pcMenuBar->SetFrame( cMenuFrame );
	m_pcMenuBar->SetTargetForItems( this );
	AddChild( m_pcMenuBar );

	// Toolbar and Toolbar buttons
	m_pcToolBar = new ToolBar( Rect( 0, m_vMenuHeight + 1, GetBounds().Width(), AW_TOOLBAR_HEIGHT + m_vMenuHeight ), "aview_ng_toolbar" );

	HLayoutNode* pcNode = new HLayoutNode( "aview_ng_h_root" );
	pcNode->SetBorders( Rect( 0, 4, 5, 4 ) );

	m_pcOpenButton = new ImageButton( Rect(), "open", "", new Message( ID_EVENT_OPEN ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( m_pcOpenButton, "fileopen.png" );
	pcNode->AddChild( m_pcOpenButton, 0.0f );

	m_pcBreakerButton = new ImageButton( Rect(), "breaker", "", NULL,NULL, ImageButton::IB_TEXT_BOTTOM, false, false, false );
	SetButtonImageFromResource( m_pcBreakerButton, "breaker.png" );
	pcNode->AddChild( m_pcBreakerButton, 0.0f );

	m_pcFirstButton = new ImageButton( Rect(), "first", "", new Message( ID_EVENT_FIRST ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( m_pcFirstButton, "first.png" );
	pcNode->AddChild( m_pcFirstButton, 0.0f );

	m_pcPrevButton = new ImageButton( Rect(), "previous", "", new Message( ID_EVENT_PREV ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( m_pcPrevButton, "previous.png" );
	pcNode->AddChild( m_pcPrevButton, 0.0f );

	m_pcNextButton = new ImageButton( Rect(), "next", "", new Message( ID_EVENT_NEXT ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( m_pcNextButton, "next.png" );
	pcNode->AddChild( m_pcNextButton, 0.0f );

	m_pcLastButton = new ImageButton( Rect(), "last", "", new Message( ID_EVENT_LAST ), NULL, ImageButton::IB_TEXT_BOTTOM, true, false, true );
	SetButtonImageFromResource( m_pcLastButton, "last.png" );
	pcNode->AddChild( m_pcLastButton, 0.0f );

	pcNode->AddChild( new HLayoutSpacer( "aview_ng_h_spacer" ) );

	m_pcToolBar->SetRoot( pcNode );
	AddChild( m_pcToolBar );

	// Image View
	cIViewFrame.top = m_vMenuHeight + AW_TOOLBAR_HEIGHT + 1;
	cIViewFrame.bottom -= AW_STATUSBAR_HEIGHT + 1;
	m_pcIView = new IView( cIViewFrame, "aview_ng_iview", this );
	m_pcIView->Hide();
	AddChild( m_pcIView );

	SetFocusChild( m_pcIView );
	SetDefaultWheelView( m_pcIView );

	// Status bar
	cStatusBarFrame.top = cStatusBarFrame.Height() - AW_STATUSBAR_HEIGHT;
	cStatusBarFrame.right = cStatusBarFrame.Width();
	cStatusBarFrame.bottom = GetBounds().Height()+1;
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
	m_pcMinWindowSize = new Point( 250, 275 );
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
			m_pcAlertInvoker = new Invoker( new Message( ID_ALERT_OK ) );
			Alert* pcAbout = new Alert("About AView", AV_ABOUT, Alert::ALERT_INFO, 0x00, "OK", NULL);
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

		case ID_MENU_VIEW_FITTO:
		{
			pcMessage->FindBool( "status",&m_bFitToImage );
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

	cSettings.Save();

	// Instruct the application to quit
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
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

		if( ( cFrame.Width() > cImageSize.x ) ||
			( cFrame.Height() > cImageSize.y ) ||
			m_bFitToImage )
			ResizeTo( cImageSize );

		// Display the new Image
		m_pcIView->SetImage( m_pcImage );

		// Ensure the Window can not be made larger than the Image being displayed
		SetSizeLimits( *m_pcMinWindowSize, cImageSize );

		if( !m_pcIView->IsVisible() )
			m_pcIView->Show();

		sprintf( zStatusText, "%i of %i", m_nCurrentIndex + 1, m_vzFileList.size() );
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

