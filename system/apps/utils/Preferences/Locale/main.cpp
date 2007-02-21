// Locale Preferences :: (C)opyright 2004 Henrik Isaksson
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

#include <unistd.h>
#include <gui/desktop.h>
#include "main.h"

int main( int argc, char *argv[] )
{
	PrefsLocaleApp *pcPrefsLocaleApp;

	pcPrefsLocaleApp = new PrefsLocaleApp();
	pcPrefsLocaleApp->Run();

	return ( 0 );
}

PrefsLocaleApp::PrefsLocaleApp()
:Application( "application/x-vnd-LocalePreferences" )
{
	// Load catalog
	SetCatalog( "Locale.catalog" );
	
	// Now show main window
	pcWindow = new MainWindow();
	pcWindow->Show();
	pcWindow->MakeFocus();
}

bool PrefsLocaleApp::OkToQuit()
{
	return true;
}


PrefsLocaleApp::~PrefsLocaleApp()
{
}


