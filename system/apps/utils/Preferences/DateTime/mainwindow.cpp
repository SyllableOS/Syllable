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

#include <stdio.h>
#include <fstream>
#include <atheos/image.h>
#include <util/resources.h>
#include <util/application.h>
#include <util/message.h>
#include <storage/file.h>
#include "main.h"
#include "mainwindow.h"
#include "messages.h"
#include "resources/DateTime.h"

CMainWindow::CMainWindow(const os::Rect& cFrame) : 
  os::Window(cFrame, "MainWindow", MSG_MAINWND_TITLE, os::WND_NOT_RESIZABLE)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  m_pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  m_pcVLRoot = new os::VLayoutNode("VL");
  m_pcVLRoot->SetBorders(os::Rect(10, 5, 10, 5));

  // Tabview
  m_pcTVRoot = new os::TabView(cBounds, "TVRoot");

  // Hor time/date view & node
  m_pcLTimeDate = new os::LayoutView(cBounds, "LTimeDate", NULL, os::CF_FOLLOW_ALL);
  m_pcVLTimeDate = new os::VLayoutNode("VLTimeDate");
  m_pcVLTimeDate->SetBorders(os::Rect(5, 5, 5, 5));
  
  m_pcHLTimeDate = new os::HLayoutNode("HLTimeDate", 1.0f, m_pcVLTimeDate);

  // Time
  m_pcTPTime = new os::TimePicker(cRect, "TPTime");
  m_pcHLTime = new os::HLayoutNode("HLTime");
  m_pcHLTime->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcHLTime->AddChild( m_pcTPTime );
  m_pcFVTime = new os::FrameView(cBounds, "FVTime", MSG_MAINWND_TIME, os::CF_FOLLOW_ALL);
  m_pcFVTime->SetRoot(m_pcHLTime);
  m_pcHLTimeDate->AddChild(m_pcFVTime);
  
 			 
  // Space
  m_pcHLTimeDate->AddChild(new os::HLayoutSpacer("", 10.0f, 10.0f));
  
  // Date
  m_pcDPDate = new os::DatePicker(cRect, "DPDate");
  m_pcHLDate = new os::HLayoutNode("HLDate");
  m_pcHLDate->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcHLDate->AddChild( m_pcDPDate );
  m_pcFVDate = new os::FrameView(cBounds, "FVDate", MSG_MAINWND_DATE, os::CF_FOLLOW_ALL);
  m_pcFVDate->SetRoot(m_pcHLDate);
  m_pcHLTimeDate->AddChild(m_pcFVDate);
 
  // Add full time/date stringview
  m_pcSVFullTimeDate = new os::StringView(cRect, "SVFull", "", os::ALIGN_CENTER);
  m_pcVLTimeDate->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f));
  m_pcVLTimeDate->AddChild(m_pcSVFullTimeDate);
  m_pcSVFullTimeDate->SetMinPreferredSize(50);
  m_pcSVFullTimeDate->SetMaxPreferredSize(50);
 
  // Add time/date tab
  m_pcLTimeDate->SetRoot(m_pcVLTimeDate);
  m_pcTVRoot->AppendTab(MSG_MAINWND_TIMEDATE, m_pcLTimeDate);

  // Create bitmap object and Load map
  m_pcBIImage = new os::BitmapImage();
  m_pcBIImageSaved = new os::BitmapImage();
  
  // Load image and save it 
  os::File cSelf( open_image_file( get_image_id() ) );
  os::Resources cCol( &cSelf );
		
  os::ResStream *pcStream = cCol.GetResourceStream( "timezone.png" );
  m_pcBIImage->Load(pcStream);
  delete( pcStream );
  
  // Store it
  *m_pcBIImageSaved = *m_pcBIImage;
  
  // Add timezone
  m_pcLTimeZone = new os::LayoutView(cBounds, "LTimeZone", NULL, os::CF_FOLLOW_ALL);
  m_pcVLTimeZone = new os::VLayoutNode("VLTimeZone");
  m_pcVLTimeZone->SetBorders(os::Rect(5, 5, 5, 5));
  m_pcVLTimeZone->AddChild(new os::HLayoutSpacer(""));
  
  // Timezone
  m_pcHLTimeZone = new os::HLayoutNode("HLTimeZone", 1.0f, m_pcVLTimeZone);
  m_pcDDMTimeZone = new os::DropdownMenu(cRect, "DDMTimeZone");
  m_pcDDMTimeZone->SetSelectionMessage(new os::Message(M_MW_TIMEZONECHANGED));
  m_pcDDMTimeZone->SetTarget(this);
  m_pcDDMTimeZone->SetMinPreferredSize(30);
  m_pcDDMTimeZone->SetMaxPreferredSize(30);
  m_pcHLTimeZone->AddChild(m_pcDDMTimeZone);
  m_pcHLTimeZone->AddChild(new os::VLayoutSpacer("", 5.0f, 5.0f));

  // Map
  m_pcHLTimeZoneMap = new os::HLayoutNode("HLTimeZoneMap", 1.0f, m_pcVLTimeZone);
  m_pcHLTimeZoneMap->AddChild(m_pcIVTimeZone = new os::ImageView(cRect, "IVImage", m_pcBIImage));
  
  // Add frameview and add to tab
  m_pcLTimeZone->SetRoot(m_pcVLTimeZone);
  m_pcTVRoot->AppendTab(MSG_MAINWND_TIMEZONE, m_pcLTimeZone);
  
  // Add tabview to root
  m_pcVLRoot->AddChild(m_pcTVRoot);

  // Add buttons  (if root)
  if (g_bRoot) {
    m_pcVLRoot->AddChild(new os::VLayoutSpacer("", 10.0f, 10.0f));
    m_pcHLButtons = new os::HLayoutNode("HLButtons", 1.0f, m_pcVLRoot);
    m_pcHLButtons->AddChild(new os::HLayoutSpacer(""));
    m_pcHLButtons->AddChild(m_pcBApply = new os::Button(cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message(M_MW_APPLY)));
    m_pcHLButtons->AddChild(new os::HLayoutSpacer("", 5.0f, 5.0f) );
    m_pcHLButtons->SameWidth("BApply", NULL);
  }

  // Set root and add to window
  m_pcLRoot->SetRoot(m_pcVLRoot);
  AddChild(m_pcLRoot);

  // Set Default button and initial focus
  if (g_bRoot) {
    this->SetDefaultButton(m_pcBApply);
  }
  
  // Set tab order
  int iTabOrder = 0;
  if (g_bRoot) {
    m_pcBApply->SetTabOrder(iTabOrder++);
  }

  // Set fields to disabled for non-root
  if (!g_bRoot) {
    this->SetTitle("Date & Time (View)");
  }
  
  
  // Set Icon
  pcStream = cCol.GetResourceStream( "icon48x48.png" );
  os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
  SetIcon( pcIcon->LockBitmap() );
  delete( pcIcon );
  
  // Parse in time zones
  AddTimeZones();

  // Add timer to update full time/date at bottom
  AddTimer(this, 0, 500000, false);
}

