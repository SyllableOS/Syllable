// Date Picker Control :: (C)opyright 2002 Daryl Dudey
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

#ifndef __DATEPICKER_H_
#define __DATEPICKER_H_

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/spinner.h>
#include <gui/dropdownmenu.h>

// Make sure in correct namespace
namespace os {
#if 0
}
#endif

class DatePickerDay : public os::View, public os::Invoker {
public:
  DatePickerDay(const os::Rect cFrame, const char *pszname, const char *pszString, bool bColourBackground, bool bHeader,  
		os::Message *pcMessage);
  virtual void AttachedToWindow();
  virtual void Paint(const os::Rect &cUpdateRect);
  virtual os::Point GetPreferredSize(bool bLargest) const;
  virtual void MouseDown(const os::Point &cPosition, uint32 iButtons);

  void SetString(const char *pszString);
  void SetSelected(bool bSelected);
private:
  std::string m_strValue;
  bool m_bColourBackground;
  float m_fWidth;
  float m_fHeight;
  float m_fAscender;
  bool m_bSelected;
  int m_iCode;
  bool m_bHeader;
};

class DatePicker : public os::View {
public:
  DatePicker(const os::Rect& cFrame, const char *pszName);
  virtual void Paint(const os::Rect &cUpdateRect);
  virtual void FrameSized(const os::Point &cDelta);
  virtual void HandleMessage(os::Message *pcMessage);
  virtual void AttachedToWindow();
  virtual os::Point GetPreferredSize(bool bLargest) const;

  void SetDate(int iDay, int iMonth, int iYear);
  void GetDate(int *iDay, int *m_iMonth, int *m_iYear, std::string *strFormatted);
private:
  int m_iDay, m_iMonth, m_iYear, m_iDayOfWeek;

  void ReDraw();
  void DayChanged(os::Message *pcMessage);
  void MonthChanged();
  void YearChanged();

  os::LayoutView *m_pcLRoot;
  os::VLayoutNode *m_pcVLRoot;
  os::HLayoutNode *m_pcHLCurrentDate, *m_pcHLDateRows[7];
  os::DatePickerDay *m_pcSVDateNumbers[43], *m_pcSVDays[8];
  os::Spinner *m_pcSPYear;
  os::DropdownMenu *m_pcDDMMonth;
  
  enum Messages {
    M_DAYCHANGED,
    M_MONTHCHANGED,
    M_YEARCHANGED
  };
};

} // End of namespace

#endif /* __DATEPICKER_H_ */
