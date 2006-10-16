// Screen Preferences :: (C)opyright 2000-2001 Daryl Dudey
//                       (C)opyright Jonas Jarvoll 2005
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

bool bRoot;

int main(int argc, char* argv[]) 
{
  PrefsScreenApp *pcPrefsScreenApp;

  bRoot = ( getuid()==0 );

  pcPrefsScreenApp=new PrefsScreenApp();

  // Give a warning if the user is not root
  if(!bRoot)
  {
	std::string cRootOnlyText="Only root is allowed to change the screen resolution.\n";
	os::Alert* pcRootOnlyAlert=new os::Alert("Not allowed", cRootOnlyText, os::Alert::ALERT_INFO, "OK", NULL);
  	pcRootOnlyAlert->Go(new os::Invoker());
  }

  pcPrefsScreenApp->Run();
}

PrefsScreenApp::PrefsScreenApp() : os::Application("application/x-vnd-ScreenPreferences")
{
  SetCatalog("Screen.catalog");

  // Show main window
  pcWindow = new MainWindow(os::Rect());
  pcWindow->CenterInScreen();
  pcWindow->Show();
  pcWindow->MakeFocus();
}

PrefsScreenApp::~PrefsScreenApp()
{
}












