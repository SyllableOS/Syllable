// Screen Preferences :: (C)opyright 2002 Daryl Dudey
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
#include <dirent.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/desktop.h>
#include <gui/guidefines.h>
#include "main.h"
#include "mainwindow.h"
#include "messages.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", "Screen", os::WND_NOT_RESIZABLE)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );

  // Store v-node to store settings
  pcVLSettings = new os::VLayoutNode("VLSettings");
  pcVLSettings->SetBorders( os::Rect(5,5,5,5));
  pcVLSettings->AddChild( new os::HLayoutSpacer(""));
  
  // Workspaces
  pcHLWorkspace = new os::HLayoutNode("HLWorkspace");
  pcHLWorkspace->AddChild( new os::HLayoutSpacer(""));
  pcHLWorkspace->AddChild( new os::StringView(cRect, "SVWorkspace", "Apply to") );
  pcHLWorkspace->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f));
  pcHLWorkspace->AddChild( pcDDMWorkspace = new os::DropdownMenu(cRect, "DDMWorkspace"));
  pcDDMWorkspace->SetMinPreferredSize(12);
  pcDDMWorkspace->SetMaxPreferredSize(12);
  pcDDMWorkspace->SetReadOnly(true);
  pcHLWorkspace->AddChild( new os::HLayoutSpacer(""));
  pcVLSettings->AddChild(pcHLWorkspace);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f));

  // Resolution
  pcHLResolution = new os::HLayoutNode("HLResolution");
  pcHLResolution->AddChild( new os::StringView(cRect, "SVResolution", "Resolution") );
  pcHLResolution->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLResolution->AddChild( pcDDMResolution = new os::DropdownMenu(cRect, "DDMResolution"));
  pcDDMResolution->SetMinPreferredSize(12);
  pcDDMResolution->SetMaxPreferredSize(12);
  pcDDMResolution->SetReadOnly(true);
  pcDDMResolution->SetSelectionMessage(new os::Message(M_MW_RESOLUTION));
  pcDDMResolution->SetTarget(this);
  pcVLSettings->AddChild(pcHLResolution);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f));

  // Colour space
  pcHLColour = new os::HLayoutNode("HLColour");
  pcHLColour->AddChild( new os::StringView(cRect, "SVColour", "Colour Depth") );
  pcHLColour->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLColour->AddChild( pcDDMColour = new os::DropdownMenu(cRect, "DDMColour"));
  pcDDMColour->SetMinPreferredSize(12);
  pcDDMColour->SetMaxPreferredSize(12);
  pcDDMColour->SetReadOnly(true);
  pcDDMColour->SetSelectionMessage(new os::Message(M_MW_COLOUR));
  pcDDMColour->SetTarget(this);
  pcVLSettings->AddChild(pcHLColour);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f, 5.0f));

  // Refresh
  pcHLRefresh = new os::HLayoutNode("HLRefresh");
  pcHLRefresh->AddChild( new os::StringView(cRect, "SVRefresh", "Refresh Rate") );
  pcHLRefresh->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLRefresh->AddChild( pcDDMRefresh = new os::DropdownMenu(cRect, "DDMRefresh"));
  pcDDMRefresh->SetMinPreferredSize(12);
  pcDDMRefresh->SetMaxPreferredSize(12);
  pcDDMRefresh->SetReadOnly();
  pcDDMRefresh->SetSelectionMessage(new os::Message(M_MW_REFRESH));
  pcDDMRefresh->SetTarget(this);
  pcVLSettings->AddChild(pcHLRefresh);

  // Align the controls
  pcVLSettings->SameWidth( "SVWorkspace", "SVResolution", "SVColour", "SVRefresh", "SVCustom", NULL);
  pcVLSettings->SameWidth( "DDMWorkspace", "DDMResolution", "DDMColour", "DDMRefresh", "SLCustom", NULL);

  // Create frameview to store settings
  pcFVSettings = new os::FrameView( cBounds, "FVSettings", "Display", os::CF_FOLLOW_ALL);
  pcFVSettings->SetRoot(pcVLSettings);
  pcVLRoot->AddChild( pcFVSettings );
  
  // Create apply/revert/close buttons
  pcHLButtons = new os::HLayoutNode("HLButtons");
  if (bRoot) {
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));
    pcHLButtons = new os::HLayoutNode("HLButtons");
    pcHLButtons->AddChild( new os::HLayoutSpacer(""));
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", "Apply", new os::Message(M_MW_APPLY)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBUndo = new os::Button(cRect, "BUndo", "Undo", new os::Message(M_MW_UNDO)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDefault = new os::Button(cRect, "BDefault", "Default", new os::Message(M_MW_DEFAULT)) );
    pcHLButtons->SameWidth( "BApply", "BUndo", "BDefault", NULL );
    pcVLRoot->AddChild(pcHLButtons);
  }

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set default button and initial focus (if root)
  if (bRoot) {
    this->SetDefaultButton(pcBApply);
    pcDDMResolution->MakeFocus();
  }

  // Set read only if not root
  if (!bRoot) {
    pcDDMWorkspace->SetEnable(false);
    pcDDMResolution->SetEnable(false);
    pcDDMColour->SetEnable(false);
    pcDDMRefresh->SetEnable(false);
  }

  // Set tab order
  int iTabOrder = 0;
  pcDDMWorkspace->SetTabOrder(iTabOrder++);
  pcDDMResolution->SetTabOrder(iTabOrder++);
  pcDDMColour->SetTabOrder(iTabOrder++);
  if (bRoot) {
    pcBDefault->SetTabOrder(iTabOrder++);
    pcBUndo->SetTabOrder(iTabOrder++);
    pcBApply->SetTabOrder(iTabOrder++);
  }

  // Get screen mode data
  cCurrent = os::Desktop().GetScreenMode();
  memcpy(&cUndo, &cCurrent, sizeof(cCurrent));
  memcpy(&cUndo2, &cCurrent, sizeof(cCurrent));

  // Show data
  ShowData();
  ColourChange();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData() 
{
  // How many modes?
  os::Application *pcApp = os::Application::GetInstance();
  int iModeCount = pcApp->GetScreenModeCount();
  os::screen_mode sMode;

  // Loop through all modes, to figure out all colour depths available
  int iScratch[MAX_DEPTH_COUNT];
  for (int i=0;i<iModeCount;i++) {

    // See if mode is valid, if it is, add values to dropdown menus
    if (pcApp->GetScreenModeInfo(i, &sMode)==0) {
      iScratch[sMode.m_eColorSpace] = 1;
    }
  }

  // Figure out how many depths are available
  int iAvailDepths = 0;
  pcDDMColour->Clear();
  for (int i=MAX_DEPTH_COUNT-1;i>=0;i--) {
    if (iScratch[i]==1) {
      
      // Add to drop down colour
      switch (i) {
      
      case os::CS_RGB15: pcDDMColour->AppendItem("15 Bit"); break;
      case os::CS_RGB16: pcDDMColour->AppendItem("16 Bit"); break;
      case os::CS_RGB24: pcDDMColour->AppendItem("24 Bit"); break;
      case os::CS_RGB32: pcDDMColour->AppendItem("32 Bit"); break;
      default: pcDDMColour->AppendItem("Unknown"); break;
      
      }

      // Set current colour space
      if (i==cCurrent.m_eColorSpace) {
	pcDDMColour->SetSelection(pcDDMColour->GetItemCount()-1);
      }
      iDepths[iAvailDepths++]=i;
    }
  } 

  // Populate workspace dropdown
  pcDDMWorkspace->Clear();
  pcDDMWorkspace->AppendItem("This workspace");
  pcDDMWorkspace->AppendItem("All workspaces");
  pcDDMWorkspace->SetSelection(0);

  // And populate refresh rates
  pcDDMRefresh->AppendItem("56 Hz");
  pcDDMRefresh->AppendItem("60 Hz");
  pcDDMRefresh->AppendItem("70 Hz");
  pcDDMRefresh->AppendItem("72 Hz");
  pcDDMRefresh->AppendItem("75 Hz");
  pcDDMRefresh->AppendItem("85 Hz");

  // Set current refresh selectin
  switch ((int)cCurrent.m_vRefreshRate) {
    
  case 56: pcDDMRefresh->SetSelection(0); break;
  case 60: pcDDMRefresh->SetSelection(1); break;
  case 70: pcDDMRefresh->SetSelection(2); break;
  case 72: pcDDMRefresh->SetSelection(3); break;
  case 75: pcDDMRefresh->SetSelection(4); break;
  case 85: pcDDMRefresh->SetSelection(5); break;
   
  }
}

