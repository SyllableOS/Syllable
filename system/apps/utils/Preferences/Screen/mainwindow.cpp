// Screen Preferences :: (C)opyright 2002 Daryl Dudey
//                       (C)opyright Jonas Jarvoll 2005
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
#include <util/resources.h>
#include <gui/desktop.h>
#include <gui/guidefines.h>
#include <gui/image.h>
#include <gui/requesters.h>
#include "main.h"
#include "mainwindow.h"
#include "messages.h"
#include "resources/Screen.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", MSG_MAINWND_TITLE/*, os::WND_NOT_RESIZABLE*/)
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
  pcHLWorkspace->AddChild( new os::StringView(cRect, "SVWorkspace", MSG_MAINWND_DISPLAY_APPLYTO) );
  pcHLWorkspace->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLWorkspace->AddChild( pcDDMWorkspace = new os::DropdownMenu(cRect, "DDMWorkspace"));
  pcDDMWorkspace->SetMinPreferredSize(12);
  pcDDMWorkspace->SetReadOnly(true);
  pcVLSettings->AddChild(pcHLWorkspace);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f));
  
  // Populate workspace dropdown
  pcDDMWorkspace->AppendItem(MSG_MAINWND_DROPDOWN_APPLY_THIS);
  pcDDMWorkspace->AppendItem(MSG_MAINWND_DROPDOWN_APPLY_ALL);
  pcDDMWorkspace->SetSelection(0);

  // Resolution
  pcHLResolution = new os::HLayoutNode("HLResolution");
  pcHLResolution->AddChild( new os::StringView(cRect, "SVResolution", MSG_MAINWND_DISPLAY_RESOLUTION) );
  pcHLResolution->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLResolution->AddChild( pcDDMResolution = new os::DropdownMenu(cRect, "DDMResolution"));
  pcDDMResolution->SetMinPreferredSize(12);
  pcDDMResolution->SetReadOnly(true);
  pcDDMResolution->SetSelectionMessage(new os::Message(M_MW_RESOLUTION));
  pcDDMResolution->SetTarget(this);
  pcVLSettings->AddChild(pcHLResolution);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f));

  // Colour space
  pcHLColour = new os::HLayoutNode("HLColour");
  pcHLColour->AddChild( new os::StringView(cRect, "SVColour", MSG_MAINWND_DISPLAY_COLORDEPTH) );
  pcHLColour->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLColour->AddChild( pcDDMColour = new os::DropdownMenu(cRect, "DDMColour"));
  pcDDMColour->SetMinPreferredSize(12);
  pcDDMColour->SetReadOnly(true);
  pcDDMColour->SetSelectionMessage(new os::Message(M_MW_COLOUR));
  pcDDMColour->SetTarget(this);
  pcVLSettings->AddChild(pcHLColour);
  pcVLSettings->AddChild( new os::VLayoutSpacer("", 5.0f));

  // Refresh
  pcHLRefresh = new os::HLayoutNode("HLRefresh");
  pcHLRefresh->AddChild( new os::StringView(cRect, "SVRefresh", MSG_MAINWND_DISPLAY_REFRESHRATE) );
  pcHLRefresh->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLRefresh->AddChild( pcDDMRefresh = new os::DropdownMenu(cRect, "DDMRefresh"));
  pcDDMRefresh->SetMinPreferredSize(12);
  pcDDMRefresh->SetReadOnly();
  pcDDMRefresh->SetSelectionMessage(new os::Message(M_MW_REFRESH));
  pcDDMRefresh->SetTarget(this);
  pcVLSettings->AddChild(pcHLRefresh);

  // Align the controls
  pcVLSettings->SameWidth( "SVWorkspace", "SVResolution", "SVColour", "SVRefresh", NULL);
  pcVLSettings->SameWidth( "DDMWorkspace", "DDMResolution", "DDMColour", "DDMRefresh", NULL);

  // Create frameview to store settings
  pcFVSettings = new os::FrameView( cBounds, "FVSettings", MSG_MAINWND_DISPLAY, 0/*os::CF_FOLLOW_ALL*/);
  pcFVSettings->SetRoot(pcVLSettings);
  pcVLRoot->AddChild( pcFVSettings );
  
  // Create apply/revert/close buttons
  pcHLButtons = new os::HLayoutNode("HLButtons");
  pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));
  pcHLButtons = new os::HLayoutNode("HLButtons", 0.0f);
  pcHLButtons->AddChild( new os::HLayoutSpacer(""));
  pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message(M_MW_APPLY)) );
  pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLButtons->AddChild( pcBClose = new os::Button(cRect, "BClose", MSG_MAINWND_BUTTON_CLOSE, new os::Message(M_MW_CLOSE)) );
  pcHLButtons->SameWidth( "BApply", "BClose", NULL );
  pcVLRoot->AddChild(pcHLButtons);

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set default button and initial focus (if root)
  if (bRoot) {
	EnableAll();
    this->SetDefaultButton(pcBApply);
    pcDDMResolution->MakeFocus();
  }

  // Set read only if not root
  if (!bRoot) {
	DisableAll();
	pcBClose->SetEnable(true);
  }

  // Set tab order
  int iTabOrder = 0;
  pcDDMWorkspace->SetTabOrder(iTabOrder++);
  pcDDMResolution->SetTabOrder(iTabOrder++);
  pcDDMColour->SetTabOrder(iTabOrder++);
  pcBApply->SetTabOrder(iTabOrder++);
  pcBClose->SetTabOrder(iTabOrder++);
  
  // Query all screenmodes
  m_nModeCount = os::Application::GetInstance()->GetScreenModeCount();
  m_pcModes = new os::screen_mode[m_nModeCount];
  for( int i = 0; i < m_nModeCount; i++ ) {
  	os::Application::GetInstance()->GetScreenModeInfo( i, &m_pcModes[i] );
  }
  
  // Set Icon
  os::Resources cCol( get_image_id() );
  os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
  os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
  SetIcon( pcIcon->LockBitmap() );
  delete( pcIcon );

  // Get screen mode data
  cCurrentMode = os::Desktop().GetScreenMode();
  cOldMode = os::Desktop().GetScreenMode();

  // Show data
  ShowData();
  
  ResizeTo( pcLRoot->GetPreferredSize( false ) );
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
		if( (int)( m_cResolutions[i] >> 16 ) == cCurrentMode.m_nWidth && (int)( m_cResolutions[i] & 0xffff ) ==
			cCurrentMode.m_nHeight ) 
			pcDDMResolution->SetSelection( pcDDMResolution->GetItemCount() - 1, false );
	}
	
	// Save matching color spaces
	m_cColorSpaces.clear();
	pcDDMColour->Clear();
	for( int i = 0; i < m_nModeCount; i++ ) 
	{
		bool bFound = false;
		if( m_pcModes[i].m_nWidth == cCurrentMode.m_nWidth && m_pcModes[i].m_nHeight == cCurrentMode.m_nHeight ) { 
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
		/* Ugly Workaround to make localization work. Someone should have a look at it... */	
		os::String zBitWorkaround;
		zBitWorkaround = os::String( "" );

		switch( m_cColorSpaces[i] )
		{
			case os::CS_RGB15:
				strcpy( pzScratch, "15 " );
			break;
			case os::CS_RGB16:
				strcpy( pzScratch, "16 " );
			break;
			case os::CS_RGB32:
				strcpy( pzScratch, "32 " );
			break;
			default:
				strcpy( pzScratch, MSG_MAINWND_DROPDOWN_COLOUR_UNKNOWN.c_str() );
			break;
		}
		pcDDMColour->AppendItem( zBitWorkaround+pzScratch+MSG_MAINWND_DROPDOWN_COLOUR_BIT );
		if( i == 0 )
			pcDDMColour->SetSelection( pcDDMColour->GetItemCount() - 1, false );
		if( m_cColorSpaces[i] == (uint32)cCurrentMode.m_eColorSpace ) 
			pcDDMColour->SetSelection( pcDDMColour->GetItemCount() - 1, false );
		
			
	}
	// Save and add matching refresh rates
	m_cRefreshRates.clear();
	pcDDMRefresh->Clear();
	bool bFirst = true;
	for( int i = 0; i < m_nModeCount; i++ ) 
	{
		if( m_pcModes[i].m_nWidth == cCurrentMode.m_nWidth && m_pcModes[i].m_nHeight == cCurrentMode.m_nHeight 
		 && m_pcModes[i].m_eColorSpace == cCurrentMode.m_eColorSpace ) { 
			sprintf( pzScratch, MSG_MAINWND_DROPDOWN_HERTZ.c_str(), (int)m_pcModes[i].m_vRefreshRate );
			m_cRefreshRates.push_back( (uint32)m_pcModes[i].m_vRefreshRate );
			pcDDMRefresh->AppendItem( pzScratch );
			if( bFirst ) {
				bFirst = false;
				pcDDMRefresh->SetSelection( pcDDMRefresh->GetItemCount() - 1, false );
			}
			if( m_pcModes[i].m_vRefreshRate == cCurrentMode.m_vRefreshRate )
				pcDDMRefresh->SetSelection( pcDDMRefresh->GetItemCount() - 1, false );
		}
	}
	
	delete( pzScratch );
}

