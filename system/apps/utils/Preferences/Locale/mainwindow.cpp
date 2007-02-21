// Locale Preferences :: (C)opyright 2004 Henrik Isaksson
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

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <util/application.h>
#include <util/locale.h>
#include <util/message.h>
#include <util/settings.h>
#include <util/resources.h>
#include <storage/file.h>
#include <gui/tabview.h>
#include <gui/treeview.h>
#include <appserver/keymap.h>
#include "mainwindow.h"
#include "messages.h"
#include "main.h"
#include <iostream>
#include <sys/stat.h>

#include "resources/Locale.h"

/*
  // Create the layouts/views
  m_pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  m_pcVLRoot = new os::VLayoutNode("VL");
  m_pcVLRoot->SetBorders(os::Rect(10, 5, 10, 5));

  // Tabview

  // Hor time/date view & node
  m_pcLTimeDate = new os::LayoutView(cBounds, "LTimeDate", NULL, os::CF_FOLLOW_ALL);
  m_pcVLTimeDate = new os::VLayoutNode("VLTimeDate");
  m_pcVLTimeDate->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcHLTimeDate = new os::HLayoutNode("HLTimeDate", 1.0f, m_pcVLTimeDate);

  // Time
  m_pcHLTime = new os::HLayoutNode("HLTime");
  m_pcHLTime->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcHLTime->AddChild(m_pcTPTime = new os::TimePicker(cRect, "TPTime"));
  m_pcFVTime = new os::FrameView(cBounds, "FVTime", "Time", os::CF_FOLLOW_ALL);
  m_pcFVTime->SetRoot(m_pcHLTime);
  m_pcHLTimeDate->AddChild(m_pcFVTime);
				 
  // Space
  m_pcHLTimeDate->AddChild(new os::HLayoutSpacer("", 10.0f, 10.0f));
  
  // Date
  m_pcHLDate = new os::HLayoutNode("HLDate");
  m_pcHLDate->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcHLDate->AddChild(m_pcDPDate = new os::DatePicker(cRect, "DPDate"));
  m_pcFVDate = new os::FrameView(cBounds, "FVDate", "Date", os::CF_FOLLOW_ALL);
  m_pcFVDate->SetRoot(m_pcHLDate);
  m_pcHLTimeDate->AddChild(m_pcFVDate);
 
  // Add full time/date stringview
  m_pcVLTimeDate->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f));
  m_pcVLTimeDate->AddChild(m_pcSVFullTimeDate = new os::StringView(cRect, "SVFull", "", os::ALIGN_CENTER));
  m_pcSVFullTimeDate->SetMinPreferredSize(50);
  m_pcSVFullTimeDate->SetMaxPreferredSize(50);
  
  // Add time/date tab
  m_pcLTimeDate->SetRoot(m_pcVLTimeDate);
  m_pcTVRoot->AppendTab("Time & Date", m_pcLTimeDate);

  // Create bitmap object and Load map
  m_pcBIImage = new os::BitmapImage();
  m_pcBIImageSaved = new os::BitmapImage();
  
  // Load image and save it 
  os::File *cFile = new os::File("timezone.png");
  m_pcBIImage->Load(cFile, "image/png");
  delete cFile;
  
  // Store it
  *m_pcBIImageSaved = *m_pcBIImage;
  
  // Add timezone
  m_pcLTimeZone = new os::LayoutView(cBounds, "LTimeZone", NULL, os::CF_FOLLOW_ALL);
  m_pcVLTimeZone = new os::VLayoutNode("VLTimeZone");
  m_pcVLTimeZone->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcVLTimeZone->AddChild(new os::HLayoutSpacer(""));
  
  // Timezone
  m_pcHLTimeZone = new os::HLayoutNode("HLTimeZone", 1.0f, m_pcVLTimeZone);
  m_pcDDMTimeZone = new os::DropdownMenu(cRect, "DDMTimeZone");
  m_pcDDMTimeZone->SetSelectionMessage(new os::Message(M_MW_TIMEZONECHANGED));
  m_pcDDMTimeZone->SetTarget(this);
  m_pcDDMTimeZone->SetMinPreferredSize(30);
  m_pcDDMTimeZone->SetMaxPreferredSize(30);
  m_pcHLTimeZone->AddChild(m_pcDDMTimeZone);
  m_pcHLTimeZone->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f));

  // Map

*/

