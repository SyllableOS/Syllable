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

#include <gui/textview.h>
#include <gui/checkbox.h>
#include <util/application.h>
#include <util/message.h>
#include "modifyconnection.h"
#include "messages.h"
#include "main.h"
#include "validator.h"
#include "errorwindow.h"

// Boolean to signal if window is already open
bool bModifyOpen[C_CO_MAXADAPTORS];

ModifyConnectionWindow::ModifyConnectionWindow(const os::Rect& cFrame, AdaptorConfiguration *pcAdaptorIn, int iAdaptorIDIn) : os::Window(cFrame, "ModifyConnectionWindow", "Modify - Network" ,os::WND_NOT_RESIZABLE)
{
  pcAdaptor = pcAdaptorIn;
  iAdaptorID = iAdaptorIDIn;

  os::Rect cBounds = GetBounds();
  os::Rect cRect = os::Rect(0,0,0,0);

  // Create the layouts/views
  pcLRoot = new os::LayoutView(cBounds, "L", NULL, os::CF_FOLLOW_ALL);
  pcVLRoot = new os::VLayoutNode("VL");
  pcVLRoot->SetBorders( os::Rect(10,10,10,10) );

  // Show adaptor description
  pcHLDesc = new os::HLayoutNode("HLDesc");
  pcHLDesc->AddChild( new os::HLayoutSpacer("") );
  pcHLDesc->AddChild( new os::StringView(cRect, "AdaptorDesc1", "Interface : " ) );
  pcHLDesc->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
  pcHLDesc->AddChild( new os::StringView(cRect, "AdaptorDesc2", pcAdaptor->pzDescription ) );
  pcHLDesc->AddChild( new os::HLayoutSpacer("") );
  pcVLRoot->AddChild( pcHLDesc );
  pcVLRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
			     
  // Show connection on/off controls
  if (bRoot) {
    pcVLRoot->AddChild( pcCBConnection = new os::CheckBox(cRect, "CBConnection", "Connection Enabled", new os::Message(M_MC_CONNECTIONENABLE)) );
  } else {
    if (pcAdaptor->bEnabled) {
      pcVLRoot->AddChild( new os::StringView(cRect, "", "Interface Enabled") );
    } else {
      pcVLRoot->AddChild( new os::StringView(cRect, "", "Interface Disabled") );
    }
  }
  pcVLRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
  
  // Show DHCP on/off controls (Note this is disables until we get DHCP support)
  /*
  pcVLRoot->AddChild( pcRBDHCPOn = new os::RadioButton(cRect, "RBDHCPOn", "Enable DHCP", new os::Message(M_MC_DHCPON)) );
  pcVLRoot->AddChild( pcRBDHCPOff = new os::RadioButton(cRect, "RBDHCPOff", "Disable DHCP", new os::Message(M_MC_DHCPOFF)) );
  */

  // Create Vertical layout for settings
  pcVLSettings = new os::VLayoutNode("VLSettings");
  pcVLSettings->SetBorders( os::Rect(5,5,5,5) );

  // Create IP Address layout
  pcHLIP = new os::HLayoutNode("HLIP");
  pcHLIP->AddChild( new os::HLayoutSpacer("") );
  pcHLIP->AddChild( pcSVIPAddr = new os::StringView(cRect, "SVIPAddr", "IP Address") );
  pcHLIP->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLIP->AddChild( pcTVIPAddr = new os::TextView(cRect, "TVIPAddr", "") );
  pcHLIP->AddChild( new os::HLayoutSpacer("") );
  pcTVIPAddr->SetMinPreferredSize( 12,1 );
  pcTVIPAddr->SetMaxLength( C_CO_IPLEN );
  pcVLSettings->AddChild( pcHLIP );
  pcVLSettings->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

  // Create Subnet mask layout
  pcHLSN = new os::HLayoutNode("HLSN");
  pcHLSN->AddChild( new os::HLayoutSpacer("") );
  pcHLSN->AddChild( pcSVSubNet = new os::StringView(cRect, "SVSubNet", "Subnet Mask") );
  pcHLSN->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLSN->AddChild( pcTVSubNet = new os::TextView(cRect, "TVSubNet", "") );
  pcHLSN->AddChild( new os::HLayoutSpacer("") );
  pcTVSubNet->SetMinPreferredSize( 12,1 );
  pcTVSubNet->SetMaxLength( C_CO_IPLEN );
  pcVLSettings->AddChild( pcHLSN );
  pcVLSettings->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

  // Create gateway layout
  pcHLGW = new os::HLayoutNode("HLGW");
  pcHLGW->AddChild( new os::HLayoutSpacer("") );
  pcHLGW->AddChild( pcSVGateway = new os::StringView(cRect, "SVGateway", "Gateway Address") );
  pcHLGW->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
  pcHLGW->AddChild( pcTVGateway = new os::TextView(cRect, "TVGateway", "") );
  pcHLGW->AddChild( new os::HLayoutSpacer("") );
  pcTVGateway->SetMinPreferredSize( 12,1 );
  pcTVGateway->SetMaxLength( C_CO_IPLEN );
  pcVLSettings->AddChild( pcHLGW );
  
  // Make sure all controls are same width
  pcVLSettings->SameWidth("TVIPAddr", "TVSubNet", "TVGateway", NULL);
  pcVLSettings->SameWidth("SVIPAddr", "SVSubNet", "SVGateway", NULL);
  pcFVSettings = new os::FrameView(cBounds, "FVSettings", "Settings", os::CF_FOLLOW_ALL);
  pcFVSettings->SetRoot(pcVLSettings);
  pcVLRoot->AddChild(pcFVSettings);
 
  // Add a spacer
  pcVLRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f) );

  // Set the root 
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set tab order
  int iTabOrder = 0;
  pcCBConnection->SetTabOrder(iTabOrder++);
  pcTVIPAddr->SetTabOrder(iTabOrder++);
  pcTVSubNet->SetTabOrder(iTabOrder++);
  pcTVGateway->SetTabOrder(iTabOrder++);

  // Set read only if not root
  if (!bRoot) {
    SetTitle("Modify - Network (View)");
    //pcCBConnection->SetEnable(false);
    pcTVIPAddr->SetReadOnly(true);
    pcTVSubNet->SetReadOnly(true);
    pcTVGateway->SetReadOnly(true);
  }

  // Show data
  ShowData();
}

