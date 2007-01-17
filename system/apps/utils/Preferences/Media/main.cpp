
/*  Syllable Media Preferences
 *  Copyright (C) 2003 Arno Klenke
 *  Based on the preferences code by Daryl Dudey
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#include <unistd.h>
#include <iostream>
#include <gui/desktop.h>
#include <gui/requesters.h>
#include "resources/Media.h"
#include "main.h"

using namespace os;

int main( int argc, char *argv[] )
{
	PrefsMediaApp *pcPrefsMediaApp;

	pcPrefsMediaApp = new PrefsMediaApp();
	pcPrefsMediaApp->Run();
}

PrefsMediaApp::PrefsMediaApp():os::Application( "application/x-vnd-MediaPreferences" )
{
	SetCatalog("Media.catalog");

	// Get screen width and height to centre the window
	os::Desktop * pcDesktop = new os::Desktop();
	
	// Create media manager 
	m_pcManager = new os::MediaManager();

	if ( !m_pcManager->IsValid() )
	{
		std::cout << "Media server not running" << std::endl;

		os::Alert* pcAlert = new os::Alert( MSG_ALERTWND_NOMEDIASERVER_TITLE, MSG_ALERTWND_NOMEDIASERVER_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_NOMEDIASERVER_OK.c_str(),NULL);
		pcAlert->Go();
		pcAlert->MakeFocus();

		PostMessage( os::M_QUIT );
		return;
	}

	// Show main window
	pcWindow = new MainWindow( os::Rect( 0, 0, 600, 350 ) );
	pcWindow->CenterInScreen();
	pcWindow->Show();
	pcWindow->MakeFocus();
}

bool PrefsMediaApp::OkToQuit()
{
	return ( true );
}

PrefsMediaApp::~PrefsMediaApp()
{
	m_pcManager->Put();
}

