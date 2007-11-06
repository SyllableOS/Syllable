/*
 *  
 *  Copyright (C) 2001 Henrik Isaksson
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

#include <gui/window.h>
#include <gui/view.h>
#include <gui/rect.h>
#include <gui/desktop.h>
#include <util/message.h>
#include <util/application.h>

using namespace os;

#include "mainwin.h"
#include "app.h"

#include "resources/SettingsEditor.h"

class MyApp:public Application
{
	public:
	MyApp();
	~MyApp();

	private:
	Window *win;
};

MyApp::MyApp()
	:Application(APP_ID)
{
	SetCatalog("SettingsEditor.catalog");
	
	win = new MainWin(Rect(0,0,340, 250), NULL, NULL, "");
	win->CenterInScreen();
	win->Show();
	win->MakeFocus();
}

MyApp::~MyApp()
{
}

int main()
{
	MyApp *ThisApp;

	ThisApp = new MyApp;
	ThisApp->Run();

	return 0;
}
