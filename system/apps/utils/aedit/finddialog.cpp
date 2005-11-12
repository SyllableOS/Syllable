//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//             (C)opyright 2004 Jonas Jarvoll
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

#include "finddialog.h"
#include "appwindow.h"
#include "buffer.h"
#include "messages.h"
#include "resources/aedit.h"

#include <gui/window.h>
#include <util/message.h>

FindDialog::FindDialog(const Rect& cFrame, AEditWindow* pcParent) : Window(cFrame, "find_dialog", MSG_FIND_TITLE, WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcTarget=pcParent;		// We need to know the parent window so we can send messages back to it

	// Create the Layoutviews
	pcMainLayoutView=new LayoutView(GetBounds(),"", NULL,CF_FOLLOW_ALL);

	// Make the base Vertical LayoutNode
	pcHLayoutNode=new HLayoutNode("main_layout_node");

	// Create the InputNode
	pcInputNode=new VLayoutNode("input_layout_node");

	pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcInputNode, 1.0f));

	pcFindLabel=new StringView(Rect(0,0,0,0),"find_label",MSG_FIND_FIND_LABEL,ALIGN_LEFT,CF_FOLLOW_NONE,WID_WILL_DRAW);
	pcInputNode->AddChild(pcFindLabel);
	
	pcFindTextView= new TextView(Rect(0,0,0,0), "find_text_view", NULL, CF_FOLLOW_NONE);
	pcFindTextView->SetMinPreferredSize( 15, 1 );
	pcInputNode->AddChild(pcFindTextView);

	pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcInputNode, 1.0f));

	pcCaseCheckBox=new CheckBox(Rect(0,0,0,0),"case_checkbox", MSG_FIND_CASE_SENSITIVE, new Message(M_VOID), CF_FOLLOW_NONE, WID_WILL_DRAW);
	pcInputNode->AddChild(pcCaseCheckBox);

	pcInputNode->SameWidth("find_label","find_text_view","case_checkbox",NULL);

	// Add a spacer to force the edit box to the top
	pcInputNode->AddChild(new VLayoutSpacer("spacer", 1));

	// Add a spacer on the far left
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	pcHLayoutNode->AddChild(pcInputNode);

	// Add a spacer between the Edit box & the Buttons
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	// Create the ButtonNode
	pcButtonNode= new VLayoutNode("button_layout_node");

	pcFindButton=new Button(Rect(0,0,0,0), "find_button", MSG_FIND_FIND, new Message(M_BUT_FIND_GO), CF_FOLLOW_NONE);
	pcButtonNode->AddChild(pcFindButton);
	pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

	pcNextButton=new Button(Rect(0,0,0,0), "find_next_button", MSG_FIND_FIND_NEXT, new Message(M_BUT_FIND_NEXT), CF_FOLLOW_NONE);
	pcButtonNode->AddChild(pcNextButton);
	pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

	pcCloseButton=new Button(Rect(0,0,0,0), "close_button", MSG_FIND_CLOSE, new Message(M_BUT_FIND_CLOSE), CF_FOLLOW_NONE);
	pcButtonNode->AddChild(pcCloseButton);
	pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

	pcButtonNode->SameWidth("find_button","find_next_button","close_button", NULL);

	pcHLayoutNode->AddChild(pcButtonNode);

	// Add a spacer on the far right
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	// Add the LayoutNodes to the layoutview
	pcMainLayoutView->SetRoot(pcHLayoutNode);

	// Add the View to the Window
	AddChild(pcMainLayoutView);

	// Set the focus
	SetFocusChild(pcFindTextView);
	SetDefaultButton(pcFindButton);

	Point cTopLeft = GetBounds().LeftTop();
	Point cSize = pcMainLayoutView->GetPreferredSize( false );
	Rect cWFrame( cTopLeft.x, cTopLeft.y, cTopLeft.x + cSize.x, cTopLeft.y + cSize.y );
	SetFrame( cWFrame );
}

void FindDialog::HandleMessage(Message* pcMessage)
{
	switch(pcMessage->GetCode())
	{
		case M_BUT_FIND_GO:
		{
			if(pcTarget->pcCurrentBuffer!=NULL)
			{
				std::string text=pcFindTextView->GetBuffer()[0];

				if(pcTarget->pcCurrentBuffer->FindFirst(text, (pcCaseCheckBox->GetValue()? true : false)))
				{
					char buffer[1024];
					sprintf(buffer,MSG_STATUSBAR_SEARCH_FOUND.c_str(), text.c_str());

					pcTarget->SetStatus(string(buffer));
				}
				else
				{
					char buffer[1024];
					sprintf(buffer,MSG_STATUSBAR_SEARCH_NOT_FOUND_1.c_str(), text.c_str());

					pcTarget->SetStatus(string(buffer));
				}
			}
			break;
		}

		case M_BUT_FIND_NEXT:
		{
			if(pcTarget->pcCurrentBuffer!=NULL)
			{
				std::string text;

				text=pcTarget->pcCurrentBuffer->FindNext();

				if(text!="")
				{
					char buffer[1024];
					sprintf(buffer,MSG_STATUSBAR_SEARCH_FOUND.c_str(), text.c_str());

					pcTarget->SetStatus(string(buffer));
				}
				else
				{
					pcTarget->SetStatus(MSG_STATUSBAR_SEARCH_NOT_FOUND_2);
				}
			}
			break;
		}

		case M_BUT_FIND_CLOSE:
		{
			// Hide the Window
			Hide();

			break;
		}

		default:
		{
			pcTarget->PostMessage(pcMessage,pcTarget);	// Send the messages from the dialog back to the parent window
			break;
		}
	}
}

// Disable/enable buttons in dialog depending on the parameter
void FindDialog::SetEnable(bool bEnable)
{
	pcFindButton->SetEnable(bEnable);
	pcNextButton->SetEnable(bEnable);
}

bool FindDialog::OkToQuit(void)
{
	Message *msg=new Message(M_BUT_FIND_CLOSE);
	HandleMessage(msg);
	delete msg;
	return (false);
}

void FindDialog::MakeFocus()
{
	Window::MakeFocus();
	// Set cursor on the text entry
	pcFindTextView->MakeFocus();
}

void FindDialog::Raise()
{
	if(IsVisible())
		Show(false);

	Show(true);

	MakeFocus();
}

