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

#include <stdio.h>
#include <util/application.h>
#include <util/message.h>
#include <time.h>
#include <math.h>
#include "timepicker.h"

using namespace os;

TimePicker::TimePicker(const os::Rect& cFrame, const char *pszName) : 
  os::View(cFrame, pszName)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  m_pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  m_pcVLRoot = new os::VLayoutNode("VL");

  // Current time
  m_pcHLCurrentTime = new os::HLayoutNode("", 1.0f, m_pcVLRoot);
  
  // Hour
  m_pcHLCurrentTime->AddChild(m_pcSPHour = new os::Spinner(cRect, "SPHour", 0.0f, new os::Message(M_HOURCHANGED)));
  m_pcSPHour->SetMinMax(0.0f, 23.0f);
  m_pcSPHour->SetStep(1.0f);
  m_pcSPHour->SetMinPreferredSize(2);
  m_pcSPHour->SetMaxPreferredSize(2);
  m_pcSPHour->SetFormat("%0.f");
  m_pcHLCurrentTime->AddChild(new os::HLayoutSpacer("", 5.0f, 5.0f));

  // Minute
  m_pcHLCurrentTime->AddChild(m_pcSPMin = new os::Spinner(cRect, "SPMin", 0.0f, new os::Message(M_MINCHANGED)));
  m_pcSPMin->SetMinMax(0.0f, 59.0f);
  m_pcSPMin->SetStep(1.0f);
  m_pcSPMin->SetMinPreferredSize(2);
  m_pcSPMin->SetMaxPreferredSize(2);
  m_pcSPMin->SetFormat("%0.f");
  m_pcHLCurrentTime->AddChild(new os::HLayoutSpacer("", 5.0f, 5.0f));

  // Seconds
  m_pcHLCurrentTime->AddChild(m_pcSPSec = new os::Spinner(cRect, "SPSec", 0.0f, new os::Message(M_SECCHANGED)));
  m_pcSPSec->SetMinMax(0.0f, 59.0f);
  m_pcSPSec->SetStep(1.0f);
  m_pcSPSec->SetMinPreferredSize(2);
  m_pcSPSec->SetMaxPreferredSize(2);
  m_pcSPSec->SetFormat("%0.f");

  // Add Clock
  m_pcVLRoot->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f));
  m_pcVLRoot->AddChild(m_pcClock = new os::Clock(cBounds, "Clock"));

  // Set root and add to window
  m_pcLRoot->SetRoot(m_pcVLRoot);
  AddChild(m_pcLRoot);

  // Set tab order
  int iTabOrder = 0;
  m_pcSPHour->SetTabOrder(iTabOrder++);
  m_pcSPMin->SetTabOrder(iTabOrder++);
  m_pcSPSec->SetTabOrder(iTabOrder++);

  // Get current date/time
  time_t sTime;
  time(&sTime);

  // Convert to local time
  tm *sLTime = localtime(&sTime);

  // Store this 
  m_iHour = sLTime->tm_hour;
  m_iMin = sLTime->tm_min;
  m_iSec = sLTime->tm_sec;
  m_iHourDiff = 0;
  m_iMinDiff = 0;
  m_iSecDiff = 0;

  // Create looper
  m_pcLooper = new Looper(pszName);
}

TimePicker::~TimePicker()
{
  // Delete looper object
  m_pcLooper->RemoveTimer(this, 0);
  m_pcLooper->Quit();
}

void TimePicker::AttachedToWindow()
{
  // Attach all controls
  m_pcSPHour->SetTarget(this);
  m_pcSPMin->SetTarget(this);
  m_pcSPSec->SetTarget(this);

  // Create timer
  m_pcLooper->AddTimer(this, 0, 1000000, false);
  m_pcLooper->Run();
}

void TimePicker::Paint(const os::Rect &cUpdateRect)
{
  os::View::Paint(cUpdateRect);

  // Clear all
  os::Rect cBounds = GetBounds();
  FillRect(cBounds, get_default_color(os::COL_NORMAL));

  // Get time
  int iHours, iMinutes, iSeconds;
  GetTime(&iHours, &iMinutes, &iSeconds);

  // Show hours
  m_pcSPHour->SetValue(iHours, false);

  // Minutes
  m_pcSPMin->SetValue(iMinutes, false);

  // Seconds
  m_pcSPSec->SetValue(iSeconds, false);

  m_pcClock->SetTime(iHours, iMinutes, iSeconds);
}

void TimePicker::HandleMessage(os::Message* pcMessage)
{
  int iMessage = pcMessage->GetCode();

  // Get message code and act on it
  switch(iMessage) {

  case M_SECCHANGED:
    SecondsChanged();
    break;

  case M_MINCHANGED:
    MinutesChanged();
    break;

  case M_HOURCHANGED:
    HoursChanged();
    break;

  default:
    os::View::HandleMessage(pcMessage); 
    break;
  }

}

void TimePicker::SecondsChanged()
{
  // Work out difference
  int iDiff = m_pcSPSec->GetValue().AsInt32() - m_iSec;
  m_iSecDiff = iDiff;
}

void TimePicker::MinutesChanged()
{
  // Work out difference
  int iDiff = m_pcSPMin->GetValue().AsInt32() - m_iMin;
  m_iMinDiff = iDiff;
}

void TimePicker::HoursChanged()
{
  // Work out difference
  int iDiff = m_pcSPHour->GetValue().AsInt32() - m_iHour;
  m_iHourDiff = iDiff;
}

