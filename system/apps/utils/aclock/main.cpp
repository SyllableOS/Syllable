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

// ---------------------------------------------------------------------------

MyWindow::MyWindow( const Rect& cFrame ) : Window( cFrame, "main_window", "Clock" )
{
   // Create a menu bar.
   Rect cMenuBounds = GetBounds();
   cMenuBounds.bottom = MENU_OFFSET;
   
   Menu* pcMenuBar = new Menu( cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE );

   // Create the menus within the bar.
   Menu* pcFileMenu = new Menu( Rect(0,0,1,1), "File", ITEMS_IN_COLUMN );
   pcFileMenu->AddItem( "Exit", new Message (ID_FILE_EXIT) );
   
   pcMenuBar->AddItem( pcFileMenu );
   
   Menu* pcViewMenu = new Menu( Rect(0,0,1,1), "View", ITEMS_IN_COLUMN );

   Menu* pcScaleMenu = new Menu( Rect(0,0,1,1), "Seconds  >>", ITEMS_IN_COLUMN );   
   pcScaleMenu->AddItem( "On", new Message( ID_VIEW_SECONDS_ON ) );   
   pcScaleMenu->AddItem( "Off", new Message( ID_VIEW_SECONDS_OFF ) );
   pcViewMenu->AddItem( pcScaleMenu );
   
   Menu* pcModeMenu = new Menu( Rect(0,0,1,1), "Mode  >>", ITEMS_IN_COLUMN );
   pcModeMenu->AddItem( "Digital", new Message( ID_VIEW_DIGITAL_ON ) );   
   pcModeMenu->AddItem( "Analog", new Message( ID_VIEW_DIGITAL_OFF ) );
   pcViewMenu->AddItem( pcModeMenu );

   pcMenuBar->AddItem( pcViewMenu );
   
//   Menu* pcHelpMenu = new Menu( Rect(0,0,1,1), "Help", ITEMS_IN_COLUMN );
//   pcHelpMenu->AddItem( "About", new Message( ID_HELP_ABOUT ) );
//   pcMenuBar->AddItem( pcHelpMenu );

   pcMenuBar->SetTargetForItems( this );
   
   // Add the menubar to the window.
   AddChild( pcMenuBar );
   
   Rect cWindowBounds = GetBounds();
   cWindowBounds.left = 0;
   cWindowBounds.top = MENU_OFFSET + 1;
   
   // Add the component to the window.
   m_pcDisplay = new ClockView( cWindowBounds );
   AddChild( m_pcDisplay );
}

MyWindow::~MyWindow()
{
    delete m_pcDisplay;
}

bool MyWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT );
    return( false );
}

void MyWindow::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() ) //Get the custom message code.
    {
      case ID_FILE_EXIT:
        OkToQuit();
        break;
      case ID_VIEW_SECONDS_ON:
        m_pcDisplay->showSeconds(true);
        break;
      case ID_VIEW_SECONDS_OFF:
        m_pcDisplay->showSeconds(false);
        break;
      case ID_VIEW_DIGITAL_ON:
        m_pcDisplay->showDigital(true);
        break;
      case ID_VIEW_DIGITAL_OFF:
        m_pcDisplay->showDigital(false);
        break;
      default:
        Window::HandleMessage( pcMessage );
        break;
    }
}

// ---------------------------------------------------------------------------

MyApp::MyApp() : Application( "application/x-VND.KHS-Clock" )
{
    m_pcMainWindow = new MyWindow( Rect(150,150,270,270) );
    m_pcMainWindow->Show();
    m_pcMainWindow->MakeFocus();
}

MyApp::~MyApp()
{
   delete m_pcMainWindow;
}

// ---------------------------------------------------------------------------

int main( int argc, char** argv )
{
    MyApp* pcApp = new MyApp();
    
    pcApp->Run();                               // Enter main GUI loop.
    return( 0 );
}

// ---------------------------------------------------------------------------
