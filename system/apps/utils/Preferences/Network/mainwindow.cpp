// Network Preferences :: (C)opyright 2002 Daryl Dudey
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

#include <stdlib.h>
#include <stdio.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/requesters.h>
#include "mainwindow.h"
#include "messages.h"
#include "main.h"
#include "validator.h"

MainWindow::MainWindow(const os::Rect& cFrame) : os::Window(cFrame, "MainWindow", "Network", os::WND_NOT_RESIZABLE)
{
  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );
  
  // Create the names frameview and contents
  pcVLNames = new os::VLayoutNode("VLNames");
  pcVLNames->SetBorders( os::Rect(5,5,5,5) );

  pcHLHost = new os::HLayoutNode("HLHost");
  pcHLHost->AddChild( new os::HLayoutSpacer("") );
  pcHLHost->AddChild( new os::StringView(cRect, "SVHost", "Host") );
  pcHLHost->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f) );
  pcHLHost->AddChild( pcTVHost = new os::TextView(cRect, "TVHost", "") );
  pcHLHost->AddChild( new os::HLayoutSpacer("") );
  pcTVHost->SetMinPreferredSize(20,1);
  pcTVHost->SetMaxLength( C_CO_HOSTLEN );
  pcVLNames->AddChild(pcHLHost);
  pcVLNames->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );

  pcHLDomain = new os::HLayoutNode("HLDomain");
  pcHLDomain->AddChild( new os::HLayoutSpacer("") );
  pcHLDomain->AddChild( new os::StringView(cRect, "SVDomain", "Domain") );
  pcHLDomain->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f) );
  pcHLDomain->AddChild( pcTVDomain = new os::TextView(cRect, "TVDomain", "") );
  pcHLDomain->AddChild( new os::HLayoutSpacer("") );
  pcTVDomain->SetMinPreferredSize(20,1);
  pcTVDomain->SetMaxLength( C_CO_DOMAINLEN );
  pcVLNames->AddChild(pcHLDomain);
  pcVLNames->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );

  pcHLName1 = new os::HLayoutNode("HLName1");
  pcHLName1->AddChild( new os::HLayoutSpacer("") );
  pcHLName1->AddChild( new os::StringView(cRect, "SVName1", "Primary DNS") );
  pcHLName1->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f) );
  pcHLName1->AddChild( pcTVName1 = new os::TextView(cRect, "TVName1", "") );
  pcHLName1->AddChild( new os::HLayoutSpacer("") );
  pcTVName1->SetMinPreferredSize(20,1);
  pcTVName1->SetMaxLength( C_CO_IPLEN );
  pcVLNames->AddChild(pcHLName1);
  pcVLNames->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );

  pcHLName2 = new os::HLayoutNode("HLName2");
  pcHLName2->AddChild( new os::HLayoutSpacer("") );
  pcHLName2->AddChild( new os::StringView(cRect, "SVName2", "Secondary DNS") );
  pcHLName2->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f) );
  pcHLName2->AddChild( pcTVName2 = new os::TextView(cRect, "TVName2", "") );
  pcHLName2->AddChild( new os::HLayoutSpacer("") );
  pcTVName2->SetMinPreferredSize(20,1);
  pcTVName2->SetMaxLength( C_CO_IPLEN );
  pcVLNames->AddChild(pcHLName2);

  pcVLNames->SameWidth( "SVHost", "SVDomain", "SVName1", "SVName2", NULL);
  pcVLNames->SameWidth( "TVHost", "TVDomain", "TVName1", "TVName2", NULL);
  pcFVNames = new os::FrameView(cBounds, "FVNames", "Names", os::CF_FOLLOW_ALL);
  pcFVNames->SetRoot(pcVLNames);
  pcVLRoot->AddChild(pcFVNames);

  // Bit of padding
  pcVLRoot->AddChild( new os::VLayoutSpacer( "", 10.0f, 10.0f) );

  // Create the connections frameview and contents
  pcVLConnectionList =new os::VLayoutNode("VLConnectionList");
  pcVLConnectionList->SetBorders( os::Rect(5,5,5,5) );
  pcLVConnections = new os::ListView(cRect, "LVConnections", os::ListView::F_RENDER_BORDER | os::ListView::F_NO_HEADER, os::CF_FOLLOW_ALL);
  pcLVConnections->InsertColumn("Interfaces",1);
  pcVLConnectionList->AddChild(pcLVConnections); 
  pcVLConnectionButtons = new os::VLayoutNode("VLConnectionButton"); 
  pcVLConnectionButtons->SetBorders( os::Rect(5,5,5,5) );
  pcVLConnectionButtons->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );
  pcVLConnectionButtons->AddChild( pcBModify = new os::Button(cRect, "BModify", "Modify", new os::Message(M_MW_MODIFY)) );
  if (!bRoot) {
    pcBModify->SetLabel("View");
  }
  pcVLConnectionButtons->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );
  pcHLConnection = new os::HLayoutNode("HLConnection");
  pcHLConnection->SetBorders( os::Rect(5,5,5,5) );
  pcHLConnection->AddChild(pcVLConnectionList);
  pcHLConnection->AddChild(pcVLConnectionButtons); 
  pcFVConnections = new os::FrameView(cBounds, "FVConnections", "Network Interfaces", os::CF_FOLLOW_ALL);
  pcFVConnections->SetRoot(pcHLConnection);
  pcVLRoot->AddChild(pcFVConnections);
  
  // Create apply/revert/close buttons
  if (bRoot) {
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));
    pcHLButtons = new os::HLayoutNode("HLButtons");
    pcHLButtons->AddChild( new os::HLayoutSpacer(""));
    pcHLButtons->AddChild( pcBApply = new os::Button(cRect, "BApply", "Apply", new os::Message(M_MW_APPLY)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f));
    pcHLButtons->AddChild( pcBRevert = new os::Button(cRect, "BRevert", "Revert", new os::Message(M_MW_REVERT)) );
    pcHLButtons->SameWidth( "BApply", "BRevert", NULL );
    pcVLRoot->AddChild(pcHLButtons);
  }

  // Set root and add to window
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set default button and initial focus (if root)
  if (bRoot) {
    this->SetDefaultButton(pcBApply);
    pcTVHost->MakeFocus();
  }

  // Set tab order
  int iTabOrder = 0;
  if (bRoot) {
    pcTVHost->SetTabOrder(iTabOrder++);
    pcTVDomain->SetTabOrder(iTabOrder++);
    pcTVName1->SetTabOrder(iTabOrder++);
    pcTVName2->SetTabOrder(iTabOrder++);
    pcLVConnections->SetTabOrder(iTabOrder++);
    pcBModify->SetTabOrder(iTabOrder++);
    pcBRevert->SetTabOrder(iTabOrder++);
    pcBApply->SetTabOrder(iTabOrder++);
  } else {
    pcBModify->SetTabOrder(iTabOrder++);
  }

  // If non-root, only allow view
  if (!bRoot) {
    SetTitle("Network (View)");
    pcTVHost->SetEnable(false);
    pcTVDomain->SetEnable(false);
    pcTVName1->SetEnable(false);
    pcTVName2->SetEnable(false);
  }

  // Set modify window to null, so syllable doesnt try and release them and set all to not visible
  for (int i=0;i<C_CO_MAXADAPTORS;i++) {
    pcModifyWindow[i] = NULL;
    bModifyOpen[i] = false;
  }

  // Detect interfaces and show data
  pcConfig->Detect();
  ShowData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData() 
{
   // Add names
  pcTVHost->Set(pcConfig->pzHost);
  pcTVDomain->Set(pcConfig->pzDomain);

  // Add DNS servers
  pcTVName1->Set(pcConfig->pzName1);
  pcTVName2->Set(pcConfig->pzName2);

  ShowList();
}

