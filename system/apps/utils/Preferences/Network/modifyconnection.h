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

#ifndef __MODIFYCONNECTION_H_
#define __MODIFYCONNECTION_H_

#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/textview.h>
#include <gui/checkbox.h>
#include <gui/stringview.h>
#include <gui/radiobutton.h>
#include <gui/button.h>
#include "configuration.h"

extern bool bModifyOpen[C_CO_MAXADAPTORS];

class ModifyConnectionWindow : public os::Window {
public:
  ModifyConnectionWindow(const os::Rect& cFrame, AdaptorConfiguration *pcAdaptorIn, int iAdaptorIDIn);
  ~ModifyConnectionWindow();
  void ShowData();
  void SaveValues();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);

private:
  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot;
  os::FrameView *pcFVSettings;
  os::VLayoutNode *pcVLSettings;
  os::HLayoutNode *pcHLDesc, *pcHLIP, *pcHLSN, *pcHLGW, *pcHLButtons;
  os::TextView *pcTVIPAddr,*pcTVSubNet, *pcTVGateway;
  os::StringView *pcSVAdaptor;
  os::CheckBox *pcCBDhcp;
  os::Button *pcBEnable, *pcBDisable;

  AdaptorConfiguration *pcAdaptor;
  int iAdaptorID;
};

#endif /* __MODIFYCONNECTION_H_ */