void MainWindow::Apply() 
{
  // See if this or all desktops
  if (pcDDMWorkspace->GetSelection()==0) {
    os::Desktop().SetScreenMode(&cCurrent);
  } else {
    for (uint32 i=0;i<32;i++) {
      os::Desktop(i).SetScreenMode(&cCurrent);
    }
  }

  // Now copy current settings over the old
  memcpy(&cUndo2, &cUndo, sizeof(cUndo));
  memcpy(&cUndo, &cCurrent, sizeof(cCurrent));
}

void MainWindow::Undo()
{
  // Copy original state back
  memcpy(&cCurrent, &cUndo2, sizeof(cUndo));

  // Update controls
  ShowData();
  ColourChange();
}

void MainWindow::Default()
{
  // Create system defaults
  cCurrent.m_nWidth = 640;
  cCurrent.m_nHeight = 480;
  cCurrent.m_eColorSpace = os::CS_RGB16;
  cCurrent.m_vRefreshRate = 60.0f;

  // Update controls
  ShowData();
  ColourChange();
}

void MainWindow::ColourChange()
{
  // Now figure out applicable resolutions
  os::Application *pcApp = os::Application::GetInstance();
  int iModeCount = pcApp->GetScreenModeCount();
  os::screen_mode sMode;

  // Now loop through all and figure out how many resolutions are applicable
  pcDDMResolution->Clear();
  int iResCount = 0;
  int iPos = pcDDMColour->GetSelection();
  char *pzScratch = new char[20];
  for (int i=0;i<iModeCount;i++) {
    if (pcApp->GetScreenModeInfo(i, &sMode)==0) {
      if (sMode.m_eColorSpace==iDepths[iPos]) {
	
	// Fill the table with the values
	iScrWidth[iResCount] = sMode.m_nWidth;
	iScrHeight[iResCount] = sMode.m_nHeight;
	
	// Now add this to the dropdown menu
	sprintf(pzScratch, "%d x %d", iScrWidth[iResCount], iScrHeight[iResCount]);
	pcDDMResolution->AppendItem(pzScratch);

	// If current resolution, make selection
	if (sMode.m_nWidth==cCurrent.m_nWidth && sMode.m_nHeight==cCurrent.m_nHeight) {
	  pcDDMResolution->SetSelection(pcDDMResolution->GetItemCount()-1, true);
	}

	iResCount++;
      }
    }
  }
  delete pzScratch;
}