void MainWindow::ShowList()
{
  // Add interfaces
  pcLVConnections->Clear();
  char *pzScratch = new char[C_CO_DESCLEN+10];
  os::ListViewStringRow *pcLVRow;
  for (int i=0;i<C_CO_MAXADAPTORS;i++) {
    if (pcConfig->pcAdaptors[i].bExists) {
      pcLVRow = new os::ListViewStringRow;
      strcpy( pzScratch, pcConfig->pcAdaptors[i].pzDescription );
      if (pcConfig->pcAdaptors[i].bEnabled) {
	strcat( pzScratch, " (Enabled)");
	pcLVRow->AppendString( pzScratch );
      } else {
	strcat (pzScratch, " (Disabled)");
	pcLVRow->AppendString( pzScratch );
      }
      pcLVConnections->InsertRow(pcLVRow, true);
    }
  }
  delete pzScratch;
}

void MainWindow::Apply() 
{
  // Check data is valid first
  if (ValidateIP((char *)pcTVName1->GetBuffer()[0].c_str())!=0) {

    os::Alert *pcError = new os::Alert("Invalid - Network", "The value entered in Primary DNS is invalid", os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE || os::WND_MODAL, "OK", NULL);
    pcError->Go();

  } else if(ValidateIP((char *)pcTVName2->GetBuffer()[0].c_str())!=0) {

    os::Alert *pcError = new os::Alert("Invalid - Network", "The value entered in Secondary DNS is invalid", os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE || os::WND_MODAL, "OK", NULL);
    pcError->Go();

  } else {
    // Save data 
    strcpy(pcConfig->pzHost, (char *)pcTVHost->GetBuffer()[0].c_str());
    strcpy(pcConfig->pzDomain, (char *)pcTVDomain->GetBuffer()[0].c_str());
    strcpy(pcConfig->pzName1, (char *)pcTVName1->GetBuffer()[0].c_str());
    strcpy(pcConfig->pzName2, (char *)pcTVName2->GetBuffer()[0].c_str());
    pcConfig->Save();
    pcConfig->Activate();
  }
  ShowList();
}