void TimePicker::GetTime(int *iHours, int *iMinutes, int *iSeconds)
{
  // Check within sensible values for diff's
  if (m_iHourDiff > 23)
    m_iHourDiff-= 24;
  if (m_iHourDiff < -23)
    m_iHourDiff+= 24;
  if (m_iMinDiff > 59)
    m_iMinDiff-= 60;
  if (m_iMinDiff < -59)
    m_iMinDiff+= 60;
  if (m_iSecDiff > 59)
    m_iSecDiff-= 60;
  if (m_iSecDiff < -59)
    m_iSecDiff+= 60;
  
  // Get values
  *iHours = m_iHour + m_iHourDiff;
  *iMinutes = m_iMin + m_iMinDiff;
  *iSeconds = m_iSec + m_iSecDiff;

  // Adjust for overflow
  if (*iSeconds < 0) {
    (*iSeconds)+= 60;
    (*iMinutes)--;
  } else if (*iSeconds > 59) {
    (*iSeconds)-=60;
    (*iMinutes)++;
  }

  if (*iMinutes < 0) {
    (*iMinutes)+= 60;
    (*iHours)--;
  } else if (*iMinutes > 59) {
    (*iMinutes)-= 60;
    (*iHours)++;
  }

  if (*iHours < 0) { 
    (*iHours)+= 24;
  } else if (*iHours > 59) {
    (*iHours)-= 24;
  }
}

void TimePicker::TimerTick(int nID)
{
  // If nID = 0, its our every second timer
  if (nID==0) {

    // Get time
    time_t sTime;
    time(&sTime);

    // Convert to local time
    tm *sLTime = localtime(&sTime);

    // Get hours/mins/seconds
    m_iHour  =  sLTime->tm_hour;
    m_iMin = sLTime->tm_min;
    m_iSec = sLTime->tm_sec;

    // Update clock
    Invalidate();
    m_pcClock->Invalidate();
    Flush();
  }
}

os::Point TimePicker::GetPreferredSize(bool bLargest) const
{
  return (os::Point(120, 140));
}

Clock::Clock(const os::Rect cFrame, const char *pszName) :
  os::View(cFrame, pszName)
{
  // Store values
  m_iHour = 0;
  m_iMin = 0;
  m_iSec = 0;
}

void Clock::AttachedToWindow()
{
}

void Clock::SetTime(int iHour, int iMin, int iSec)
{
  m_iHour = iHour;
  m_iMin = iMin;
  m_iSec = iSec;
//  Invalidate();
//  Flush();
}

void Clock::Paint(const os::Rect &cUpdateRect)
{
  os::View::Paint(cUpdateRect);
  os::Rect cBounds = GetBounds();
 
  // Fill background 
  View *pcParent = GetParent();
  os::Color32_s sColor = pcParent->GetBgColor();
  FillRect(cBounds, sColor);

  // Clock
  float fCenterX = (cBounds.left + cBounds.right)/2;
  float fCenterY = (cBounds.top + cBounds.bottom)/2;
  float fWidth = cBounds.right - cBounds.left + 1;
  float fHeight = cBounds.bottom - cBounds.top + 1;
  float fSize = fWidth / 2.0f;
  if (fHeight > fWidth) {
    cBounds.top += (fHeight-fWidth) / 2.0f;
    cBounds.bottom -= (fHeight-fWidth) / 2.0f;
  } else {
    cBounds.left += (fWidth-fHeight) / 2.0f;
    cBounds.right -= (fWidth-fHeight) / 2.0f;
    fSize = fHeight / 2.0f;
  }

  // Draw hour spots
  SetFgColor(os::Color32_s(0, 0, 0));
  int j = 4;
  for (float i = 0; i < 48; i++) {
    if (j==4) {
      DrawCircleFrame(7.5f * i, fCenterX, fCenterY, fSize - 2.0f, 1.0f);
      j = 0;
   } else {
      DrawCircleFrame(7.5f * i, fCenterX, fCenterY, fSize - 2.0f, 0.0f);
    }
    j++;
  }

  // Draw hour hand
  float fHour = (float)m_iHour;
  if (fHour > 12)
    fHour -= 12;
  fHour += (float)m_iMin / 60.0f;
  DrawCircleLine(30.0f * fHour, fCenterX, fCenterY, 0.0f, fSize / 2.0f);

  // Minute hand
  DrawCircleLine(6.0f * ((float)m_iMin + ((float)m_iSec / 60.0f)), fCenterX, fCenterY, 0.0f, fSize / 1.25f);

  // Second hand
  DrawCircleLine(6.0f * (float)m_iSec, fCenterX, fCenterY, 0.0f, fSize / 2.5f);
}

void Clock::DrawCircleLine(float fAngle, float fCenterX, float fCenterY, float fStart, float fEnd)
{
  float fDAngle = (fAngle * (float)M_PI) / 180.0f;
  DrawLine(os::Point(fCenterX + (fStart * sin(fDAngle)), fCenterY - (fStart * cos(fDAngle))),
	   os::Point(fCenterX + (fEnd * sin(fDAngle)), fCenterY - (fEnd * cos(fDAngle))));
} 

void Clock::DrawCircleFrame(float fAngle, float fCenterX, float fCenterY, float fPosition, float fSize)
{
  float fDAngle = (fAngle * (float)M_PI) / 180.0f;
  float x = fCenterX + (fPosition * sin(fDAngle));
  float y = fCenterY + (fPosition * cos(fDAngle));
  if (fSize == 0.0f) {
    DrawLine(os::Point(x, y), os::Point(x, y));
  } else {
    DrawFrame(os::Rect(x-fSize, y-fSize, x+fSize, y+fSize), os::FRAME_ETCHED);
  }
}

os::Point Clock::GetPreferredSize(bool bLargest) const
{
  // And return
  return os::Point(100, 100);
}