CMainWindow::~CMainWindow()
{
	delete( m_pcBIImage );
	delete( m_pcBIImageSaved );
}

void CMainWindow::GetLine( os::StreamableIO* pcIO, char* zBuffer, int nMaxLen, char zEnd )
{
	char zTemp;
	int nPointer = 0;
	
	while( nPointer < nMaxLen && pcIO->Read( (void*)&zTemp, 1 ) == 1 )
	{
		if( zTemp == zEnd )
		{
			zBuffer[nPointer] = 0;
			return;
		}
		
		zBuffer[nPointer++] = zTemp;
	}
}

void CMainWindow::AddTimeZones()
{
  // Open up file
  os::File cSelf( open_image_file( get_image_id() ) );
  os::Resources cCol( &cSelf );
  os::ResStream *pcStream = cCol.GetResourceStream( "timezone.info" );

  // Scratch space to read in lines
  char szScratch[1024];
  
  // Read in number of lines first
  GetLine( pcStream, szScratch, 1024, '\n');
  int iNumZones = atoi((const char *)&szScratch);
 
  // Dimension arrays
  m_pcstrShort = new std::string*[iNumZones];
  m_pcstrFile = new std::string*[iNumZones];
  m_piDiff = new int[iNumZones];

  // Now start reading in lines
  for (int i = 0; i < iNumZones; i++) {
    
    // Get description
    GetLine( pcStream, szScratch, 1024, '|');
    
    m_pcDDMTimeZone->AppendItem(szScratch);

    // Get short description
    GetLine( pcStream, szScratch, 1024, '|');
    m_pcstrShort[i] = new std::string((const char *)&szScratch);

    // Get locale link
    GetLine( pcStream, szScratch, 1024, '|');
    m_pcstrFile[i] = new std::string((const char *)&szScratch);

    // get Horiz offset
    GetLine( pcStream, szScratch, 1024, '\n');
    m_piDiff[i] = atoi(szScratch);
  }  

  // And close
  delete( pcStream );
}

