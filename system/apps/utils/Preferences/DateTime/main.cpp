// Date & Time Preferences :: (C)opyright 2002 Daryl Dudey
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

bool g_bRoot;

int main(int argc, char* argv[]) 
{
  CDateTimeApp *pcDateTimeApp;

  // See if user is root
  g_bRoot = (getuid()==0);

  pcDateTimeApp=new CDateTimeApp();
  pcDateTimeApp->Run();
}

CDateTimeApp::CDateTimeApp() : os::Application("application/x-vnd.Preferences-DateTime")
{
  SetCatalog("DateTime.catalog");

  // Get screen width and height to centre the window
  os::Desktop *pcDesktop = new os::Desktop();
  int iWidth = pcDesktop->GetScreenMode().m_nWidth;
  int iHeight = pcDesktop->GetScreenMode().m_nHeight;
  int iLeft = (iWidth-340)/2;
  int iTop = (iHeight-260)/2;

  // Show main window
  if (g_bRoot) {
    m_pcWindow = new CMainWindow(os::Rect(iLeft, iTop, iLeft+350, iTop+260));
  } else {
    m_pcWindow = new CMainWindow(os::Rect(iLeft, iTop, iLeft+350, iTop+260));
  }
  m_pcWindow->Show();
  m_pcWindow->MakeFocus();
}











