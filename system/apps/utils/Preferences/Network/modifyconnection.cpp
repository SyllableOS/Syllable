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
#include <gui/requesters.h>
#include "modifyconnection.h"
#include "messages.h"
#include "main.h"
#include "validator.h"

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
  pcVLRoot->SetBorders( os::Rect(10,5,10,5) );

  // Show adaptor description
  pcHLDesc = new os::HLayoutNode("HLDesc");
  pcHLDesc->AddChild( new os::HLayoutSpacer("") );
  pcHLDesc->AddChild( new os::StringView(cRect, "AdaptorDesc1", "Interface : " ) );
  pcHLDesc->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
  pcHLDesc->AddChild( new os::StringView(cRect, "AdaptorDesc2", pcAdaptor->pzDescription ) );
  pcHLDesc->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
  pcHLDesc->AddChild( pcSVAdaptor = new os::StringView(cRect, "", "") );
  pcSVAdaptor->SetMinPreferredSize(10);
  if (pcAdaptor->bEnabled) {
    pcSVAdaptor->SetString("(Enabled)");
  } else {
    pcSVAdaptor->SetString("(Disabled)");
  }
  pcHLDesc->AddChild( new os::HLayoutSpacer("") );
  pcVLRoot->AddChild( pcHLDesc );
  pcVLRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

  // Create Vertical layout for settings
  pcVLSettings = new os::VLayoutNode("VLSettings");
  pcVLSettings->SetBorders( os::Rect(5,5,5,5) );

  // Create IP Address layout
  pcHLIP = new os::HLayoutNode("HLIP");
  pcHLIP->AddChild( new os::HLayoutSpacer("") );
  pcHLIP->AddChild( new os::StringView(cRect, "SVIPAddr", "IP Address") );
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
  pcHLSN->AddChild( new os::StringView(cRect, "SVSubNet", "Subnet Mask") );
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
  pcHLGW->AddChild( new os::StringView(cRect, "SVGateway", "Gateway Address") );
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

  // Allow the user to switch DHCP On or Off
  pcCBDhcp = new os::CheckBox( os::Rect( 0,0,0,0 ), "CBDhcp", "_Use DHCP", new os::Message( M_MC_DHCP ) );
  pcVLRoot->AddChild( pcCBDhcp );
 
  // Add buttons to enable/disable
  pcHLButtons = new os::HLayoutNode("HLButtons");
  pcHLButtons->AddChild( new os::HLayoutSpacer("") );
  if (bRoot) {
    pcVLRoot->AddChild( new os::VLayoutSpacer("", 10.0f, 10.0f));
    pcHLButtons = new os::HLayoutNode("HLButtons");
    pcHLButtons->AddChild( pcBEnable = new os::Button(cRect, "BEnable", "_Enable", new os::Message(M_MC_ENABLE)) );
    pcHLButtons->AddChild( new os::HLayoutSpacer("", 5.0f, 5.0f) );
    pcHLButtons->AddChild( pcBDisable = new os::Button(cRect, "BDisable", "_Disable", new os::Message(M_MC_DISABLE)) );
    pcHLButtons->SameWidth( "BEnable", "BDisable", NULL );
    pcVLRoot->AddChild(pcHLButtons);
  }
  
  // Set the root 
  pcLRoot->SetRoot(pcVLRoot);
  AddChild(pcLRoot);

  // Set initial focus (if root)
  if (bRoot) {
    pcTVIPAddr->MakeFocus();
  }

  // Set tab order
  int iTabOrder = 0;
  pcTVIPAddr->SetTabOrder(iTabOrder++);
  pcTVSubNet->SetTabOrder(iTabOrder++);
  pcTVGateway->SetTabOrder(iTabOrder++);
  pcCBDhcp->SetTabOrder(iTabOrder++);
  if (bRoot) {
    pcBEnable->SetTabOrder(iTabOrder++);
    pcBDisable->SetTabOrder(iTabOrder++);
  }

  // Set read only if not root
  if (!bRoot) {
    SetTitle("Modify - Network (View)");
    //pcCBConnection->SetEnable(false);
    pcTVIPAddr->SetEnable(false);
    pcTVSubNet->SetEnable(false);
    pcTVGateway->SetEnable(false);
    pcCBDhcp->SetEnable(false);
  }

  // Show data
  ShowData();
}

