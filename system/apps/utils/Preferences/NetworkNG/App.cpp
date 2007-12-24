// Syllable Network Preferences - Copyright 2006 Andrew Kennan
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

#include <gui/requesters.h>
#include <util/catalog.h>

#include "App.h"
#include "MainWin.h"
#include "Interface.h"
#include "Strings.h"

App::App( Action nAction ):Application( "application/x-vnd-syllable-NetPrefs" )
{
	SetCatalog( "Network.catalog" );

	switch ( nAction )
	{

	case Normal:
		{
			// Normal operation - display main window.
			MainWin *pcWin = new MainWin();

			pcWin->CenterInScreen();
			pcWin->Show();
			pcWin->MakeFocus();
			break;
		}

	case Detect:
		DetectChanges();
		break;
	}
}

App::~App()
{
}

void App::DetectChanges( void )
{
	InterfaceManager cIM;
	InterfaceList_t cConfig;
	InterfaceList_t cDetect;

	// Load interfaces from config. 
	cIM.LoadConfig( cConfig );
	// Ask system about interfaces.
	cIM.DiscoverInterfaces( cDetect );

	// Do they match?
	if( cIM.InterfacesHaveChanged( cConfig, cDetect ) )
	{
		// No. Display an alert and then exit.
		Alert *pcAlert = new Alert( MSG_APP_DETECT_ALERT_TITLE, MSG_APP_DETECT_ALERT_MESSAGE, Alert::ALERT_INFO, 0, MSG_APP_DETECT_ALERT_CLOSE.c_str(), NULL );

		pcAlert->Go( new Invoker( new Message( M_QUIT ) ) );
	}
	else
	{
		// Yes. Exit the program.
		printf( MSG_APP_DETECT_NO_CHANGE.c_str() );
		Application::GetInstance()->PostMessage( M_QUIT );
	}
}
