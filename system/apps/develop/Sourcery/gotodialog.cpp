//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#include "gotodialog.h"
#include "messages.h"
using namespace os;

/****************************************
* Description: Simple goto dialog with advanced feature
* Author: Rick Caudill
* Date: Fri Mar 19 15:38:40 2004
****************************************/
GoToDialog::GoToDialog(Window* pcParent, bool bAdvance) : os::Window(Rect(0,0,250,75),"","Go To:",WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcMainLayout = NULL;
	pcParentWindow = pcParent;
	bAdvanced = bAdvance;

	_Layout();
}

GoToDialog::~GoToDialog()
{
}

/****************************************
* Description: Lays out the system
* Author: Rick Caudill
* Date: Fri Mar 19 15:39:23 2004
****************************************/
void GoToDialog::_Layout()
{
	if (pcMainLayout)
		delete pcMainLayout;
		
	pcMainLayout = new LayoutView(GetBounds(),"",NULL,CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT);
	pcHLMain = new HLayoutNode("HLMain");
	pcHLMain->SetBorders(Rect(5,5,5,5));
	
	pcVLMain = new VLayoutNode("VLMain");
	
	pcHLLine = new HLayoutNode("HLLine");
	pcHLLine->AddChild(pcLineStringView = new StringView(Rect(0,0,0,0),"String1","Line:",ALIGN_LEFT,CF_FOLLOW_NONE));
	pcHLLine->AddChild(new HLayoutSpacer("",5,5));
	pcHLLine->AddChild(pcLineTextView = new TextView(Rect(0,0,0,0),"TextView1","",CF_FOLLOW_NONE));
	pcHLLine->AddChild(new HLayoutSpacer("",5,5));
	pcLineTextView->SetMultiLine(false);
	pcLineTextView->SetNumeric(true);
	
	if (bAdvanced == true)
	{
		pcHLCol = new HLayoutNode("HLCol");
		pcHLCol->AddChild(pcColStringView = new StringView(Rect(0,0,0,0),"String2","Column:",ALIGN_LEFT,CF_FOLLOW_NONE));
		pcHLCol->AddChild(new HLayoutSpacer("",5,5));
		pcHLCol->AddChild(pcColTextView = new TextView(Rect(0,0,0,0),"TextView2","",CF_FOLLOW_NONE));
		pcHLCol->AddChild(new HLayoutSpacer("",5,5));
		pcColTextView->SetMultiLine(false);
		pcColTextView->SetNumeric(true);
	}
	else
		pcHLLine->SetWeights(1000.0f,"TextView1",NULL);
	pcHLCheck = new HLayoutNode("HLCheck");
	pcHLCheck->AddChild(pcAdvancedCheckBox=new CheckBox(Rect(0,0,0,0),"","_Advanced",new Message(M_GOTO_CHECK)));
	pcAdvancedCheckBox->SetValue(bAdvanced);
	
	pcVLButton = new VLayoutNode("VLButton");
	pcVLButton->AddChild(pcGoToButton = new Button(Rect(0,0,0,0),"goto","_Go To...",new Message(M_GOTO_GOTO),CF_FOLLOW_NONE));
	pcVLButton->AddChild(new VLayoutSpacer("",5,5));
	pcVLButton->AddChild(pcCancelButton = new Button(Rect(0,0,0,0),"cancel","_Cancel",new Message(M_GOTO_CANCEL),CF_FOLLOW_NONE));
	pcVLButton->SameWidth("cancel","goto",NULL);	
	
	pcHLMain->AddChild(new HLayoutSpacer("",5,5));
	pcVLMain->AddChild(pcHLLine);
	if (bAdvanced == true)
		pcVLMain->AddChild(pcHLCol);
	
	pcVLMain->AddChild(pcHLCheck);
	
	if (bAdvanced)
		pcVLMain->SameWidth("String1","String2",NULL);
	
	if (bAdvanced == true)
		pcVLMain->SameWidth("TextView1, TextView2",NULL);
	
	pcHLMain->AddChild(pcVLMain);
	pcHLMain->AddChild(pcVLButton);
	pcHLMain->SameHeight("VLMain","VLButton",NULL);
	pcMainLayout->SetRoot(pcHLMain);
	AddChild(pcMainLayout);
	_SetTabs();
}

/****************************************
* Description: Sets the way tabbing works
* Author: Rick Caudill
* Date: Fri Mar 19 15:39:57 2004
****************************************/
void GoToDialog::_SetTabs()
{
	SetFocusChild(pcLineTextView);
	SetDefaultButton(pcGoToButton);
	
	if (bAdvanced)
	{
		pcLineTextView->SetTabOrder(NEXT_TAB_ORDER);
		pcColTextView->SetTabOrder(NEXT_TAB_ORDER);
		pcAdvancedCheckBox->SetTabOrder(NEXT_TAB_ORDER);
		pcGoToButton->SetTabOrder(NEXT_TAB_ORDER);
		pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	}
	else
	{
		pcLineTextView->SetTabOrder(NEXT_TAB_ORDER);
		pcAdvancedCheckBox->SetTabOrder(NEXT_TAB_ORDER);
		pcGoToButton->SetTabOrder(NEXT_TAB_ORDER);
		pcCancelButton->SetTabOrder(NEXT_TAB_ORDER);
	}
}

/************************************
* Description: Called when the window is closed
* Author: Rick Caudill
* Date: Fri Mar 19 15:40:31 2004
****************************************/
bool GoToDialog::OkToQuit()
{
	Close();
	return true;
}

/****************************************
* Description:  Handles the messages passed in the window
* Author: Rick Caudill
* Date: Fri Mar 19 15:41:18 2004
****************************************/
void GoToDialog::HandleMessage(os::Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_GOTO_CANCEL:  //canceling the dialog
		{
			OkToQuit();
			break;
		}
		
		case M_GOTO_CHECK: //clicked advance checkbox
		{
			if (bAdvanced) //if checked then uncheck
				bAdvanced = false;
			else //check
				bAdvanced = true;
			_Layout();	
		}
		break;
		
		case M_GOTO_GOTO:  //clicked the goto button now lets pass the information
		{
			if (bAdvanced && pcColTextView->GetBuffer()[0].c_str() != "")
				pcMessage->AddInt32("goto_col",atoi(pcColTextView->GetBuffer()[0].c_str()));
			else
				pcMessage->AddInt32("goto_col",0);
				
			if (pcLineTextView->GetBuffer()[0].c_str() != "")
				pcMessage->AddInt32("goto_line",atoi(pcLineTextView->GetBuffer()[0].c_str()));
			else
				pcMessage->AddInt32("goto_line",1);
				
			pcParentWindow->PostMessage(pcMessage,pcParentWindow);
		}
		break;
	}
}



