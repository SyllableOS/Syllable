#include <util/application.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/menu.h>
#include <gui/requesters.h>
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
	ID_MENU_FONT_SIZE,
	ID_MENU_FONT_FAMILY_AND_STYLE,
};

MainWin::MainWin( const Rect& cRect )
 : Window( cRect, "MainWin", "Character map" )
{
    LayoutNode* pcRootLayout = new VLayoutNode( "pcRootLayout" );
   
    Menu*       pcMenuBar = _CreateMenuBar();
	m_pcStatusBar = new StringView( Rect(), "pcStringView", "Status: OK" );

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
	Menu* pcFileMenu = new Menu(Rect(),"File", ITEMS_IN_COLUMN);
	pcFileMenu->AddItem("Exit", new Message( ID_EXIT ), "CTRL+Q");
	
	pcMenuBar->AddItem(pcFileMenu);

	Menu* pcFontMenu = new Menu(Rect(),"Font", ITEMS_IN_COLUMN);

	// Add Size Submenu
	Menu* pcSizeMenu = new Menu(Rect(0,0,1,1), "Size", ITEMS_IN_COLUMN);

	float sz,del=1.0;

	Message *pcSizeMsg;
	char zSizeLabel[8];

	for (sz=5.0; sz < 30.0; sz += del)
	{
		pcSizeMsg = new Message(ID_MENU_FONT_SIZE);

		pcSizeMsg->AddFloat("size",sz);
		sprintf(zSizeLabel,"%.1f",sz);
		pcSizeMenu->AddItem(zSizeLabel,pcSizeMsg);

		if (sz == 10.0)
			del = 2.0;
		else if (sz == 16.0)
			del = 4.0;
   }

	pcFontMenu->AddItem(pcSizeMenu);
	pcFontMenu->AddItem(new MenuSeparator());

	// Add Family and Style Menu
	int fc = Font::GetFamilyCount();
	int sc,i=0,j=0;
	char zFFamily[FONT_FAMILY_LENGTH],zFStyle[FONT_STYLE_LENGTH];
	uint32 flags;

	Menu *pcFamilyMenu;
	Message *pcFontMsg;

	for (i=0; i<fc; i++)
	{
		if (Font::GetFamilyInfo(i,zFFamily) == 0)
		{
			sc = Font::GetStyleCount(zFFamily);
			pcFamilyMenu = new Menu(Rect(0,0,1,1),zFFamily,ITEMS_IN_COLUMN);

			for (j=0; j<sc; j++)
			{
				if (Font::GetStyleInfo(zFFamily,j,zFStyle,&flags) == 0)
				{
					pcFontMsg = new Message(ID_MENU_FONT_FAMILY_AND_STYLE);
					pcFontMsg->AddString("family",zFFamily);
					pcFontMsg->AddString("style",zFStyle);
					pcFontMsg->AddInt32("flags", flags );

					pcFamilyMenu->AddItem(zFStyle,pcFontMsg);
				}
			}

		pcFontMenu->AddItem(pcFamilyMenu);

		}
	}
	
	pcMenuBar->AddItem(pcFontMenu);

	Menu* pcHelpMenu = new Menu(Rect(),"Help", ITEMS_IN_COLUMN);
	pcHelpMenu->AddItem("Help", new Message( ID_HELP ), "CTRL+?");
	pcHelpMenu->AddItem("About", new Message( ID_ABOUT ) );

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
						
						for( int i = 0; cUTF8.c_str()[i] != 0; i++ ) {
							utf8 += s.Format( "%02x ", int(cUTF8.c_str()[i]) & 0xFF);
						}
						
						s = String( " '" ) + cUTF8 + String( "'  " ) + numeric + String( " UTF-8: " ) + utf8;
						m_pcStatusBar->SetString( s );
				} else {
					m_pcStatusBar->SetString( "" );
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
				Alert *pcAlert = new Alert( "CharMap",
                                  "CharMap V1.0\nCharacter map viewer for Syllable.",
                                  Alert::ALERT_INFO, 0,
                                  "Ok", 0 );
    	    	pcAlert->CenterInWindow( this );
	    	    pcAlert->Go( new Invoker(NULL) );
        	}
			break;
		case ID_HELP:
			{
				Alert *pcAlert = new Alert( "CharMap",
                                  "When you click on a character, it will be copied to the clipboard.\n"
                                  "",
                                  Alert::ALERT_INFO, 0,
                                  "Ok", 0 );
    	    	pcAlert->CenterInWindow( this );
	    	    pcAlert->Go( new Invoker(NULL) );
        	}
			break;
		case ID_MENU_FONT_SIZE:
			{
				float size;
	
				if (pcMsg->FindFloat("size",&size) == 0)
				{
					Font *font = new Font();
					font->SetFamilyAndStyle( m_cFamily.c_str(), m_cStyle.c_str() );
					font->SetSize( size );
					font->SetFlags( m_nFlags );
					m_pcCharMap->SetFont( font );
					font->Release();
	
					m_vSize = size;
				}
			}
			break;

		case ID_MENU_FONT_FAMILY_AND_STYLE:
			{
				const char *family,*style;
				int32 flags;

				if (pcMsg->FindString("family",&family) == 0 && pcMsg->FindString("style",&style) == 0 && pcMsg->FindInt32( "flags", &flags) == 0 )
				{
					Font *font = new Font();
					font->SetFamilyAndStyle(family,style);
					font->SetSize(m_vSize);
					flags |= FPF_SMOOTHED;
					font->SetFlags(flags);
					m_pcCharMap->SetFont(font);
					font->Release();
					m_cFamily = family;
					m_cStyle = style;
					m_nFlags = flags;
				}
			}
			break;

		default:
			// If we don't recognize the message, we'll pass it on to the superclass
			Window::HandleMessage( pcMsg );
	}
}


