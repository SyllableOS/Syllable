// Font Preferences :: (C)opyright Daryl Dudey 2002
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
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/spinner.h>
#include <gui/font.h>
#include <gui/checkbox.h>

class MainWindow : public os::Window {
public:
  MainWindow(const os::Rect& cFrame);
  ~MainWindow();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);

private:
  void GetCurrentValues();
  void ShowData();
  void Apply();
  void Revert();
  void Default();
  void UpdateExample(const char *pzFont = "", float fSize = 0.0);
  bool DecodeFamilyStyle(const char *, float, os::font_properties *);

  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot, *pcVLTypes;
  os::HLayoutNode *pcHLNormal, *pcHLBold, *pcHLFixed, *pcHLWindow, *pcHLTWindow, *pcHLExample, *pcHLButtons;
  os::FrameView *pcFVTypes, *pcFVExample;
  os::DropdownMenu *pcDDMNormal, *pcDDMBold, *pcDDMFixed, *pcDDMWindow, *pcDDMTWindow;
  os::Button *pcBApply, *pcBRevert, *pcBDefault;
  os::Spinner *pcSPNormal, *pcSPBold, *pcSPFixed, *pcSPWindow, *pcSPTWindow;
  os::StringView *pcSVExample;
  os::CheckBox *pcCBAntiAliasing;

  bool m_bAntiAliasing;

  std::string cNorDefFam, cNorDefSty;
  std::string cBldDefFam, cBldDefSty;
  std::string cFxdDefFam, cFxdDefSty;
  std::string cWndDefFam, cWndDefSty;
  std::string cTWdDefFam, cTWdDefSty;
  float cNorDefSiz, cBldDefSiz, cFxdDefSiz, cWndDefSiz, cTWdDefSiz;
};

#endif /* __MAINWINDOW_H_ */