class LanguageInfo {
	public:
	LanguageInfo( char *s )
	{
		char bfr[256], *b;
		int i, j;

		for( j = 0; j < 6 && *s; j++ ) {
			for( b = bfr, i = 0; i < sizeof( bfr ) && *s && *s != '\t'; i++ )
				*b++ = *s++;
			*b = 0;
			for( ; *s == '\t' ; s++ );

			m_cInfo[j] = bfr;
		}

		m_bValid = false;		
		if( m_cInfo[0][0] != '#' && GetName() != "" && GetCode() != "" && GetFlag() != "" ) {
			m_bValid = true;
		}
	}

	bool IsValid() { return m_bValid; }
	String& GetName() { return m_cInfo[0]; }
	String& GetContinent() { return m_cInfo[1]; }
	String& GetCountry() { return m_cInfo[2]; }
	String& GetFamily() { return m_cInfo[3]; }
	String& GetCode() { return m_cInfo[4]; }
	String& GetFlag() { return m_cInfo[5]; }
	private:
	String m_cInfo[6];
	bool m_bValid;
};

void MainWindow::_LoadLanguages()
{
	int i, n, lp = 0;
	char chr, linebfr[ 512 ];
	Locale l;

	StreamableIO* pcDatabase = l.GetLocalizedResourceStream( "languages.db" );

	do {
		n = pcDatabase->Read( &chr, sizeof( char ) );
		if( n != 1 || chr == '\n' ) {	// EOF or EOL
			linebfr[ lp ] = 0;
			lp = 0;
			LanguageInfo	li( linebfr );

			if( li.IsValid() ) {
				TreeViewStringNode* lvr = new TreeViewStringNode();
				lvr->AppendString( li.GetName() );
				lvr->AppendString( li.GetCode() );
				StreamableIO* pcResStream = l.GetResourceStream( li.GetFlag() );
				if( pcResStream ) {
					Image* pcIcon = new BitmapImage( pcResStream );
					pcIcon->SetSize( Point( 36, 24 ) );
					lvr->SetIcon( pcIcon );
					delete pcResStream;
				}
				m_pcAvailable->InsertRow( lvr );
			}
		} else {
			linebfr[ lp++ ] = chr;
			if( lp > sizeof( linebfr ) - 1 ) {
				lp = sizeof( linebfr ) - 1;
			}
		}
	} while( n > 0 );
}

