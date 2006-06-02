//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <util/application.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/event.h>

#include "splash.h"
#include "mainwindow.h"
#include "settings.h"

using namespace os;


class App : public Application
{
public:
	App(int argc, char** argv);
	~App();
	bool OkToQuit();
	void HandleMessage( os::Message* pcMsg );
private:
	String cFileName;
	MainWindow* pcMainWindow;
	AppSettings* pcSettings;
	
};

#endif //APPLICATION_H