ModifyConnectionWindow::~ModifyConnectionWindow()
{
  SaveValues();
}

void ModifyConnectionWindow::ShowData() 
{
  // Load in values for controls
  pcCBConnection->SetValue( pcAdaptor->bEnabled, false );
  //  pcRBDHCPOff->SetValue( true, false );
  
  // IP Info
  pcTVIPAddr->Set( pcAdaptor->pzIP );
  pcTVSubNet->Set( pcAdaptor->pzSN );
  pcTVGateway->Set( pcAdaptor->pzGW );
}

void ModifyConnectionWindow::SaveValues()
{
  pcAdaptor->bEnabled = pcCBConnection->GetValue().AsBool();
  strcpy( pcAdaptor->pzIP, pcTVIPAddr->GetBuffer()[0].c_str());
  strcpy( pcAdaptor->pzSN, pcTVSubNet->GetBuffer()[0].c_str());
  strcpy( pcAdaptor->pzGW, pcTVGateway->GetBuffer()[0].c_str());
}

void ModifyConnectionWindow::HandleMessage(os::Message* pcMessage)
{
  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MC_CONNECTIONENABLE: 
    break;
  
  case M_MC_DHCPON:
    pcFVSettings->Show(false); break;

  case M_MC_DHCPOFF:
    pcFVSettings->Show(true); break;

  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool ModifyConnectionWindow::OkToQuit()
{
  // If error window open, dont close
  if (bErrorWindowOpen) 
    return false;

  // Check everything is valid
  os::Rect wPos = this->GetFrame();
  wPos.left-=30; wPos.right=wPos.left+370; wPos.top+=30; wPos.bottom=wPos.top+55;
  
  if (ValidateIP(pcTVIPAddr->GetBuffer()[0].c_str())!=0) {
    ErrorWindow *pcError = new ErrorWindow("The value entered in IP Address is not valid, can not save", wPos);
    pcError->Show();
    pcError->MakeFocus();
    return false;
  } else if(ValidateIP(pcTVSubNet->GetBuffer()[0].c_str())!=0) {
    ErrorWindow *pcError = new ErrorWindow("The value entered in Subnet Mask is not valid, can not save", wPos);
    pcError->Show();
    pcError->MakeFocus();
    return false;
  } else if(ValidateIP(pcTVGateway->GetBuffer()[0].c_str())!=0) {
    ErrorWindow *pcError = new ErrorWindow("The value entered in Gateway is not valid, can not save", wPos);
    pcError->Show();
    pcError->MakeFocus();
    return false;
  } else {
    bModifyOpen[iAdaptorID] = false;
    return true;
  }
}










