/*  mywindow.h - the main window for the AtheOS Zoom v0.1
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


#include <util/application.h>
#include <gui/window.h>
#include <gui/textview.h>
#include <util/message.h>
#include <gui/menu.h>
#include "gui/zoomview.h"


using namespace os;



const unsigned int MENU_OFFSET = 15;


/**
 * Container for the components.
 */
class MyWindow : public Window
{
  public:
    MyWindow( const Rect& cFrame );
    virtual ~MyWindow();
    virtual void HandleMessage( Message* pcMessage );
    virtual bool OkToQuit();
    
  private:

    enum mymenus { ID_EXIT,
               ID_VIEW_SCALE, ID_VIEW_SCALE_2, ID_VIEW_SCALE_3, ID_VIEW_SCALE_4, ID_VIEW_SCALE_8,
               ID_HELP_ABOUT };
   
    ZoomView* m_pcZoom;                                   // The embedded visual component.
};


/**
 * Main class.
 */
class MyApp : public Application
{
  public:
    MyApp();
    virtual ~MyApp();
  private:
    MyWindow* m_pcMainWindow;
};

