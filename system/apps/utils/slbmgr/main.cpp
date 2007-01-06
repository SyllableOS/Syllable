
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
#include <gui/checkmenu.h>
#include <gui/requesters.h>

#include <util/application.h>
#include <util/message.h>
#include <util/settings.h>
#include <util/resources.h>

#include <atheos/time.h>
#include <atheos/kernel.h>

#include "processpanel.h"
#include "sysinfopanel.h"
#include "performancepanel.h"
#include "devicespanel.h"
#include "drivespanel.h"

#include "resources/SysInfo.h"

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
	ID_ABOUT,
	ID_UPDATE,
	ID_SAVE_WINFRAME
};

class MyWindow:public os::Window
{
	public:
	MyWindow();
	~MyWindow();

	// From Window:
	bool OkToQuit();
	virtual void TimerTick( int nID );

	//void SetInfoDetail( bool bVal );
	void ShowAbout();
	void CheckSpeedMenuItem( int nItem );
	virtual void HandleMessage( Message * pcMsg );

	void SetDelay( bigtime_t nDelay );

	void DoUpdate();

	private:
	void SetupMenus();

	int m_nUpdateCount;
	bigtime_t m_nDelay;
	Menu *m_pcMenu;
	TabView *m_pcView;
	ProcessPanel *pcProcessPanel;
	SysInfoPanel *pcSysInfoPanel;
	PerformancePanel *pcPerformancePanel;
	DevicesPanel *pcDevicesPanel;
	DrivesPanel *pcDrivesPanel;
	CheckMenu*	m_pcMenuMedium;
	CheckMenu*	m_pcMenuLow;
	CheckMenu*	m_pcMenuHigh;
	CheckMenu*	m_pcMenuPause;
	Settings*	m_pcSettings;
};

class MyApp:public Application
{
	public:
	MyApp( const char *pzName );
	 ~MyApp();

	// From Handler:
	virtual void HandleMessage( Message * pcMsg );

	private:
	MyWindow * m_pcWindow;
};

MyApp::MyApp( const char *pzName ):Application( pzName )
{
	SetCatalog("SysInfo.catalog");

	m_pcWindow = new MyWindow();
	m_pcWindow->SetDelay( 500000 );
	m_pcWindow->Show();
}

//----------------------------------------------------------------------------
MyApp::~MyApp()
{
	dbprintf("MyApp::~MyApp() IN\n");
	m_pcWindow->Terminate();
	dbprintf("MyApp::~MyApp() OUT\n");
}


//----------------------------------------------------------------------------
void MyApp::HandleMessage( Message * pcMsg )
{
	switch ( pcMsg->GetCode() )
	{
		default:
			Application::HandleMessage( pcMsg );
			return;
	}
}

void MyWindow::SetDelay( bigtime_t nDelay )
{
	m_nDelay = nDelay;
	AddTimer( this, 0, nDelay, false );
}

void MyWindow::CheckSpeedMenuItem( int nItem )
{
	m_pcMenuLow->SetChecked( nItem == 1 );
	m_pcMenuMedium->SetChecked( nItem == 2 );
	m_pcMenuHigh->SetChecked( nItem == 3 );
	m_pcMenuPause->SetChecked( nItem == 4 );
}

void MyWindow::HandleMessage( Message * pcMsg )
{
	switch ( pcMsg->GetCode() )
	{
		case ID_QUIT:
			Application::GetInstance()->PostMessage( M_QUIT );
			break;
		case ID_SPEED_LOW:
			CheckSpeedMenuItem( 1 );
			SetDelay( 1000000 );
			break;
		case ID_SPEED_MEDIUM:
			CheckSpeedMenuItem( 2 );
			SetDelay( 500000 );
			break;
		case ID_SPEED_HIGH:
			CheckSpeedMenuItem( 3 );
			SetDelay( 250000 );
			break;
		case ID_SPEED_PAUSE:
			CheckSpeedMenuItem( 4 );
			SetDelay( 1000001 );
			break;
		case ID_SAVE_WINFRAME:
			m_pcSettings->SetRect( "windowFrame", GetFrame() );
			m_pcSettings->Save();
			break;
		case ID_ABOUT:
			ShowAbout();
			break;
		case ID_UPDATE:
			DoUpdate();
			break;
		default:
			Window::HandleMessage( pcMsg );
			return;
	}
}

//void MyWindow::SetInfoDetail( bool bVal ){
//  pcSysInfoPanel->SetDetail( bVal );  
//}

void MyWindow::ShowAbout()
{
	char szTitle[128];
	char szMessage[1024];

	sprintf( szTitle, MSG_ABOUTWND_TITLE.c_str(), APP_VERSION );
	sprintf( szMessage, MSG_ABOUTWND_TEXT.c_str(), APP_VERSION );

	Alert *sAbout = new Alert( szTitle, szMessage, Alert::ALERT_INFO, 0x00, MSG_ABOUTWND_OK.c_str(), NULL );

	sAbout->Go( new Invoker(NULL) );
}