void MainWindow::HandleMessage(os::Message* pcMessage)
{
int iAdaptorID;

  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MW_MODIFY:
    if ((iAdaptorID = pcLVConnections->GetFirstSelected())>=0) {
      if (!bModifyOpen[iAdaptorID]) { 
	bModifyOpen[iAdaptorID]=true;
	os::Rect wPos = this->GetFrame();
	
	if (bRoot) {
	  wPos.left+=50; wPos.right=wPos.left+250; wPos.top+=30; wPos.bottom=wPos.top+140;
	} else {
	  wPos.left+=50; wPos.right=wPos.left+250; wPos.top+=30; wPos.bottom=wPos.top+110;
	}
	pcModifyWindow[iAdaptorID] = new ModifyConnectionWindow(wPos, &(pcConfig->pcAdaptors[iAdaptorID]), iAdaptorID);
	pcModifyWindow[iAdaptorID]->Show();
	pcModifyWindow[iAdaptorID]->MakeFocus();
      } else {
	pcModifyWindow[iAdaptorID]->MakeFocus();
      }
    }
    break;

  case M_MW_APPLY:
    Apply();
    break;

  case M_MW_REVERT:
    pcConfig->Load();
    pcConfig->Detect();
    ShowData();
    break;

  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool MainWindow::OkToQuit()
{
  bool bCanClose = true;

  // Check no modify windows are open
  for (int iAdaptorID=0;iAdaptorID<C_CO_MAXADAPTORS;iAdaptorID++) {
    if (bModifyOpen[iAdaptorID]) {
      bCanClose = false; break;
    }
  }

  if (bCanClose) {
    os::Application::GetInstance()->PostMessage(os::M_QUIT);
    return true;
  }

  return false;
}




