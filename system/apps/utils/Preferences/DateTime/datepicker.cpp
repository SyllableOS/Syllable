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

#include <stdio.h>
#include <util/application.h>
#include <util/message.h>
#include <time.h>
#include <string>
#include "datepicker.h"
#include "resources/DateTime.h"

using namespace os;

DatePicker::DatePicker(const os::Rect& cFrame, const char *pszName) : 
  os::View(cFrame, pszName)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  m_pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  m_pcVLRoot = new os::VLayoutNode("VL");

  // Current date
  m_pcHLCurrentDate = new os::HLayoutNode("", 1.0f, m_pcVLRoot);

  // Month
  m_pcHLCurrentDate->AddChild(m_pcDDMMonth = new os::DropdownMenu(cRect, "DDMMonth"), 1.0f);
  m_pcDDMMonth->SetMinPreferredSize( 2 );
  m_pcDDMMonth->SetSelectionMessage(new os::Message(M_MONTHCHANGED));
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_JANUARY);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_FEBRUARY);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_MARCH);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_APRIL);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_MAY);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_JUNE);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_JULY);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_AUGUST);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_SEPTEMBER);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_OCTOBER);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_NOVEMBER);
  m_pcDDMMonth->AppendItem(MSG_DAYMONTH_MONTH_DECEMBER);
  m_pcHLCurrentDate->AddChild(new os::HLayoutSpacer("", 5.0f, 5.0f));

  // Year
  m_pcHLCurrentDate->AddChild(m_pcSPYear = new os::Spinner(cRect, "SPYear", 0.0f, new os::Message(M_YEARCHANGED)), 1.0f);
  m_pcSPYear->SetMinMax(1900.0f, 2100.0f); // Sure this should futureproof for a while :]
  m_pcSPYear->SetStep(1.0f);
  m_pcSPYear->SetMinPreferredSize(5);
  m_pcSPYear->SetMaxPreferredSize(5);
  m_pcSPYear->SetFormat("%0.f");

  // Create grid, spacing first
  m_pcVLRoot->AddChild(new os::VLayoutSpacer("", 10.0f, 10.0f));
  
  // Create row with days for header
  m_pcHLDateRows[0] = new os::HLayoutNode("", 1.0f, m_pcVLRoot);
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[0] = new os::DatePickerDay(cRect, "SVSu", MSG_DAYMONTH_SHORTDAY_SUNDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[1] = new os::DatePickerDay(cRect, "SVMo", MSG_DAYMONTH_SHORTDAY_MONDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[2] = new os::DatePickerDay(cRect, "SVTu", MSG_DAYMONTH_SHORTDAY_TUESDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[3] = new os::DatePickerDay(cRect, "SVWe", MSG_DAYMONTH_SHORTDAY_WEDNESDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[4] = new os::DatePickerDay(cRect, "SVTh", MSG_DAYMONTH_SHORTDAY_THURSDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[5] = new os::DatePickerDay(cRect, "SVFr", MSG_DAYMONTH_SHORTDAY_FRIDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcHLDateRows[0]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
  m_pcHLDateRows[0]->AddChild(m_pcSVDays[6] = new os::DatePickerDay(cRect, "SVSa", MSG_DAYMONTH_SHORTDAY_SATURDAY.c_str(), false, true, new os::Message(M_DAYCHANGED)));
  m_pcVLRoot->AddChild(new os::VLayoutSpacer("", 2.0f, 2.0f));

  // Fill in 6 rows with numbers
  int k = 0;
  for (int i = 1; i < 7; i++) {
    m_pcHLDateRows[i] = new os::HLayoutNode("", 1.0f, m_pcVLRoot);
    for (int j = 0; j < 7; j++) {
      char psScratch[8];
      sprintf(psScratch, "SVDay%d", k);
      m_pcSVDateNumbers[k] = new os::DatePickerDay(cRect, psScratch, "", true, false, new os::Message(M_DAYCHANGED));
      m_pcHLDateRows[i]->AddChild(m_pcSVDateNumbers[k]);
      k++;
      if (j != 6) {
	m_pcHLDateRows[i]->AddChild(new os::HLayoutSpacer("", 2.0f, 2.0f));
      }
    }
    m_pcVLRoot->AddChild(new os::VLayoutSpacer("", 2.0f, 2.0f));
  }
    
  // Line things up (this runs REALLY slow)
  /*m_pcVLRoot->SameWidth("SVSu", "SVDay0", "SVDay7",  "SVDay14", "SVDay21", "SVDay28", "SVDay35",
			"SVMo", "SVDay1", "SVDay8",  "SVDay15", "SVDay22", "SVDay29", "SVDay36",
			"SVTu", "SVDay2", "SVDay9",  "SVDay16", "SVDay23", "SVDay30", "SVDay37",
			"SVWe", "SVDay3", "SVDay10", "SVDay17", "SVDay24", "SVDay31", "SVDay38",
			"SVTh", "SVDay4", "SVDay11", "SVDay18", "SVDay25", "SVDay32", "SVDay39",
			"SVFr", "SVDay5", "SVDay12", "SVDay19", "SVDay26", "SVDay33", "SVDay40",
			"SVSa", "SVDay6", "SVDay13", "SVDay20", "SVDay27", "SVDay34", "SVDay41", NULL);
			*/

  // Set root and add to window
  m_pcLRoot->SetRoot(m_pcVLRoot);
  AddChild(m_pcLRoot);
  
  // Set tab order
  int iTabOrder = 0;
  m_pcDDMMonth->SetTabOrder(iTabOrder++);
  m_pcSPYear->SetTabOrder(iTabOrder++);

  // Get current date/time
  time_t sTime;
  time(&sTime);

  // Convert to local time
  tm *sLTime = localtime(&sTime);

  // Store this 
  m_iDay = sLTime->tm_mday;
  m_iMonth = sLTime->tm_mon;
  m_iYear = sLTime->tm_year;
}

void DatePicker::AttachedToWindow()
{
  // Attach all
  m_pcDDMMonth->SetTarget(this);
  m_pcSPYear->SetTarget(this);

  for (int i = 0; i < 42; i++) {
    m_pcSVDateNumbers[i]->SetTarget(this);
  }
} 

void DatePicker::ReDraw() 
{
  // Figure out how many days in month
  int iDaysInMonth;
  int iIndex = 0;
  switch (m_iMonth) {

  case 1: // February
    iDaysInMonth = 28;
    if (__isleap(m_iYear))
      iDaysInMonth++;
    break;

  case 3: // April
  case 5: // June
  case 8: // September
  case 10: // November
    iDaysInMonth = 30;
    break;

  default: // Else 31 days
    iDaysInMonth = 31;
    break;
  }

  // If current day higher than days, drop down
  if (m_iDay > iDaysInMonth)
    m_iDay = iDaysInMonth;

  // Figure out day of week for 1st of month, by parsing date through timelocal
  tm sTime;
  sTime.tm_sec = 0;
  sTime.tm_min = 0;
  sTime.tm_hour = 0;
  sTime.tm_mday = 1;
  sTime.tm_mon = m_iMonth;
  sTime.tm_year = m_iYear;
  timelocal(&sTime);
  int m_iDayOfWeek = sTime.tm_wday;

  // Blank out unused date fields at start
  for (int i = 0; i < m_iDayOfWeek; i++) {
    m_pcSVDateNumbers[iIndex]->SetString("");
    m_pcSVDateNumbers[iIndex]->SetSelected(false);
    iIndex++;
  }

  // Now start filling the listview
  for (int iDay = 1; iDay < iDaysInMonth + 1; iDay++) {
  
    // Set string for day
    char szScratch[3];
    sprintf(szScratch, "%d", iDay);
    m_pcSVDateNumbers[iIndex]->SetString(szScratch);
    if (iDay != m_iDay) {
      m_pcSVDateNumbers[iIndex]->SetSelected(false);
    } else {
      m_pcSVDateNumbers[iIndex]->SetSelected(true);
    }
    iIndex++;
  }
  
  // Blank out unused date fields at end
  for (; iIndex < 42; iIndex++) {
    m_pcSVDateNumbers[iIndex]->SetString("");
    m_pcSVDateNumbers[iIndex]->SetSelected(false);
  }

  // Set current day on grid

  // Set current month dropdown selection
  m_pcDDMMonth->SetSelection(m_iMonth);

  // Show year
  m_pcSPYear->SetValue(m_iYear + 1900, false);
}

void DatePicker::DayChanged(os::Message *pcMessage)
{
  // Day changed, update variables
  std::string strValue;
  pcMessage->FindString("day", &strValue);
  m_iDay = atoi(strValue.c_str());

  // Update grid
  ReDraw();
}

void DatePicker::MonthChanged()
{
  // Month changed, update variables
  m_iMonth = m_pcDDMMonth->GetSelection();

  // And update grid
  ReDraw();
}

void DatePicker::YearChanged()
{
  // Year changed, update variables
  m_iYear = m_pcSPYear->GetValue();
  m_iYear -= 1900;

  // Update grid
  ReDraw();
}

void DatePicker::HandleMessage(os::Message* pcMessage)
{
  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_DAYCHANGED:
    DayChanged(pcMessage);
    break;

  case M_MONTHCHANGED:
    MonthChanged();
    break;

  case M_YEARCHANGED:
    YearChanged();
    break;

  default:
    os::View::HandleMessage(pcMessage); 
    break;
  }

}

void DatePicker::Paint(const os::Rect &cUpdateRect)
{
  os::View::Paint(cUpdateRect);
  //ReDraw();
}

void DatePicker::FrameSized(const os::Point &cDelta)
{
  os::View::FrameSized(cDelta);
  ReDraw();
}

os::Point DatePicker::GetPreferredSize(bool bLargest) const
{
  return os::Point(160, 140);
}

void DatePicker::GetDate(int *iDay, int *iMonth, int *iYear, std::string *strFormatted)
{
  *iDay = m_iDay;
  *iMonth = m_iMonth;
  *iYear = m_iYear + 1900;

  // Return day of week
  std::string strDayOfWeek;
  switch (m_iDayOfWeek) {
  case 0: strDayOfWeek = MSG_DAYMONTH_DAY_SUNDAY; break;
  case 1: strDayOfWeek = MSG_DAYMONTH_DAY_MONDAY; break;
  case 2: strDayOfWeek = MSG_DAYMONTH_DAY_TUESDAY; break;
  case 3: strDayOfWeek = MSG_DAYMONTH_DAY_WEDNESDAY; break;
  case 4: strDayOfWeek = MSG_DAYMONTH_DAY_THURSDAY; break;
  case 5: strDayOfWeek = MSG_DAYMONTH_DAY_FRIDAY; break;
  case 6: strDayOfWeek = MSG_DAYMONTH_DAY_SATURDAY; break;
  default: strDayOfWeek = MSG_DAYMONTH_DAY_UNKNOWN; break; // Should never happen!
  }

  // Return text month string
  std::string strMonth;
  switch (m_iMonth) {
  case 0: strMonth = MSG_DAYMONTH_MONTH_JANUARY; break;
  case 1: strMonth = MSG_DAYMONTH_MONTH_FEBRUARY; break;
  case 2: strMonth = MSG_DAYMONTH_MONTH_MARCH; break;
  case 3: strMonth = MSG_DAYMONTH_MONTH_APRIL; break;
  case 4: strMonth = MSG_DAYMONTH_MONTH_MAY; break;
  case 5: strMonth = MSG_DAYMONTH_MONTH_JUNE; break;
  case 6: strMonth = MSG_DAYMONTH_MONTH_JULY; break;
  case 7: strMonth = MSG_DAYMONTH_MONTH_AUGUST; break;
  case 8: strMonth = MSG_DAYMONTH_MONTH_SEPTEMBER; break;
  case 9: strMonth = MSG_DAYMONTH_MONTH_OCTOBER; break;
  case 10: strMonth = MSG_DAYMONTH_MONTH_NOVEMBER; break;
  case 11: strMonth = MSG_DAYMONTH_MONTH_DECEMBER; break;
  default: strMonth = MSG_DAYMONTH_MONTH_UNKNOWN; break; // Obviously this should never happen!
  }

  // Set day of week
  char szScratch[10];
  sprintf(szScratch, "%d", m_iDay);
  std::string strDay = std::string(szScratch);

  // Add th/nd/rd as appropriate
  if (m_iDay==1 || m_iDay==21 || m_iDay==31) {
    strDay+="st";
  } else if (m_iDay==2 || m_iDay==22) {
    strDay+="nd";
  } else if (m_iDay==3 || m_iDay==23) {
    strDay+="rd";
  } else {
    strDay+="th";
  }

  // Add year
  sprintf(szScratch, "%d", m_iYear + 1900);
  std::string strYear = std::string(szScratch);

  *strFormatted = strDayOfWeek + ", " + strDay  + " " + strMonth + " " + strYear;
				
}

void DatePicker::SetDate(int iDay, int iMonth, int iYear)
{
  // FIXME: could do with checks to make sure passed data is valid
  m_iDay = iDay;
  m_iMonth = iMonth;
  m_iYear = iYear - 1900;
}

DatePickerDay::DatePickerDay(const os::Rect cFrame, const char *pszName, const char *pszString, 
			     bool bColourBackground, bool bHeader,
			     os::Message *pcMessage) :
  os::View(cFrame, pszName), os::Invoker()
{
  // Store value
  m_strValue = std::string(pszString);
  m_bColourBackground = bColourBackground;
  m_bSelected = false;
  m_bHeader = bHeader;

  // Update message
  m_iCode = pcMessage->GetCode();
  pcMessage->AddString("day", pszString);
  SetMessage(pcMessage);
}

void DatePickerDay::AttachedToWindow()
{
  // Set font width/heights
  os::font_height sHeight;
  GetFontHeight(&sHeight);
  m_fHeight = (sHeight.ascender + sHeight.descender);
  m_fAscender = sHeight.ascender;
  GetFont()->SetProperties( "System/Bold");
  //SetFont(new os::Font("System/Bold"));
  m_fWidth = GetStringWidth("MM");
  GetFont()->SetProperties( "System/Regular");
 // SetFont(new os::Font("System/Regular"));
}

void DatePickerDay::SetString(const char *pszString)
{
  m_strValue = std::string(pszString);
  
  // Change message
  os::Message *pcMessage = GetMessage();
  pcMessage->RemoveData("day");
  pcMessage->AddString("day", pszString);
  Invalidate();
  Flush();
}

void DatePickerDay::Paint(const os::Rect &cUpdateRect)
{
  os::View::Paint(cUpdateRect);
  os::Rect cBounds = GetBounds();
  // Fill background with slightly brighter color than normal
  View *pcParent = GetParent();
  os::Color32_s sColor = pcParent->GetBgColor();
  os::Color32_s sSavedColor = sColor;
  if (m_bColourBackground) {
    sColor = GetBgColor();
    if (m_bSelected) {
      sColor.red=255;
      sColor.green=255;
      sColor.blue=255;
    }
  }
  FillRect(cBounds, sColor);
  SetBgColor(sColor);
  SetFgColor(0, 0, 0);
  
  // Calc width
  float x = (cBounds.Width() + 1.0f) / 2.0f - GetStringWidth(m_strValue) / 2.0f;
  float y = (cBounds.Height() + 1.0f) /  2.0f - m_fHeight / 2.0f + m_fAscender;

  // And show text
  if (!m_bHeader) {
    GetFont()->SetProperties("System/Regular");
  } else {
    GetFont()->SetProperties("System/Bold");
  }
  DrawString(os::Point(x, y), m_strValue);
  SetBgColor(sSavedColor);
}

os::Point DatePickerDay::GetPreferredSize(bool bLargest) const
{
  // And return
  return os::Point(m_fWidth, m_fHeight * 1.5f);
}

void DatePickerDay::SetSelected(bool bSelected)
{
  m_bSelected = bSelected;
}

void DatePickerDay::MouseDown(const os::Point &cPosition, uint32 iButtons)
{
  Invoke();
}


