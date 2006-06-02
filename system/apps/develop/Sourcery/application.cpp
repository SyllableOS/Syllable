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

#include "application.h"
#include "mainwindow.h"
#include <util/string.h>
#include <storage/registrar.h>

using namespace os;

/*************************************************************
* Description: App Constructor
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:36:29 
*************************************************************/
App::App(int argc, char** argv) : os::Application( "application/x-vnd.syllable-Sourcery" )
{
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Sourcery";
	mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	zPath+="/Backups";
	mkdir(zPath.c_str(),S_IRWXU | S_IRWXG | S_IRWXO);
	
	pcSettings = new AppSettings();
	pcSettings->LoadSettings();
	if (pcSettings->GetSplashScreen())
	{
		SplashWindow* pcSplashWindow = new SplashWindow();		
		pcSplashWindow->CenterInScreen();
		pcSplashWindow->Show();
		pcSplashWindow->MakeFocus();
		snooze(3000000);
		pcSplashWindow->Hide();	
	}

	if (argc == 1)
	{
		pcMainWindow = new MainWindow(pcSettings,"");
	}
	else if (argc > 1)
	{	
		pcMainWindow = new MainWindow(pcSettings,argv[1]);
	}
	else
	{
		dbprintf("Sourcery crashed abnormally, help!!!\n");
		return;
	}
	if (pcSettings->GetSizeAndPositionOption() && pcSettings->GetSizeAndPosition() != Rect(-1,-1,-1,-1))
	{	
		pcMainWindow->ResizeTo(pcSettings->GetSizeAndPosition().Width(),pcSettings->GetSizeAndPosition().Height());
		pcMainWindow->MoveTo(pcSettings->GetSizeAndPosition().left, pcSettings->GetSizeAndPosition().top);
	}		
	pcMainWindow->Show();
	pcMainWindow->MakeFocus();
			
	
}

App::~App()
{
	
	delete pcSettings;
}

bool App::OkToQuit()
{
	pcMainWindow->PostMessage( os::M_QUIT, pcMainWindow );
	return( false );
}

void App::HandleMessage( os::Message* pcMsg )
{
	os::Application::HandleMessage( pcMsg );
}