MainWindow::MainWindow()
:Window( Rect(0,0,100,100), "MainWindow", MSG_MAIN_TITLE )
{
	Rect cBounds = GetBounds();
	LayoutView* pcLayoutView = new LayoutView( cBounds, "pcLayoutView", NULL, CF_FOLLOW_ALL );


	VLayoutNode*	pcRootLayout = new VLayoutNode( "pcRootLayout" );
	pcRootLayout->SetBorders( Rect( 10, 5, 10, 5 ) );

	{
		TabView* pcTabView = new TabView(cBounds, "pcTabView");
		
		{
			LayoutView* pcLangLayout = new LayoutView( cBounds, "pcLangLayout", NULL, CF_FOLLOW_ALL );
			{
				VLayoutNode* pcLangRoot = new VLayoutNode( "pcLangRoot" );
				pcLangRoot->SetBorders( Rect( 10, 5, 10, 5 ) );
				{
					HLayoutNode* pcListViews = new HLayoutNode( "pcListViews", 100.0f );
					VLayoutNode* pcAvail = new VLayoutNode( "pcAvail" );
					StringView* pcAvailLabel = new StringView( cBounds, "pcAvailLabel", MSG_MAIN_LANGUAGE_AVAILABLE );
					pcAvail->AddChild( pcAvailLabel, 0.0f );
					m_pcAvailable = new ListView( cBounds, "m_pcAvailable" );
					m_pcAvailable->SetHasColumnHeader( false );
					m_pcAvailable->InsertColumn( "", 1000 );
					pcAvail->AddChild( m_pcAvailable );
					pcListViews->AddChild( pcAvail );

					pcListViews->AddChild( new HLayoutSpacer( "", 5.0f, 5.0f ) );
					VLayoutNode* pcPref = new VLayoutNode( "pcPref" );
					StringView* pcPrefLabel = new StringView( cBounds, "pcPrefLabel", MSG_MAIN_LANGUAGE_USED );
					pcPref->AddChild( pcPrefLabel, 0.0f );
					m_pcPreferred = new ListView( cBounds, "m_pcPreferred" );
					m_pcPreferred->SetHasColumnHeader( false );
					m_pcPreferred->InsertColumn( "", 1000 );
					pcPref->AddChild( m_pcPreferred );
					pcListViews->AddChild( pcPref );
					
					float vWidth = std::max( pcPref->GetPreferredSize( false ).x , pcAvail->GetPreferredSize( false ).x );
					pcPref->ExtendMinSize( Point( vWidth, 0.0f ) );
					pcAvail->ExtendMinSize( Point( vWidth, 0.0f ) );
					
					pcLangRoot->AddChild( pcListViews );
				}			
				pcLangRoot->AddChild( new VLayoutSpacer( "", 5.0f, 5.0f ) );
				{
					os::String cAvail = os::String( MSG_MAIN_LANGUAGE_ACTIVATE );
					for( uint8 i = 0 ; i < 75 - strlen(cAvail.c_str()) ; i++ )
						cAvail = cAvail + os::String(" ");
					os::String cPref = os::String( MSG_MAIN_LANGUAGE_DEACTIVATE );
					for( uint8 i = 0 ; i < 75 - strlen(cPref.c_str()) ; i++ )
						cPref = cPref + os::String(" ");

					HLayoutNode* pcAddRemLang = new HLayoutNode( "pcAddRemLang", 0.0f );
					m_pcAddLang = new Button( cBounds, "m_pcAddLang", cAvail, new Message( M_MW_ADDLANG ) );
					m_pcRemLang = new Button( cBounds, "m_pcRemLang", cPref, new Message( M_MW_REMLANG ) );
					pcAddRemLang->AddChild( new HLayoutSpacer( "" ) );
					pcAddRemLang->AddChild( m_pcAddLang );
					pcAddRemLang->AddChild( new HLayoutSpacer( "" ) );
					pcAddRemLang->AddChild( m_pcRemLang );
					pcAddRemLang->AddChild( new HLayoutSpacer( "" ) );
					pcAddRemLang->SameWidth( "m_pcAddLang", "m_pcRemLang", NULL );
					pcLangRoot->AddChild( pcAddRemLang );
				}
				pcLangLayout->SetRoot( pcLangRoot );
			}
			pcTabView->AppendTab(MSG_MAIN_LANGUAGE_TAB, pcLangLayout);
		}

		{
			LayoutView* pcCountryLayout = new LayoutView( cBounds, "pcCountryLayout", NULL, CF_FOLLOW_ALL );
			LayoutNode* pcCountryRoot = new HLayoutNode( "" );
			pcCountryRoot->AddChild(new VLayoutSpacer( "" ) );
			pcCountryLayout->SetRoot( pcCountryRoot );
			pcTabView->AppendTab(MSG_MAIN_COUNTRY_TAB, pcCountryLayout);
		}
		
		pcRootLayout->AddChild( pcTabView );
	}

	pcRootLayout->AddChild( new VLayoutSpacer( "", 5.0f, 5.0f ) );

	{
		HLayoutNode*	pcButtonsLayout = new HLayoutNode( "pcButtonsLayout", 0.0f );
		m_pcApply = new Button( cBounds, "m_pcApply", MSG_MAIN_APPLY, new Message( M_MW_APPLY ) );
		m_pcRevert = new Button( Rect(), "m_pcRevert", MSG_MAIN_REVERT, new Message( M_MW_UNDO ) );
		m_pcDefault = new Button( Rect(), "m_pcDefault", MSG_MAIN_DEFAULT, new Message( M_MW_DEFAULT ) );
		pcButtonsLayout->AddChild( new HLayoutSpacer( "" ) );
		pcButtonsLayout->AddChild( m_pcApply );
		pcButtonsLayout->AddChild( m_pcRevert );
		pcButtonsLayout->AddChild( m_pcDefault );
		pcButtonsLayout->SameWidth( "m_pcApply", "m_pcRevert", "m_pcDefault", NULL );
		pcRootLayout->AddChild( pcButtonsLayout );
	}
	
	// Set Icon
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	
	_LoadLanguages();
	_ShowData();

	pcLayoutView->SetRoot( pcRootLayout );
	AddChild( pcLayoutView );
	Point	cSize = pcLayoutView->GetPreferredSize( false );
	Rect	cFrame( 0, 0, cSize.x, cSize.y + 220.0f );
	cFrame.MoveTo( 100, 100 );
	SetFrame( cFrame );
}