void MainWindow::ResolutionChange()
{
  // Save the newly selected resolution
  int iPos = pcDDMResolution->GetSelection();
  cCurrent.m_nWidth = iScrWidth[iPos];
  cCurrent.m_nHeight = iScrHeight[iPos];
}

void MainWindow::RefreshChange() 
{
  // Save the newly selected refresh rate
  int iPos = pcDDMRefresh->GetSelection();

  switch(iPos) {

  case 0: cCurrent.m_vRefreshRate = 56.0f; break;
  case 1: cCurrent.m_vRefreshRate = 60.0f; break;
  case 2: cCurrent.m_vRefreshRate = 70.0f; break;
  case 3: cCurrent.m_vRefreshRate = 72.0f; break;
  case 4: cCurrent.m_vRefreshRate = 75.0f; break;
  case 5: cCurrent.m_vRefreshRate = 85.0f; break;
 
  }
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{
  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MW_COLOUR:
    ColourChange();
    break;
   
  case M_MW_RESOLUTION:
    ResolutionChange();
    break;

  case M_MW_REFRESH:
    RefreshChange();
    break;

  case M_MW_APPLY:
    Apply();
    break;

  case M_MW_DEFAULT:
    Default();
    break;

  case M_MW_UNDO:
    Undo();
    break;

  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool MainWindow::OkToQuit()
{
  os::Application::GetInstance()->PostMessage(os::M_QUIT);
  return true;
}




