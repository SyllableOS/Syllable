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

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/spinner.h>
#include <gui/dropdownmenu.h>
#include <gui/tabview.h>
#include <gui/imageview.h>
#include <gui/listview.h>
#include "datepicker.h"
#include "timepicker.h"

class CMainWindow : public os::Window {
public:
  CMainWindow(const os::Rect& cFrame);
  ~CMainWindow();
  virtual void HandleMessage(os::Message* pcMessage);
  bool OkToQuit(void);

private:
  void TimerTick(int nID);
  void GetLine( os::StreamableIO* pcIO, char* zBuffer, int nMaxLen, char zEnd );
  void AddTimeZones();
  void UpdateTimeZoneMap();
  void Apply();

  std::string **m_pcstrShort;
  std::string **m_pcstrFile;
  int *m_piDiff;

  os::BitmapImage *m_pcBIImage, *m_pcBIImageSaved;
  os::LayoutView *m_pcLRoot, *m_pcLTimeDate, *m_pcLTimeZone;
  os::VLayoutNode *m_pcVLRoot, *m_pcVLTimeDate, *m_pcVLTimeZone;
  os::HLayoutNode *m_pcHLTimeDate, *m_pcHLTime, *m_pcHLDate, *m_pcHLTimeZoneMap, *m_pcHLTimeZone, *m_pcHLButtons;
  os::TabView *m_pcTVRoot;
  os::FrameView *m_pcFVTime, *m_pcFVDate;
  os::Button *m_pcBApply;
  os::DropdownMenu *m_pcDDMTimeZone;
  os::DatePicker *m_pcDPDate;
  os::TimePicker *m_pcTPTime;
  os::StringView *m_pcSVFullTimeDate;
  os::ImageView *m_pcIVTimeZone;
};

#endif /* __MAINWINDOW_H_ */













