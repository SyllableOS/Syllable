/*
 *  AtheMgr - System Manager for AtheOS
 *  Copyright (C) 2001 John Hall
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/requesters.h>

#include <util/application.h>
#include <util/message.h>

#include <atheos/time.h>
#include <atheos/kernel.h>

#include "processpanel.h"
#include "sysinfopanel.h"
#include "performancepanel.h"


#include <gui/tabview.h>
using namespace os;

enum
{
  ID_QUIT,
  ID_SPEED_LOW,
  ID_SPEED_MEDIUM,
  ID_SPEED_HIGH,
  ID_SPEED_PAUSE,
  ID_INFO_SIMPLE,
  ID_INFO_DETAIL,
  ID_ABOUT
};

class MyWindow : public os::Window
{
public:
    MyWindow( const Rect& cFrame );
    ~MyWindow();

    // From Window:
    bool    OkToQuit();
    virtual void TimerTick( int nID );

    //void SetInfoDetail( bool bVal );
    void ShowAbout();
   
    bigtime_t m_nDelay;
   
private:
    int	m_nUpdateCount;

    void SetupMenus();

    Menu* m_pcMenu;
    TabView*          m_pcView;
    ProcessPanel*     pcProcessPanel;
    SysInfoPanel*     pcSysInfoPanel;
    PerformancePanel* pcPerformancePanel;
};

class	MyApp : public Application
{
public:
  MyApp( const char* pzName );
  ~MyApp();

    // From Handler:
  virtual void HandleMessage( Message* pcMsg );
          void SetDelay( bigtime_t nDelay );

private:
  MyWindow* m_pcWindow;
  bigtime_t m_nDelay;
};

MyApp::MyApp( const char* pzName ) : Application( pzName )
{
  m_pcWindow = new MyWindow( Rect( 0, 0, 250 - 1, 300 - 1 ) + Point( 50, 25 ) );
  SetDelay( 500000 );
  m_pcWindow->Show();
}

//----------------------------------------------------------------------------
MyApp::~MyApp()
{
  m_pcWindow->Quit();
}

void MyApp::SetDelay( bigtime_t nDelay)
{
  m_nDelay = nDelay;
  m_pcWindow->m_nDelay = nDelay;
}



//----------------------------------------------------------------------------
void MyApp::HandleMessage( Message* pcMsg )
{
    switch( pcMsg->GetCode() )
    {
	case ID_QUIT:{
	    Message cMsg( M_QUIT );
	    m_pcWindow->PostMessage( &cMsg );
	    return;
	}
	case ID_SPEED_LOW:     SetDelay( 1000000 );  break;
    	case ID_SPEED_MEDIUM:  SetDelay( 500000  );  break;
    	case ID_SPEED_HIGH:    SetDelay( 250000  );  break;
    	case ID_SPEED_PAUSE:   SetDelay( 1000001 );  break;

        /*  DEPRECATED:  8/18/2001  */
        //case ID_INFO_SIMPLE:   m_pcWindow->SetInfoDetail( false ); break;
    	//case ID_INFO_DETAIL:   m_pcWindow->SetInfoDetail( true ); break;
        case ID_ABOUT:         m_pcWindow->ShowAbout(); break;
       
	default:
	    Application::HandleMessage( pcMsg );
	    return;
    }

    m_pcWindow->Lock();
    m_pcWindow->AddTimer( m_pcWindow, 0, m_nDelay, false );
    m_pcWindow->Unlock();
}

//void MyWindow::SetInfoDetail( bool bVal ){
//  pcSysInfoPanel->SetDetail( bVal );  
//}

void MyWindow::ShowAbout(){
  char szTitle[128];
  char szMessage[1024];
  
  sprintf( szTitle,   "About Syllable Manager v.%s", APP_VERSION );
  sprintf( szMessage, "SLBMgr v.%s\n\nResource Manager For Syllable\nJohn Hall, 2001\n\nAtheMgr is released under the GNU Public License (GPL)\n\nAtheMgr: http://www.triumvirate.net/athemgr.htm\nGNU: http://www.gnu.org/", APP_VERSION );

  Alert* sAbout = new Alert( szTitle, szMessage, 0x00, "O.K", NULL);
  sAbout->Go();
}

