// Appearance Preferences :: (C)opyright Daryl Dudey 2002
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
#include <gui/listview.h>
#include <gui/slider.h>
#include <gui/textview.h>
#include <gui/gfxtypes.h>

class ColourPreview : public os::View {
public:
  ColourPreview( const os::Rect &cFrame, const char *pzName );
  
  void SetValue(int iRed, int iGreen, int iBlue);

  virtual void Paint( const os::Rect &cUpdateRec );
  virtual void FrameSized( const os::Point &cDelta ); 
private:
  os::Color32_s cColour;
};

class MainWindow : public os::Window {
public:
  MainWindow(const os::Rect& cFrame);
  ~MainWindow();
  virtual void HandleMessage(os::Message* pcMessage);

  bool OkToQuit(void);

private:
  void ShowData();
  void Apply();
  void Default();
  void ColourChangePreview();
  void LoadPen();
  void Load();
  void Save();
  void Delete();

  os::Color32_s ePens[os::COL_COUNT];

  os::LayoutView *pcLRoot;
  os::VLayoutNode *pcVLRoot, *pcVLAppearance, *pcVLSlider, *pcVLColour, *pcVLPreview;
  os::HLayoutNode *pcHLButtons, *pcHLDecor, *pcHLTheme, *pcHLHeader, *pcHLRed, *pcHLGreen, *pcHLBlue, *pcHLCustSpacer, *pcHLCustomise, *pcHLSchemeButtons;
  os::FrameView *pcFVAppearance, *pcFVColour, *pcFVCustomise;
  os::Button *pcBSave, *pcBDelete, *pcBDefault, *pcBApply;
  os::DropdownMenu *pcDDMDecor, *pcDDMTheme, *pcDDMItem;
  os::Slider *pcSLRed, *pcSLGreen, *pcSLBlue;
  os::TextView *pcTVRed, *pcTVGreen, *pcTVBlue;
  os::TextView *pcTVRedHex, *pcTVGreenHex, *pcTVBlueHex;
  ColourPreview *pcCPPreview;
};

#endif /* __MAINWINDOW_H_ */