void MainWindow::Apply() 
{
  // See if this or all desktops
  if (pcDDMWorkspace->GetSelection()==0) {
    os::Desktop().SetScreenMode(&cCurrentMode);
  } else {
    for (uint32 i=0;i<32;i++) {
      os::Desktop(i).SetScreenMode(&cCurrentMode);
    }
  }
}

void MainWindow::ColourChange()
{
	int iPos = pcDDMColour->GetSelection();
	cCurrentMode.m_eColorSpace = ( os::color_space )m_cColorSpaces[iPos];
	ShowData();
}

void MainWindow::ResolutionChange()
{	
  // Save the newly selected resolution
  int iPos = pcDDMResolution->GetSelection();
  cCurrentMode.m_nWidth = (int)( m_cResolutions[iPos] >> 16 );
  cCurrentMode.m_nHeight = (int)( m_cResolutions[iPos] & 0xffff );
  ShowData();
}

void MainWindow::RefreshChange() 
{
  // Save the newly selected refresh rate
  int iPos = pcDDMRefresh->GetSelection();
  cCurrentMode.m_vRefreshRate = (float)( m_cRefreshRates[iPos] );
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

  case M_MW_CLOSE:
	OkToQuit();	
	break;

  case M_MW_APPLY:
  {	
	DisableAll();
    os::Alert* pcApplyAlert=new os::Alert(MSG_ALERTWND_CHANGERES, MSG_ALERTWND_CHANGERES_TEXT, os::Alert::ALERT_WARNING, MSG_ALERTWND_CHANGERES_YES.c_str(), MSG_ALERTWND_CHANGERES_NO.c_str(), NULL);
	// Set flags
	pcApplyAlert->SetFlags(pcApplyAlert->GetFlags() | os::WND_NO_CLOSE_BUT);
	pcApplyAlert->CenterInWindow(this);
	pcApplyAlert->Go(new os::Invoker(new os::Message(M_MW_APPLY_ALERT), this));
    break;
  }
  case M_MW_APPLY_ALERT:
  {	
    int32 nSelection;

	if(pcMessage->FindInt32("which", &nSelection))
		break;
			
	switch(nSelection)	
	{
	  case 0:  // User wants to proceed changing the resolution
	  {
	    SaveOldMode();		
		Apply();
		// Create Confirm message box
  		pcConfirmAlert=new os::Alert(MSG_ALERTWND_CONFIRMRES, MSG_ALERTWND_CONFIRMRES_TEXT, os::Alert::ALERT_QUESTION, MSG_ALERTWND_CONFIRMRES_YES.c_str(), MSG_ALERTWND_CONFIRMRES_NO.c_str(), NULL);
		// Set flags
	    pcConfirmAlert->SetFlags(pcConfirmAlert->GetFlags() | os::WND_NO_CLOSE_BUT);
//        pcConfirmAlert->CenterInWindow(this);
		pcConfirmAlert->CenterInScreen();
	    pcConfirmAlert->Go(new os::Invoker(new os::Message(M_MW_CONFIRM_ALERT), this));
	    pcConfirmAlert->MakeFocus();
   	    AddTimer(this, TimerId, TimerCount, false);
		break;
      }
	  default:
	  {
		EnableAll();
		break;
	  }
	}
	break;
  }
  case M_MW_CONFIRM_ALERT:
  {	
    int32 nSelection;

	if(pcMessage->FindInt32("which", &nSelection))
		break;

	// Remove the 15 seconds timer
	RemoveTimer(this, TimerId);	

	switch(nSelection)	
	{
	  case 0:  // Yes, the new resolution is working fine! Lets keep it
	    break;
	  default: // No something is not working, return to old resolution
	   RestoreOldMode();
       break;
	}

	// In any case we enable all buttons
	EnableAll();

	break;
  } 
  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool MainWindow::OkToQuit()
{
  os::Application::GetInstance()->PostMessage(os::M_QUIT);
  return true;
}

void MainWindow::EnableAll()
{
  pcDDMWorkspace->SetEnable(true);
  pcDDMResolution->SetEnable(true);
  pcDDMColour->SetEnable(true);
  pcDDMRefresh->SetEnable(true);
  pcBApply->SetEnable(true);
  pcBClose->SetEnable(true);
}

void MainWindow::DisableAll()
{
  pcDDMWorkspace->SetEnable(false);
  pcDDMResolution->SetEnable(false);
  pcDDMColour->SetEnable(false);
  pcDDMRefresh->SetEnable(false);
  pcBApply->SetEnable(false);
  pcBClose->SetEnable(false);
}

void MainWindow::SaveOldMode()
{ 
  cOldMode = os::Desktop().GetScreenMode(); 
}

void MainWindow::RestoreOldMode()
{
  memcpy(&cCurrentMode, &cOldMode, sizeof(cOldMode));
  ShowData();
  Apply();
}

void MainWindow::TimerTick( int nID )
{
  if(nID==TimerId) {
	// Delete the confirmation message box 
	pcConfirmAlert->Close();

    os::Message cT(M_MW_CONFIRM_ALERT);  // Fake a NO answer of the confirm box
	cT.AddInt32("which", 1);
	HandleMessage(&cT);
  }
}