//----------------------------------------------------------------------------
void MyWindow::SetupMenus()
{
  Rect cMenuFrame = GetBounds();
  Rect cMainFrame = cMenuFrame;
  cMenuFrame.bottom = 16;
  
  m_pcMenu = new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW,
		       CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );
 
  Menu*	pcFile	 = new Menu( Rect( 0, 0, 100, 20 ), "File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    pcFile->AddItem( "Quit", new Message( ID_QUIT ) );
  Menu*	pcOption = new Menu( Rect( 0, 0, 100, 20 ), "Options", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    Menu*  pcSpeed = new Menu( Rect( 0, 0, 100, 20 ),  "Speed", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
      pcSpeed->AddItem( new MenuItem( "Low",       new Message( ID_SPEED_LOW    ) ) );
      pcSpeed->AddItem( new MenuItem( "Medium",    new Message( ID_SPEED_MEDIUM ) ) );
      pcSpeed->AddItem( new MenuItem( "High",      new Message( ID_SPEED_HIGH   ) ) );
      pcSpeed->AddItem( new MenuItem( "Pause",     new Message( ID_SPEED_PAUSE  ) ) );
    pcOption->AddItem( pcSpeed );

    /* DEPRECATED:  John Hall 8/17/2001

    Menu*  pcDetail = new Menu( Rect( 0, 0, 100, 20 ), "* System Info", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
      pcDetail->AddItem( new MenuItem( "Simple",   new Message( ID_INFO_SIMPLE ) ) );
      pcDetail->AddItem( new MenuItem( "Detailed", new Message( ID_INFO_DETAIL ) ) );
    pcOption->AddItem( pcDetail );

    */
  Menu*	pcHelp = new Menu( Rect( 0, 0, 100, 20 ), "Help", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    pcHelp->AddItem( new MenuItem( "About",        new Message( ID_ABOUT  ) ) );

  m_pcMenu->AddItem( pcFile   );
  m_pcMenu->AddItem( pcOption );
  m_pcMenu->AddItem( pcHelp   );
   
  cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1;
  cMainFrame.top = cMenuFrame.bottom + 1;
  
  m_pcMenu->SetFrame( cMenuFrame );
  
  m_pcMenu->SetTargetForItems( Application::GetInstance() );
  
  AddChild( m_pcMenu );
}

//----------------------------------------------------------------------------
MyWindow::MyWindow( const Rect& cFrame ) :
    Window( cFrame, "athe_wnd", APP_WINDOW_TITLE, 0 )
{
    m_nUpdateCount  = 0;

    SetupMenus();

    Rect cMainFrame = GetBounds();
    Rect cMenuFrame = m_pcMenu->GetFrame();

    cMainFrame.top = cMenuFrame.bottom + 1.0f;

    /* Border for actual panel */
    cMainFrame.Resize( 5, 5, -5, -5 );
  
    m_pcView = new TabView( cMainFrame, "main_tab", CF_FOLLOW_ALL, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    pcProcessPanel     = new ProcessPanel( cMainFrame );
    pcSysInfoPanel     = new SysInfoPanel( cMainFrame );
    pcPerformancePanel = new PerformancePanel( cMainFrame );

    m_pcView->AppendTab( "System Info", pcSysInfoPanel );
    m_pcView->AppendTab( "Processes", pcProcessPanel );
    m_pcView->AppendTab( "Performance", pcPerformancePanel );
  
    AddChild( m_pcView );
    MakeFocus( true );
    //pcSysInfoPanel->MakeFocus( true );
    pcProcessPanel->MakeFocus( true );
    Point cMinSize = m_pcView->GetPreferredSize(false) + Point( 150.0f, cMenuFrame.bottom + 200.0f );
    Point cMaxSize = m_pcView->GetPreferredSize(false) + Point( 800.0f, cMenuFrame.bottom + 900.0f );

    /* Uncomment this next line when done with testing */
    ResizeTo( cMinSize );
    SetSizeLimits( cMinSize, cMaxSize );

    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	struct stat sStat;
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/nSysPanel.cfg" );

	if ( stat( zPath, &sStat ) >= 0 ) {
	    hFile = fopen( zPath, "rb" );
	    if ( NULL != hFile ) {
		uint8* pBuffer = new uint8[sStat.st_size];
		fread( pBuffer, sStat.st_size, 1, hFile );
		fclose( hFile );
		cArchive.Unflatten( pBuffer );
		
		Rect cFrame;
		if ( cArchive.FindRect( "window_frame", &cFrame ) == 0 ) {
		    SetFrame( cFrame );
		}
	    }
	}
    }
  
  AddTimer( this, 0, 500000, false );
  Unlock();
}

//----------------------------------------------------------------------------
MyWindow::~MyWindow()
{
}

//----------------------------------------------------------------------------
bool MyWindow::OkToQuit()
{
/*
   Commented this section out until I can decide if I want the
   program to have a saved location or not...
*/

/*    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
	FILE*	hFile;
	char	zPath[ PATH_MAX ];
	Message	cArchive;
	
	strcpy( zPath, pzHome );
	strcat( zPath, "/config/nSysPanel.cfg" );

	hFile = fopen( zPath, "wb" );

	if ( NULL != hFile ) {
	    cArchive.AddRect( "window_frame", GetFrame() );

	    int nSize = cArchive.GetFlattenedSize();
	    uint8* pBuffer = new uint8[nSize];
	    cArchive.Flatten( pBuffer, nSize );
	    fwrite( pBuffer, nSize, 1, hFile );
	    fclose( hFile );
	}
    }*/
    
    Application::GetInstance()->PostMessage( M_QUIT );
    return( true );
}

void MyWindow::TimerTick( int nID )
{
  int nCount;
   
  if ( (m_nUpdateCount++ % 2) == 0 ) {
    if(m_nDelay <= 1000000){
      pcPerformancePanel->UpdatePerformanceList();
      pcSysInfoPanel->UpdateSysInfoPanel();
    }
     
    nCount = pcProcessPanel->ThreadCount();
     
    if( pcProcessPanel->GetThreadCount() != nCount ){
      pcProcessPanel->UpdateProcessList();
      pcProcessPanel->SetThreadCount( nCount );
    }
  }
}

//----------------------------------------------------------------------------
int main()
{
  Application* pcMyApp;
  pcMyApp = new MyApp( "application/x-vnd.JMH-Syllable Manager" );
  pcMyApp->Run();
}






