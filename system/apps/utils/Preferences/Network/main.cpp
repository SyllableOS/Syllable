// Network Preferences :: (C)opyright 2000-2001 Daryl Dudey
//
// 2 September 2003, Kaj de Vos
//   Don't execute net_init.sh when called with --detect.
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
#include "configuration.h"

Configuration *pcConfig;
bool bRoot;
bool bDetectMsg = false;

int main(int argc, char* argv[]) 
{
  PrefsNetApp* pcPrefsNetApp;
  bool bRunApp = true;
  bool bActivate = false;

  if ((getuid()==0)) {
    bRoot = true;
  } else {
    bRoot = false;
  }

  // Create global configuration variable and load the values from file
  pcConfig = new Configuration();
  pcConfig->Load();

  // First off, see if any command-line parameters
  if (argc==2) {
    if (strcmp(argv[1], "--detect")==0) {
      
      if (pcConfig->DetectInterfaceChanges()) {
        fprintf(stdout, "Detected changes in network interfaces, launching\n");
        bDetectMsg = true;
      } else {
	fprintf(stdout, "No changes detected in network interfaces\n");
	bRunApp = false;
	bActivate = false;
      }

    } else if (strcmp(argv[1], "--activate")==0) {
      
      fprintf(stdout, "Creating network interface activation files\n");
      // Don't show gui, just activate on exit
      bRunApp = false;
      bActivate = true;
    }
  }
  
  // Run app
  if (bRunApp) {
    pcPrefsNetApp=new PrefsNetApp();
    pcPrefsNetApp->Run();
  }

  // Create activate files resolv.conf etc if needed
  if (bActivate) {
    pcConfig->Activate();

    // Now execute /system/net_init.sh
    system("/system/net_init.sh");
  }

  // Delete configuration
  delete pcConfig;

  return(0);
}

PrefsNetApp::PrefsNetApp() : os::Application("application/x-vnd-NetworkPreferences")
{
  // Get screen width and height to centre the window
  os::Desktop *pcDesktop = new os::Desktop();
  int iWidth = pcDesktop->GetScreenMode().m_nWidth;
  int iHeight = pcDesktop->GetScreenMode().m_nHeight;

  // See if we want the full window or just the new network interfaces window
  if (!bDetectMsg) {
    int iLeft = (iWidth-300)/2;
    int iTop = (iHeight-260)/2;

    // Now show the main window
    if (bRoot) {
      pcWindow = new MainWindow(os::Rect(iLeft, iTop, iLeft+300, iTop+260));
    } else {
      pcWindow = new MainWindow(os::Rect(iLeft, iTop, iLeft+300, iTop+230));
    }
    pcWindow->Show();
    pcWindow->MakeFocus();
  } else {
    int iLeft = (iWidth-350)/2;
    int iTop = ((iHeight-75)/2)+200;

    // Now show detect message window
    pcMsgWindow = new DetectMsgWindow(os::Rect(iLeft, iTop, iLeft+350, iTop+75));
    pcMsgWindow->Show();
    pcMsgWindow->MakeFocus();
  }
}

PrefsNetApp::~PrefsNetApp()
{
}