void CMainWindow::UpdateTimeZoneMap()
{
  // Copy saved bitmap
  *m_pcBIImage = *m_pcBIImageSaved;

  // Translate on x axis and shade
  os::Bitmap *BTemp = m_pcBIImage->LockBitmap();
  uint8 *cData = BTemp->LockRaster();
  uint8 *cTemp =new uint8[1024];
  int start = m_piDiff[m_pcDDMTimeZone->GetSelection()];
  int width1 = 256 - start;
  for (int y = 1; y < 149; y++) {
    
    // Translate
    memcpy(cTemp, &(cData[(y*1024)]), 1024);
    memcpy(&(cData[(y*1024)+(start*4)]), cTemp, width1 * 4);
    memcpy(&(cData[(y*1024)]), cTemp + (width1*4), start * 4);

    // and colour green/yellow
    for (int x = 125*4; x < 135*4; x+=4) {
      int iShade = cData[(y*1024)+x]+cData[(y*1024)+x+1]+cData[(y*1024)+x+2];
      if (iShade>255)
	iShade = 255;
      cData[(y*1024)+x] = 0;
      cData[(y*1024)+x+1] = iShade;
      cData[(y*1024)+x+2] = iShade;
    }
  }
  BTemp->UnlockRaster();
  m_pcBIImage->UnlockBitmap();

  // And update image on screen
  m_pcIVTimeZone->SetImage(m_pcBIImage);
}

void CMainWindow::Apply() 
{
  // Delete current timezone, easier to do through system() than calls
  system("rm /etc/localtime");
  
  int nTimeZone = m_pcDDMTimeZone->GetSelection();
  if( nTimeZone < 0 )
    	nTimeZone = 0;

  // Find out current file
  std::string strFile = std::string("ln /system/indexes/share/zoneinfo");
  strFile += *m_pcstrFile[nTimeZone];
  strFile += std::string(" /etc/localtime -s");
  system(strFile.c_str());
}

void CMainWindow::HandleMessage(os::Message* pcMessage)
{
  int iMessage = pcMessage->GetCode();

  // Get message code and act on it
  switch(iMessage) {

  case M_MW_APPLY:
    Apply();
    break;

  case M_MW_TIMEZONECHANGED:
    UpdateTimeZoneMap();
    break;

  default:
    os::Window::HandleMessage(pcMessage); 
    break;
  }

}

bool CMainWindow::OkToQuit()
{
  os::Application::GetInstance()->PostMessage(os::M_QUIT);
  return true;
}

void CMainWindow::TimerTick(int nID)
{
  // Update time/date string at bottom of tabview
  if (nID==0) {
    
    // Get time/date
    int iDay, iMonth, iYear;
    int iHour, iMin, iSec;
    std::string strDate;
    m_pcTPTime->GetTime(&iHour, &iMin, &iSec);
    m_pcDPDate->GetDate(&iDay, &iMonth, &iYear, &strDate);
    
    int nTimeZone = m_pcDDMTimeZone->GetSelection();
    if( nTimeZone < 0 )
    	nTimeZone = 0;
	
    // Create the date/time text
    char szScratch[1024];
    if (iHour <= 12) {
      sprintf(szScratch, "%d:%0d:%0d AM, %s %s", iHour, iMin, iSec, 
	      strDate.c_str(), m_pcstrShort[nTimeZone]->c_str());
    } else {
      sprintf(szScratch, "%d:%0d:%0d PM, %s %s", iHour - 12, iMin, iSec, 
	      strDate.c_str(), m_pcstrShort[nTimeZone]->c_str());
    }
	
    m_pcSVFullTimeDate->SetString(szScratch);
  }
}
