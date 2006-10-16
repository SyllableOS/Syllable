#include <util/application.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/fontrequester.h>
#include <gui/layoutview.h>
#include <gui/scrollbar.h>
#include <gui/tableview.h>
#include <util/shortcutkey.h>
#include <util/clipboard.h>
#include <util/catalog.h>
#include <util/locale.h>
#include <storage/file.h>
#include <util/resources.h>
#include "mainwin.h"
#include "charmap.h"
#include "resources/CharMap.h"

#include <iostream>

using namespace std;

using namespace os;

enum MessageIDs {
	ID_OPEN = 1,
	ID_SAVE,
	ID_EXIT,
	ID_SCROLL,
	ID_MOUSEOVER,
	ID_UPDATESCROLLBAR,
	ID_KEY,
	ID_HELP,
	ID_ABOUT,
	ID_FONT,
	ID_MENU_FONT_SIZE,
	ID_MENU_FONT_FAMILY_AND_STYLE,
};

MainWin::MainWin( const Rect& cRect )
 : Window( cRect, "MainWin", MSG_MAINWND_TITLE )
{
    LayoutNode* pcRootLayout = new VLayoutNode( "pcRootLayout" );
   
    Menu*       pcMenuBar = _CreateMenuBar();
	m_pcStatusBar = new StatusBar( Rect(), "pcStringView" );
	
	m_pcStatusBar->AddPanel( "char", "", 3, StatusPanel::F_FIXED );
	m_pcStatusBar->AddPanel( "unicode", "", 100 );
	m_pcStatusBar->AddPanel( "utf-8", "", 100 );

    LayoutNode*	pcCharmapLayout = new HLayoutNode( "pcCharmapLayout" );

	m_pcScrollBar = new ScrollBar( Rect(), "pcScrollBar", new Message( ID_SCROLL ), 0.0f, 100.0f );
	m_pcCharMap = new CharMapView( Rect(), "pcCharmap", new Message( ID_KEY ) );

	m_pcCharMap->SetMouseOverMessage( new Message( ID_MOUSEOVER ) );
	m_pcCharMap->SetScrollBarChangeMessage( new Message( ID_UPDATESCROLLBAR ) );
	m_pcCharMap->SetTarget( this );

    pcCharmapLayout->AddChild( m_pcCharMap );
	pcCharmapLayout->AddChild( m_pcScrollBar );

    pcRootLayout->AddChild( pcMenuBar, 0 );
    pcRootLayout->AddChild( pcCharmapLayout );
    pcRootLayout->AddChild( m_pcStatusBar, 0 );
   
    LayoutView* pcLayout = new LayoutView( GetBounds(), "pcLayout" );
    pcLayout->SetRoot( pcRootLayout );

	Font* pcFont = new Font( DEFAULT_FONT_REGULAR );
	font_properties sProps;
	pcFont->GetDefaultFont( DEFAULT_FONT_REGULAR, &sProps );
	pcFont->SetSize( 16.0f );
	m_pcCharMap->SetFont( pcFont );
	pcFont->Release();
    m_vSize = 16.0f;
	m_cFamily = sProps.m_cFamily;
	m_cStyle = sProps.m_cStyle;
	m_nFlags = sProps.m_nFlags;

    AddChild( pcLayout );
    
    SetDefaultWheelView( m_pcScrollBar );
    
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

Menu* MainWin::_CreateMenuBar()
{
	Menu* pcMenuBar = new Menu(Rect(), "pcMenuBar", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

	// Create the menus within the bar
	Menu* pcFileMenu = new Menu(Rect(),MSG_MAINWND_MENU_FILE, ITEMS_IN_COLUMN);
	pcFileMenu->AddItem(MSG_MAINWND_MENU_FILE_CHOOSEFONT, new Message( ID_FONT ) );
	pcFileMenu->AddItem(MSG_MAINWND_MENU_FILE_EXIT, new Message( ID_EXIT ), "CTRL+Q");
	
	pcMenuBar->AddItem(pcFileMenu);

	Menu* pcHelpMenu = new Menu(Rect(),MSG_MAINWND_MENU_HELP, ITEMS_IN_COLUMN);
	pcHelpMenu->AddItem(MSG_MAINWND_MENU_HELP_HELP, new Message( ID_HELP ), "CTRL+?");
	pcHelpMenu->AddItem(MSG_MAINWND_MENU_HELP_ABOUT, new Message( ID_ABOUT ) );

	pcMenuBar->AddItem(pcHelpMenu);

	// Tell the menu to send it's messages to our window
	pcMenuBar->SetTargetForItems(this);

	return pcMenuBar;
}

MainWin::~MainWin()
{
}
	
bool MainWin::OkToQuit()
{
    Application::GetInstance()->PostMessage( M_QUIT );
    return true;
}

void MainWin::HandleMessage( Message *pcMsg )
{
	switch( pcMsg->GetCode() ) {
		case ID_EXIT:
			OkToQuit();
			break;
		case ID_SCROLL:
			{
				Variant cVal;
				pcMsg->FindVariant( "value", &cVal );
				int nLine = cVal.AsInt32();
				m_pcScrollBar->SetValue( (float)nLine );
				m_pcCharMap->SetLine( nLine );
			}
			break;
		case ID_UPDATESCROLLBAR:
			{
				int32	nMax;
				int32	nCur;
				int32	nHeight;
							
				if( pcMsg->FindInt32( "total", &nMax ) != -1 &&
					pcMsg->FindInt32( "height", &nHeight ) != -1 &&
					pcMsg->FindInt32( "current", &nCur ) != -1 ) {
						m_pcScrollBar->SetMinMax( 0, nMax - nHeight + 1.0f );
						m_pcScrollBar->SetValue( (float)nCur );
						m_pcScrollBar->SetSteps( 1.0f, nHeight - 1.0f );
						if( nMax ) {
							m_pcScrollBar->SetProportion( (float)nHeight / (float)nMax );
						}
				}
			}
			break;
		case ID_MOUSEOVER:
			{
				int32	nUnicode;
				String	cUTF8;
				
				if( pcMsg->FindInt32( "unicode", &nUnicode ) != -1 &&
					pcMsg->FindString( "utf-8", &cUTF8 ) != -1 ) {
						String s;
						String numeric, utf8;
						
						if( nUnicode < 128 ) {
							numeric = numeric.Format( "ASCII: %d (0x%02x) ", nUnicode, nUnicode );
						} else {
							numeric = numeric.Format( "Unicode: %d (0x%04x) ", nUnicode, nUnicode );
						}
						
						m_pcStatusBar->SetText( "unicode", numeric );
						
						for( int i = 0; cUTF8.c_str()[i] != 0; i++ ) {
							utf8 += s.Format( "%02x ", int(cUTF8.c_str()[i]) & 0xFF);
						}
						
						m_pcStatusBar->SetText( "char", String("\33c") + cUTF8 );
						m_pcStatusBar->SetText( "utf-8", String("UTF-8: ") + utf8 );
				} else {
					m_pcStatusBar->SetText( "char", "" );
					m_pcStatusBar->SetText( "utf-8", "" );
					m_pcStatusBar->SetText( "unicode", "" );
				}
			}
			break;
		case ID_KEY:
			{
				Clipboard cClipboard;
				String cStr;
				
				if( pcMsg->FindString( "utf-8", &cStr ) != -1 ) {
					cClipboard.Lock();
					cClipboard.Clear();
					Message *pcData = cClipboard.GetData();
	
					pcData->AddString( "text/plain", cStr );
					cClipboard.Commit();
					cClipboard.Unlock();
				}
			}
			break;
		case ID_ABOUT:
			{
				Alert *pcAlert = new Alert( MSG_ABOUTWND_TITLE,
                                  MSG_ABOUTWND_TEXT,
                                  Alert::ALERT_INFO, 0,
                                  MSG_ABOUTWND_OK.c_str(), 0 );
    	    	pcAlert->CenterInWindow( this );
	    	    pcAlert->Go( new Invoker(NULL) );
        	}
			break;
		case ID_HELP:
			{
				Alert *pcAlert = new Alert( MSG_HELPWND_TITLE,
                                  MSG_HELPWND_TEXT +"\n"
                                  "",
                                  Alert::ALERT_INFO, 0,
                                  MSG_HELPWND_OK.c_str(), 0 );
    	    	pcAlert->CenterInWindow( this );
	    	    pcAlert->Go( new Invoker(NULL) );
        	}
			break;
		case ID_FONT:			
			{
				FontRequester* pcFontRequester = new FontRequester( new Messenger( this ), false );
				pcFontRequester->Show();
			}
			break;
		case M_FONT_REQUESTED:
			{
				os::Font* pcFont = new Font();
				pcMsg->FindObject("font",*pcFont);
				m_pcCharMap->SetFont(pcFont);
				pcFont->Release();
			}
			break;

		default:
			// If we don't recognize the message, we'll pass it on to the superclass
			Window::HandleMessage( pcMsg );
	}
}


