/*
    Launcher - A program launcher for AtheOS
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <util/application.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/desktop.h>

#include <storage/file.h>
#include <storage/fsnode.h>

#include "launcher_window.h"

#include <vector>

#define PREFS_DIR  "~/config/Launcher"
#define PREFS_FILE "~/config/Launcher/Launcher1.cfg"

class Launcher : public Application
{

    std::vector<LauncherWindow *>m_apcWindow;

  public:
    Launcher( );
    ~Launcher( );
    bool OkToQuit( void );
    void HandleMessage( Message *pcMessage );
    
  private:
  	void PositionWindow( void );
	void SavePrefs( void );
};