//----------------------------------------------------------------------------
void MyWindow::SetupMenus()
{
	Rect cMenuFrame = GetBounds();
	Rect cMainFrame = cMenuFrame;

	cMenuFrame.bottom = 16;

	m_pcMenu = new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );

	Menu *pcApp = new Menu( Rect(), MSG_MAINWND_MENU_APPLICATION, ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	pcApp->AddItem( new MenuItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new Message( ID_ABOUT ) ) );
	pcApp->AddItem( new MenuSeparator() );
	pcApp->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new Message( ID_QUIT ) );
	
	
	Menu *pcOption = new Menu( Rect(), MSG_MAINWND_MENU_OPTIONS, ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	Menu *pcSpeed = new Menu( Rect(), MSG_MAINWND_MENU_OPTIONS_SPEED, ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

	pcSpeed->AddItem( m_pcMenuLow = new CheckMenu( MSG_MAINWND_MENU_OPTIONS_SPEED_LOW, new Message( ID_SPEED_LOW ), false ) );
	pcSpeed->AddItem( m_pcMenuMedium = new CheckMenu( MSG_MAINWND_MENU_OPTIONS_SPEED_MEDIUM, new Message( ID_SPEED_MEDIUM ), true ) );
	pcSpeed->AddItem( m_pcMenuHigh = new CheckMenu( MSG_MAINWND_MENU_OPTIONS_SPEED_HIGH, new Message( ID_SPEED_HIGH ), false ) );
	pcSpeed->AddItem( m_pcMenuPause = new CheckMenu( MSG_MAINWND_MENU_OPTIONS_SPEED_PAUSE, new Message( ID_SPEED_PAUSE ), false ) );
	pcOption->AddItem( pcSpeed );

	pcOption->AddItem( MSG_MAINWND_MENU_OPTIONS_SAVEWINPOS, new Message( ID_SAVE_WINFRAME ) );


	/* DEPRECATED:  John Hall 8/17/2001

	   Menu*  pcDetail = new Menu( Rect( 0, 0, 100, 20 ), "* System Info", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	   pcDetail->AddItem( new MenuItem( "Simple",   new Message( ID_INFO_SIMPLE ) ) );
	   pcDetail->AddItem( new MenuItem( "Detailed", new Message( ID_INFO_DETAIL ) ) );
	   pcOption->AddItem( pcDetail );

	 */
	

	m_pcMenu->AddItem( pcApp );
	m_pcMenu->AddItem( pcOption );

	cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1;
	cMainFrame.top = cMenuFrame.bottom + 1;

	m_pcMenu->SetFrame( cMenuFrame );

	m_pcMenu->SetTargetForItems( this );

	AddChild( m_pcMenu );
}

//----------------------------------------------------------------------------
MyWindow::MyWindow():Window( Rect(), "athe_wnd", MSG_MAINWND_TITLE, 0 )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	m_nUpdateCount = 0;

	SetupMenus();

	Rect cMainFrame = GetBounds();
	Rect cMenuFrame = m_pcMenu->GetFrame();

	cMainFrame.top = cMenuFrame.bottom + 1.0f;

	/* Border for actual panel */
	cMainFrame.Resize( 5, 5, -5, -5 );

	m_pcView = new TabView( cMainFrame, "main_tab", CF_FOLLOW_ALL, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
	pcProcessPanel = new ProcessPanel( cMainFrame );
	pcSysInfoPanel = new SysInfoPanel( cMainFrame );
	pcPerformancePanel = new PerformancePanel( cMainFrame );
	pcDevicesPanel = new DevicesPanel( cMainFrame );
	pcDrivesPanel = new DrivesPanel( cMainFrame );

	m_pcView->AppendTab( MSG_TAB_SYSINFO, pcSysInfoPanel );
	m_pcView->AppendTab( MSG_TAB_DRIVES, pcDrivesPanel );
	m_pcView->AppendTab( MSG_TAB_DEVICES, pcDevicesPanel );
	m_pcView->AppendTab( MSG_TAB_PROCESSES, pcProcessPanel );
	m_pcView->AppendTab( MSG_TAB_PERFORMANCE, pcPerformancePanel );
	
	m_pcView->SetMessage( new Message( ID_UPDATE ) );
	m_pcView->SetTarget( this );

	AddChild( m_pcView );

	Point cMinSize = m_pcView->GetPreferredSize( false ) + Point( 150.0f, cMenuFrame.bottom + 200.0f );
	Point cMaxSize = m_pcView->GetPreferredSize( false ) + Point( 800.0f, cMenuFrame.bottom + 900.0f );
	SetSizeLimits( cMinSize, cMaxSize );

	m_pcSettings = new Settings();
	m_pcSettings->Load();

	SetFrame( m_pcSettings->GetRect( "windowFrame", Rect( 0, 0, cMinSize.x, cMinSize.y ) + Point( 50, 25 ) ) );

	AddTimer( this, 0, 500000, false );
}

//----------------------------------------------------------------------------
MyWindow::~MyWindow()
{
}

//----------------------------------------------------------------------------
bool MyWindow::OkToQuit()
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return ( true );
}

void MyWindow::DoUpdate()
{
	pcPerformancePanel->UpdatePerformanceList();
	pcSysInfoPanel->UpdateSysInfoPanel();
	pcDrivesPanel->UpdatePanel();
	int	nCount = pcProcessPanel->ThreadCount();
	if( pcProcessPanel->GetThreadCount() != nCount )
	{
		pcProcessPanel->UpdateProcessList();
		pcProcessPanel->SetThreadCount( nCount );
	}
}


void MyWindow::TimerTick( int nID )
{
	int nCount;
	
	if( ( m_nUpdateCount++ % 2 ) == 0 )
	{
		if( m_nDelay <= 1000000 )
		{
			pcPerformancePanel->UpdatePerformanceList();
			pcSysInfoPanel->UpdateSysInfoPanel();
		}

		nCount = pcProcessPanel->ThreadCount();

		if( pcProcessPanel->GetThreadCount() != nCount )
		{
			pcProcessPanel->UpdateProcessList();
			pcProcessPanel->SetThreadCount( nCount );
		}
	}
}

//----------------------------------------------------------------------------
int main()
{
	Application *pcMyApp;

	pcMyApp = new MyApp( "application/x-vnd.JMH-Syllable Manager" );
	pcMyApp->Run();
}
