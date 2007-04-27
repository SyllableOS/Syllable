/*  main.cpp - the main program for the AtheOS Clock v0.3
 *  Copyright (C) 2001 Chir
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "mywindow.h"
#include "resources/aclock.h"
#include <gui/imagebutton.h>

// ---------------------------------------------------------------------------

MyWindow::MyWindow( const Rect& cFrame) : Window( cFrame, "main_window", MSG_MAINWND_TITLE, WND_NO_ZOOM_BUT )
{
    settings = new Settings();
    settings->Load();

    settings->FindColor32("backcolor", &sColor);
    settings->FindBool("showdigital",&bDigital);
    settings->FindBool("showsec", &bSec);

    // Create a menu bar.
    Rect cMenuBounds = GetBounds();
    cMenuBounds.bottom = MENU_OFFSET;

    Menu* pcMenuBar = new Menu( cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE );

    // Create the menus within the bar.
    Menu* pcFileMenu = new Menu( Rect(0,0,1,1), MSG_MAINWND_MENU_FILE, ITEMS_IN_COLUMN );
    pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_EXIT, new Message (ID_FILE_EXIT) );

    pcMenuBar->AddItem( pcFileMenu );

    Menu* pcViewMenu = new Menu( Rect(0,0,1,1), MSG_MAINWND_MENU_VIEW, ITEMS_IN_COLUMN );

    Menu* pcScaleMenu = new Menu( Rect(0,0,1,1), MSG_MAINWND_MENU_VIEW_SECONDS, ITEMS_IN_COLUMN );
    pcScaleMenu->AddItem( MSG_MAINWND_MENU_VIEW_SECONDS_ON, new Message( ID_VIEW_SECONDS_ON ) );
    pcScaleMenu->AddItem( MSG_MAINWND_MENU_VIEW_SECONDS_OFF, new Message( ID_VIEW_SECONDS_OFF ) );
    pcViewMenu->AddItem( pcScaleMenu );

    Menu* pcModeMenu = new Menu( Rect(0,0,1,1), MSG_MAINWND_MENU_VIEW_MODE, ITEMS_IN_COLUMN );
    pcModeMenu->AddItem( MSG_MAINWND_MENU_VIEW_MODE_DIGITAL, new Message( ID_VIEW_DIGITAL_ON ) );
    pcModeMenu->AddItem( MSG_MAINWND_MENU_VIEW_MODE_ANALOG, new Message( ID_VIEW_DIGITAL_OFF ) );
    pcViewMenu->AddItem( pcModeMenu );

    Menu* pcColorMenu = new Menu( Rect(0,0,1,1), MSG_MAINWND_MENU_VIEW_COLOR, ITEMS_IN_COLUMN );
    pcColorMenu->AddItem( MSG_MAINWND_MENU_VIEW_COLOR_WHITE, new Message( ID_WHITE ) );
    pcColorMenu->AddItem( MSG_MAINWND_MENU_VIEW_COLOR_BLACK, new Message( ID_BLACK ) );
    pcViewMenu->AddItem( pcColorMenu );

    pcMenuBar->AddItem( pcViewMenu );
    cMenuBounds.bottom = pcMenuBar->GetPreferredSize( true ).y + -4.0f;  //Sizes the menu.  The higher the negative, the smaller the menu.
    pcMenuBar->SetFrame( cMenuBounds);

    pcMenuBar->SetTargetForItems( this );

    // Add the menubar to the window.
    AddChild( pcMenuBar );

    Rect cWindowBounds = GetBounds();
    cWindowBounds.left = 0;
    cWindowBounds.top = cMenuBounds.bottom + 1;

    // Add the component to the window.

    m_pcDisplay = new ClockView( cWindowBounds,sColor, bSec,bDigital );
    m_pcDisplay->showSeconds(bSec);
    m_pcDisplay->showDigital(bDigital);
    AddChild( m_pcDisplay );

	// Set Icon
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

MyWindow::~MyWindow()
{
}

bool MyWindow::OkToQuit( void )
{
    settings->RemoveName("backcolor");
    settings->RemoveName("showdigital");
    settings->RemoveName("showsec");
	
	sColor = m_pcDisplay->GetBackColor();
	
    settings->AddColor32("backcolor", sColor);
    settings->AddBool("showdigital",bDigital);
    settings->AddBool("showsec", bSec);

    settings->Save();

    Application::GetInstance()->PostMessage( M_QUIT );
    return( false );
}

void MyWindow::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() ) //Get the custom message code.
    {
    case ID_FILE_EXIT:
        {
            OkToQuit();
        }
        break;

    case ID_VIEW_SECONDS_ON:
        {
            m_pcDisplay->showSeconds(true);
            bSec = true;
        }
        break;

    case ID_VIEW_SECONDS_OFF:
        {
            m_pcDisplay->showSeconds(false);
            bSec = false;
        }
        break;

    case ID_VIEW_DIGITAL_ON:
        {
            m_pcDisplay->showDigital(true);
            bDigital = true;
        }
        break;

    case ID_VIEW_DIGITAL_OFF:
        {
            m_pcDisplay->showDigital(false);
            bDigital = false;
        }
        break;

    case ID_BLACK:
        {
            m_pcDisplay->SetBackColor(Color32_s (0,0,0,0));
        }
        break;

    case ID_WHITE:
        {
            m_pcDisplay->SetBackColor(Color32_s (255,255,255,255));
        }
        break;

    default:
        Window::HandleMessage( pcMessage );
        break;
    }
}

// ---------------------------------------------------------------------------

MyApp::MyApp() : Application( "application/x-vnd-RGC-" APP_NAME )
{
    settings = new Settings();
    LoadSettings();

	SetCatalog("aclock.catalog");

    m_pcMainWindow = new MyWindow( Rect(150,150,270,270));
    m_pcMainWindow->CenterInScreen();
    m_pcMainWindow->Show();
    m_pcMainWindow->MakeFocus();


}

MyApp::~MyApp()
{
}

bool MyApp::LoadSettings()
{
    bool bFlag = false;

    if(settings->Load() != 0)
    {
        LoadDefaults();
    }
    else
    {
        settings->FindColor32("backcolor", &sColor);

        if (settings->FindBool("showdigital",&bDigital) != 0)
            bDigital = true;

        if (settings->FindBool("showsec",&bSec)!= 0)
            bSec = true;

        bFlag = true;
    }
    return bFlag;
}




void MyApp::LoadDefaults()
{
    sColor = Color32_s(255,255,255,255);
    bDigital = false;
    bSec = true;
}







// ---------------------------------------------------------------------------

int main( int argc, char** argv )
{
    MyApp* pcApp = new MyApp();

    pcApp->Run();                               // Enter main GUI loop.
    return( 0 );
}

// ---------------------------------------------------------------------------

