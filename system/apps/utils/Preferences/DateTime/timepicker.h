// Time Picker Control :: (C)opyright 2002 Daryl Dudey
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

#ifndef __TIMEPICKER_H_
#define __TIMEPICKER_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/spinner.h>
#include <util/looper.h>

namespace os {
#if 0
}
#endif

class Clock : public os::View {
public:
  Clock(const os::Rect cFrame, const char *pszname);
  virtual void AttachedToWindow();
  virtual void Paint(const os::Rect &cUpdateRect);
  virtual os::Point GetPreferredSize(bool bLargest) const;
  void SetTime(int iHour, int iMin, int iSec);
private:
  void DrawCircleLine(float fAngle, float fCenterX, float fCenterY, float fStart, float fEnd);
  void DrawCircleFrame(float fAngle, float fCenterX, float fCenterY, float fPosition, float fSize);
  int m_iHour, m_iMin, m_iSec;
};

class TimePicker : public os::View {
public:
  TimePicker(const os::Rect& cFrame, const char *szName);
  ~TimePicker();
  virtual void HandleMessage(os::Message* pcMessage);
  virtual void AttachedToWindow();
  virtual void Paint(const os::Rect &cUpdateRect);
  virtual os::Point GetPreferredSize(bool bLargest) const;

  void GetTime(int *iHours, int *iMinutes, int *iSeconds);
private:
  void TimerTick(int nId);
  int m_iHour, m_iMin, m_iSec;
  int m_iHourDiff, m_iMinDiff, m_iSecDiff;

  void SecondsChanged();
  void MinutesChanged();
  void HoursChanged();

  os::LayoutView *m_pcLRoot;
  os::VLayoutNode *m_pcVLRoot;
  os::HLayoutNode *m_pcHLCurrentTime;
  os::Spinner *m_pcSPHour, *m_pcSPMin, *m_pcSPSec;
  os::Looper *m_pcLooper;
  os::Clock *m_pcClock;

  enum Messages {
    M_HOURCHANGED,
    M_MINCHANGED,
    M_SECCHANGED
  };
};

} // End namespace

#endif /* __TIMEPICKER_H_ */
