/*  main.cpp - the main program for the AtheOS Zoom v0.1
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

MyWindow::MyWindow( const Rect& cFrame ) : Window( cFrame, "main_window", "Zoom" )
{
   // Create a menu bar.
   Rect cMenuBounds = GetBounds();
   cMenuBounds.bottom = MENU_OFFSET+3;
   
   Menu* pcMenuBar = new Menu( cMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE );

   // Create the menus within the bar.
   Menu* pcFileMenu = new Menu( Rect(0,0,1,1), "App", ITEMS_IN_COLUMN );
   pcFileMenu->AddItem( "Exit", new Message (ID_EXIT) );
   
   pcMenuBar->AddItem( pcFileMenu );
   
   Menu* pcViewMenu = new Menu( Rect(0,0,1,1), "View", ITEMS_IN_COLUMN );
   Menu* pcScaleMenu = new Menu( Rect(0,0,1,1), "Magnification  >>", ITEMS_IN_COLUMN );
   
   pcScaleMenu->AddItem( "2x", new Message( ID_VIEW_SCALE_2 ) );   
   pcScaleMenu->AddItem( "3x", new Message( ID_VIEW_SCALE_3 ) );
   pcScaleMenu->AddItem( "4x", new Message( ID_VIEW_SCALE_4 ) );
   pcScaleMenu->AddItem( "8x", new Message( ID_VIEW_SCALE_8 ) );

   pcViewMenu->AddItem( pcScaleMenu );   
   
   pcMenuBar->AddItem( pcViewMenu );
   
   Menu* pcHelpMenu = new Menu( Rect(0,0,1,1), "Help", ITEMS_IN_COLUMN );
   pcHelpMenu->AddItem( "About", new Message( ID_HELP_ABOUT ) );
   
   pcMenuBar->AddItem( pcHelpMenu );
   pcMenuBar->SetTargetForItems( this );
   
   // Add the menubar to the window.
   AddChild( pcMenuBar );
   
   Rect cWindowBounds = GetBounds();
   cWindowBounds.left = 0;
   cWindowBounds.top = MENU_OFFSET + 4;
   
   // Add the zoom component to the window.
   m_pcZoom = new ZoomView( cWindowBounds );
   AddChild( m_pcZoom );
}

MyWindow::~MyWindow()
{
    delete m_pcZoom;
}

bool MyWindow::OkToQuit( void )
{
    Application::GetInstance()->PostMessage( M_QUIT );
    return( false );
}

void MyWindow::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() ) // Get the custom message code.
    {
      case ID_EXIT:
    OkToQuit();
    break;
      case ID_VIEW_SCALE_2:
        m_pcZoom->setMagnification(2);
        break;
      case ID_VIEW_SCALE_3:
        m_pcZoom->setMagnification(3);
        break;
      case ID_VIEW_SCALE_4:
        m_pcZoom->setMagnification(4);
        break;
      case ID_VIEW_SCALE_8:
        m_pcZoom->setMagnification(8);
        break;
      case ID_HELP_ABOUT:
    {
         Window* pcAboutWindow = new Window( Rect(325,225,500,325),"about","About BZoom", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_H_RESIZABLE | WND_NOT_V_RESIZABLE );
         Rect pcAboutBounds = pcAboutWindow->GetBounds();

         TextView* pcAboutText = new TextView(pcAboutBounds,"about_text", "\t    BZoom V1.0\n\nA simple pixel zoomer for AtheOS.\nBased on AZoom by Chir.\nBZoom is released under the\nGNU GPL", CF_FOLLOW_LEFT, WID_WILL_DRAW);
         pcAboutText->SetReadOnly( true );
         pcAboutText->SetMultiLine( true );

         pcAboutWindow->AddChild( pcAboutText, false );

         pcAboutWindow->Show();
         pcAboutWindow->MakeFocus();
    }
    break;
      default:
        Window::HandleMessage( pcMessage );
    }
}

// ---------------------------------------------------------------------------

MyApp::MyApp() : Application( "application/x-VND.KHS-digitaliz-Zoom" )
{
    m_pcMainWindow = new MyWindow( Rect(10,20,150,150) );
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

