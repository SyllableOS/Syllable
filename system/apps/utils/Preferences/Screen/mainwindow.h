// Screen Preferences :: (C)opyright Daryl Dudey 2002
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

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <gui/slider.h>
#include <gui/requesters.h>
#include <vector>

// This is the number of depths, I am only using RGB ones
const int MAX_DEPTH_COUNT = 7;
const int MAX_RES_COUNT = 40;

class MainWindow : public os::Window {
public:
  MainWindow(const os::Rect& cFrame);
  ~MainWindow();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);
  void TimerTick( int nID );

private:
  void ShowData();
  void ColourChange();
  void ResolutionChange();
  void RefreshChange();
  void Apply();
  void EnableAll();
  void DisableAll();
  void SaveOldMode();
  void RestoreOldMode();

  // Number of supported desktops
  static const uint32 NoSupportedDesktop=32;

  // Timer
  static const int TimerId=0;
  static const int TimerCount=15L*1000L*1000L;  // In micro seconds

  // Saved mode
  os::screen_mode cCurrentMode;
  os::screen_mode cOldMode;

  // All screenmodes
  int m_nModeCount;
  os::screen_mode* m_pcModes;
  std::vector<uint32> m_cResolutions;
  std::vector<uint32> m_cColorSpaces;
  std::vector<uint32> m_cRefreshRates;

  // GUI components
  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot, *pcVLSettings;
  os::HLayoutNode *pcHLWorkspace, *pcHLButtons, *pcHLResolution, *pcHLColour, *pcHLRefresh;
  os::FrameView *pcFVSettings, *pcFVAppearance;
  os::Button *pcBApply, *pcBClose;
  os::DropdownMenu *pcDDMWorkspace, *pcDDMResolution, *pcDDMColour, *pcDDMRefresh;
  os::Alert* pcConfirmAlert;
};

#endif /* __MAINWINDOW_H_ */