MainWindow::~MainWindow()
{
}

void MainWindow::_RemLang()
{
	int i = m_pcPreferred->GetFirstSelected();

	if( i != -1 ) {
		m_pcPreferred->ClearSelection();
		ListViewRow* r = m_pcPreferred->RemoveRow( i );
		m_pcAvailable->InsertRow( r );
	}
}

void MainWindow::_AddLang()
{
	int i = m_pcAvailable->GetFirstSelected();

	if( i != -1 ) {
		m_pcAvailable->ClearSelection();
		ListViewRow* r = m_pcAvailable->RemoveRow( i );
		m_pcPreferred->InsertRow( 10000, r );
	}
}

ListViewRow* MainWindow::_RemoveRow( const String& cName )
{
	int n = m_pcAvailable->GetRowCount();
	int i;
	
	for( i = 0; i < n; i++ ) {
		TreeViewStringNode* pcNode = (TreeViewStringNode*)m_pcAvailable->GetRow( i );
		if( pcNode ) {
			if( pcNode->GetString( 1 ) == cName ) {
				return m_pcAvailable->RemoveRow( i );
			}
		}
	}
	return NULL;
}

void MainWindow::_ShowData()
{
	try {
		char* pzHome;
		pzHome = getenv( "HOME" );
		File* pcFile = new File( String( pzHome ) + String( "/Settings/System/Locale" ) );
		Settings cSettings( pcFile );
		cSettings.Load( );

		int i;
		int nCount = 0, nType;
		cSettings.GetNameInfo( "LANG", &nType, &nCount );
		for( i = 0; i < nCount; i++ ) {
			String cString;
			if( cSettings.FindString( "LANG", &cString, i ) == 0 ) {
				ListViewRow* r = _RemoveRow( cString );
				if( r ) {
					m_pcPreferred->InsertRow( 10000, r );
				}
			}
		}
	} catch( ... ) {
	}
}

void MainWindow::_Apply()
{
	try {
		int i;

		char* pzHome;
		pzHome = getenv( "HOME" );
		Path path( String( pzHome ) + String( "/Settings/System" ) );
//		File* pcFile = new File( String( pzHome ) + String( "/Settings/System/Locale" ), O_CREAT );
		Settings cSettings;
		cSettings.SetPath( &path );
		cSettings.SetFile( "Locale" );
		
		for( i = 0; i < m_pcPreferred->GetRowCount(); i++ ) {
			TreeViewStringNode* pcNode = (TreeViewStringNode*)m_pcPreferred->GetRow( i );
			cSettings.AddString( "LANG", pcNode->GetString( 1 ) );
		}
	
		cSettings.Save();
	} catch( ... ) {
		dbprintf("%s\n", MSG_ERROR_SAVE.c_str());
	}
}

void MainWindow::_Undo()
{
}

void MainWindow::_Default()
{
}

void MainWindow::HandleMessage( Message * pcMessage )
{
	// Get message code and act on it
	switch ( pcMessage->GetCode() )
	{
		case M_MW_ADDLANG:
			_AddLang();
			break;

		case M_MW_REMLANG:
			_RemLang();
			break;

		case M_MW_APPLY:
			_Apply();
			break;
	
		case M_MW_UNDO:
			_Undo();
			break;
	
		case M_MW_DEFAULT:
			_Default();
			break;
	
		default:
			Window::HandleMessage( pcMessage );
			break;
	}

}

bool MainWindow::OkToQuit()
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

