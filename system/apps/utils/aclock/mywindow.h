/*  mywindow.h - the main window for the AtheOS Clock v0.3
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
#include <gui/menu.h>
#include <util/message.h>
#include <gui/button.h>
#include "gui/clockview.h"
#include <util/settings.h>

using namespace os;



const unsigned int MENU_OFFSET = 15;
#define APP_NAME "Clock"
#define M_MESSAGE_PASSED 1001

/** Container for the component */
class MyWindow : public Window
{
  public:
    MyWindow( const Rect& cFrame);
    ~MyWindow();
    void HandleMessage( Message* pcMessage );   // virtual.
    bool OkToQuit( void );                      // virtual.
  private:
   
    enum mymenus { ID_FILE_EXIT,
                   ID_VIEW_SECONDS, ID_VIEW_SECONDS_ON, ID_VIEW_SECONDS_OFF,
                   ID_VIEW_DIGITAL, ID_VIEW_DIGITAL_ON, ID_VIEW_DIGITAL_OFF,
                   ID_HELP_ABOUT, ID_WHITE, ID_BLACK };
   
    ClockView* m_pcDisplay;                     // The embedded visual component.
    Color32_s sColor;
    bool bDigital;
    bool bSec;
    Settings* settings;
};

class MyApp : public Application
{
  public:
    MyApp();
    ~MyApp();
    
  private:
  	bool LoadSettings();
  	void LoadDefaults();
  	bool StoreSettings();
    MyWindow* m_pcMainWindow;                   // The frame window.
    Settings* settings;
    Color32_s sColor;
    bool bDigital;
    bool bSec;
    Message* pcMsg;
};





