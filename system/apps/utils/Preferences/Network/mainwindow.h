// Network Preferences :: (C)opyright Daryl Dudey 2002
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

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/listview.h>
#include <gui/button.h>
#include <gui/stringview.h>

#include "modifyconnection.h"

class MainWindow : public os::Window {
public:
  MainWindow(const os::Rect& cFrame);
  ~MainWindow();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);

private:
  void ShowData();
  void ShowList();
  void Apply();

  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot;
  os::HLayoutNode *pcHLHost, *pcHLDomain, *pcHLName1, *pcHLName2, *pcHLConnection, *pcHLButtons;
  os::VLayoutNode *pcVLNames, *pcVLConnectionList, *pcVLConnectionButtons;
  os::FrameView *pcFVNames, *pcFVConnections;
  os::ListView *pcLVConnections;
  os::TextView *pcTVHost, *pcTVDomain, *pcTVName1, *pcTVName2;
  os::Button *pcBModify, *pcBApply, *pcBRevert;
  
  ModifyConnectionWindow *pcModifyWindow[C_CO_MAXADAPTORS];

};

#endif /* __MAINWINDOW_H_ */











