// Screen Preferences :: (C)opyright Daryl Dudey 2002
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
#include <vector.h>

// This is the number of depths, I am only using RGB ones
const int MAX_DEPTH_COUNT = 7;
const int MAX_RES_COUNT = 40;

class MainWindow : public os::Window {
public:
  MainWindow(const os::Rect& cFrame);
  ~MainWindow();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);

private:
  void ShowData();
  void Apply();
  void Undo();
  void Default();
  void ColourChange();
  void ResolutionChange();
  void RefreshChange();

  // Default mode
  os::screen_mode cCurrent;
  os::screen_mode cUndo;
  os::screen_mode cUndo2;

  // All screenmodes
  int m_nModeCount;
  os::screen_mode* m_pcModes;
  std::vector<uint32> m_cResolutions;
  std::vector<uint32> m_cColorSpaces;
  std::vector<uint32> m_cRefreshRates;

  // Refresh flag/custom or not

  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot, *pcVLSettings;
  os::HLayoutNode *pcHLWorkspace, *pcHLButtons, *pcHLResolution, *pcHLColour, *pcHLRefresh;
  os::FrameView *pcFVSettings, *pcFVAppearance;
  os::Button *pcBDefault, *pcBApply, *pcBUndo;
  os::DropdownMenu *pcDDMWorkspace, *pcDDMResolution, *pcDDMColour, *pcDDMRefresh;
};

#endif /* __MAINWINDOW_H_ */











