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

#include <cstdio>
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
  
  // Workspaces
  pcHLWorkspace = new os::HLayoutNode("HLWorkspace");
  pcHLWorkspace->AddChild( new os::StringView(cRect, "SVWorkspace", "Apply to") );
  pcHLWorkspace->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLWorkspace->AddChild( pcDDMWorkspace = new os::DropdownMenu(cRect, "DDMWorkspace"));
  pcDDMWorkspace->SetMinPreferredSize(12);
  pcDDMWorkspace->SetMaxPreferredSize(12);
  pcDDMWorkspace->SetReadOnly(true);
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
  pcVLSettings->SameWidth( "SVWorkspace", "SVResolution", "SVColour", "SVRefresh", NULL);
  pcVLSettings->SameWidth( "DDMWorkspace", "DDMResolution", "DDMColour", "DDMRefresh", NULL);

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
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", "_Apply", new os::Message(M_MW_APPLY)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBUndo = new os::Button(cRect, "BUndo", "_Undo", new os::Message(M_MW_UNDO)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDefault = new os::Button(cRect, "BDefault", "_Default", new os::Message(M_MW_DEFAULT)) );
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
  
  // Query all screenmodes
  m_nModeCount = os::Application::GetInstance()->GetScreenModeCount();
  m_pcModes = new os::screen_mode[m_nModeCount];
  for( int i = 0; i < m_nModeCount; i++ ) {
  	os::Application::GetInstance()->GetScreenModeInfo( i, &m_pcModes[i] );
  }

  // Show data
  ShowData();
}

MainWindow::~MainWindow()
{
	if( m_pcModes ) 
		delete( m_pcModes );
}

void MainWindow::ShowData() 
{
	char *pzScratch = new char[20];
	// Save all available resolutions
	m_cResolutions.clear();
	pcDDMResolution->Clear();
	
	for( int i = 0; i < m_nModeCount; i++ ) 
	{
		bool bFound = false;
		uint32 nRes = ( (uint16)m_pcModes[i].m_nWidth << 16 ) | (uint16)m_pcModes[i].m_nHeight;
		for( uint16 j = 0; j < m_cResolutions.size(); j++ ) {
			if( m_cResolutions[j] == nRes )
				bFound = true;
		}
		if( !bFound ) 
		{
			m_cResolutions.push_back( nRes );
		}
	}
	
	// Add them to the list
	for( uint16 i = 0; i < m_cResolutions.size(); i++ ) 
	{
		sprintf(pzScratch, "%d x %d", (int)( m_cResolutions[i] >> 16 ), (int)( m_cResolutions[i] & 0xffff ) );
		pcDDMResolution->AppendItem( pzScratch );
		if( i == 0 )
			pcDDMResolution->SetSelection( pcDDMResolution->GetItemCount() - 1, false );
		if( (int)( m_cResolutions[i] >> 16 ) == cCurrent.m_nWidth && (int)( m_cResolutions[i] & 0xffff ) ==
			cCurrent.m_nHeight ) 
			pcDDMResolution->SetSelection( pcDDMResolution->GetItemCount() - 1, false );
	}
	
	// Save matching color spaces
	m_cColorSpaces.clear();
	pcDDMColour->Clear();
	for( int i = 0; i < m_nModeCount; i++ ) 
	{
		bool bFound = false;
		if( m_pcModes[i].m_nWidth == cCurrent.m_nWidth && m_pcModes[i].m_nHeight == cCurrent.m_nHeight ) { 
			uint32 nSpace = (uint32)m_pcModes[i].m_eColorSpace;
			for( uint16 j = 0; j < m_cColorSpaces.size(); j++ ) {
				if( m_cColorSpaces[j] == nSpace )
					bFound = true;
			}
			if( !bFound ) 
			{
				m_cColorSpaces.push_back( nSpace );
			}
		}
	}
	// Add them to the list
	for( uint16 i = 0; i < m_cColorSpaces.size(); i++ ) 
	{
		switch( m_cColorSpaces[i] )
		{
			case os::CS_RGB15:
				strcpy( pzScratch, "15 Bit" );
			break;
			case os::CS_RGB16:
				strcpy( pzScratch, "16 Bit" );
			break;
			case os::CS_RGB32:
				strcpy( pzScratch, "32 Bit" );
			break;
			default:
				strcpy( pzScratch, "Unknown" );
			break;
		}
		pcDDMColour->AppendItem( pzScratch );
		if( i == 0 )
			pcDDMColour->SetSelection( pcDDMColour->GetItemCount() - 1, false );
		if( m_cColorSpaces[i] == (uint32)cCurrent.m_eColorSpace ) 
			pcDDMColour->SetSelection( pcDDMColour->GetItemCount() - 1, false );
		
			
	}
	// Save and add matching refresh rates
	m_cRefreshRates.clear();
	pcDDMRefresh->Clear();
	bool bFirst = true;
	for( int i = 0; i < m_nModeCount; i++ ) 
	{
		if( m_pcModes[i].m_nWidth == cCurrent.m_nWidth && m_pcModes[i].m_nHeight == cCurrent.m_nHeight 
		 && m_pcModes[i].m_eColorSpace == cCurrent.m_eColorSpace ) { 
			sprintf( pzScratch, "%i Hz", (int)m_pcModes[i].m_vRefreshRate );
			m_cRefreshRates.push_back( (uint32)m_pcModes[i].m_vRefreshRate );
			pcDDMRefresh->AppendItem( pzScratch );
			if( bFirst ) {
				bFirst = false;
				pcDDMRefresh->SetSelection( pcDDMRefresh->GetItemCount() - 1, false );
			}
			if( m_pcModes[i].m_vRefreshRate == cCurrent.m_vRefreshRate )
				pcDDMRefresh->SetSelection( pcDDMRefresh->GetItemCount() - 1, false );
		}
	}
	
	// Populate workspace dropdown
	pcDDMWorkspace->Clear();
	pcDDMWorkspace->AppendItem("This workspace");
	pcDDMWorkspace->AppendItem("All workspaces");
	pcDDMWorkspace->SetSelection(0);
	
	
	delete( pzScratch );
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
  cCurrent = m_pcModes[0];
  // Update controls
  ShowData();
  ColourChange();
}

void MainWindow::ColourChange()
{
	int iPos = pcDDMColour->GetSelection();
	cCurrent.m_eColorSpace = ( os::color_space )m_cColorSpaces[iPos];
	ShowData();
}

void MainWindow::ResolutionChange()
{
	
  // Save the newly selected resolution
  int iPos = pcDDMResolution->GetSelection();
  cCurrent.m_nWidth = (int)( m_cResolutions[iPos] >> 16 );
  cCurrent.m_nHeight = (int)( m_cResolutions[iPos] & 0xffff );
  ShowData();
}

void MainWindow::RefreshChange() 
{
  // Save the newly selected refresh rate
  int iPos = pcDDMRefresh->GetSelection();
  cCurrent.m_vRefreshRate = (float)( m_cRefreshRates[iPos] );
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