ModifyConnectionWindow::~ModifyConnectionWindow()
{
  // If root,
  if (bRoot) {
    SaveValues();
  }
}

void ModifyConnectionWindow::ShowData() 
{
  // Load in values for controls
  if (bRoot) {
    pcBEnable->SetEnable( !pcAdaptor->bEnabled );
    pcBDisable->SetEnable( pcAdaptor->bEnabled );
    pcCBDhcp->SetValue( pcAdaptor->bUseDHCP );
  }
  
  // IP Info
  pcTVIPAddr->Set( pcAdaptor->pzIP );
  pcTVSubNet->Set( pcAdaptor->pzSN );
  pcTVGateway->Set( pcAdaptor->pzGW );
}

void ModifyConnectionWindow::SaveValues()
{
  pcAdaptor->bEnabled = !pcBEnable->IsEnabled();
  pcAdaptor->bUseDHCP = pcCBDhcp->GetValue().AsBool();
  strcpy( pcAdaptor->pzIP, pcTVIPAddr->GetBuffer()[0].c_str());
  strcpy( pcAdaptor->pzSN, pcTVSubNet->GetBuffer()[0].c_str());
  strcpy( pcAdaptor->pzGW, pcTVGateway->GetBuffer()[0].c_str());
}

void ModifyConnectionWindow::HandleMessage(os::Message* pcMessage)
{
  // Get message code and act on it
  switch(pcMessage->GetCode()) {

  case M_MC_ENABLE:
    pcBEnable->SetEnable(false);
    pcBDisable->SetEnable(true);
    pcSVAdaptor->SetString("(Enabled)");
    break;

  case M_MC_DISABLE:
    pcBEnable->SetEnable(true);
    pcBDisable->SetEnable(false);
    pcSVAdaptor->SetString("(Disabled)");
    break;

  case M_MC_DHCP:
  {
    if( pcCBDhcp->GetValue().AsBool() )	// DHCP enabled
    {
      pcTVIPAddr->SetEnable(false);
      pcTVSubNet->SetEnable(false);
      pcTVGateway->SetEnable(false);
    }
    else		// DHCP disabled
    {
      pcTVIPAddr->SetEnable(true);
      pcTVSubNet->SetEnable(true);
      pcTVGateway->SetEnable(true);
    }
    break;
  }

  default:
    os::Window::HandleMessage(pcMessage); break;
  }

}

bool ModifyConnectionWindow::OkToQuit()
{
  // If not root, just close the window, dont check values
  if (!bRoot) {
    bModifyOpen[iAdaptorID] = false;
    return true;
  }

  // Check everything is valid (if root)
  if (ValidateIP(pcTVIPAddr->GetBuffer()[0].c_str())!=0) {
    
    os::Alert *pcError = new os::Alert("Invalid - Network", "The value entered in IP Address is invalid", os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE || os::WND_MODAL, "_OK", NULL);
    pcError->Go();
    return false;
    
  } else if(ValidateIP(pcTVSubNet->GetBuffer()[0].c_str())!=0) {

    os::Alert *pcError = new os::Alert("Invalid - Network", "The value entered in Subnet Mask is invalid", os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE || os::WND_MODAL, "_OK", NULL);
    pcError->Go();
    return false;

  } else if(ValidateIP(pcTVGateway->GetBuffer()[0].c_str())!=0) {

    os::Alert *pcError = new os::Alert("Invalid - Network","The value entered in Gateway is invalid", os::Alert::ALERT_WARNING, os::WND_NOT_RESIZABLE || os::WND_MODAL, "_OK", NULL);
    pcError->Go();
    return false;

  } else {
    bModifyOpen[iAdaptorID] = false;
    return true;
  }
}











