// Network Preferences :: (C)opyright 2002 Daryl Dudey
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

#include <gui/stringview.h>
#include <gui/button.h>
#include <util/application.h>
#include <util/message.h>
#include "errorwindow.h"
#include "messages.h"
#include "main.h"

ErrorWindow::ErrorWindow(const char *pzError, const os::Rect& cFrame) : os::Window(cFrame, "DetectMsgWindow", "Invalid data - Network", os::WND_NOT_RESIZABLE | os::WND_MODAL | os::WND_NO_CLOSE_BUT | os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,10,10,10) );

  // Add the message text
  pcVLRoot->AddChild( new os::StringView(cRect, "", pzError) );
  pcVLRoot->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f) );
  pcVLRoot->AddChild( new os::Button(cRect, "", "OK", new os::Message(M_EW_OK)) );

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  bErrorWindowOpen = true;
}

ErrorWindow::~ErrorWindow()
{
  bErrorWindowOpen = false;
}

void ErrorWindow::HandleMessage(os::Message* pcMessage)
{

  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_EW_OK:
    PostMessage(os::M_QUIT);
    break;
    
  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool ErrorWindow::OkToQuit()
{
  //  PostMessage(os::M_QUIT);
  return true;
}



