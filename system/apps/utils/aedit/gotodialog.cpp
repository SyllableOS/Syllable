//  AEdit -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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
#include "resources/aedit.h"

#include <util/message.h>

GotoDialog::GotoDialog(const Rect& cFrame, Window* pcParent) : Window(cFrame, "goto_dialog", MSG_GOTO_TITLE, WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcParentWindow=pcParent;		// We need to know the parent window so we can send messages back to it

	// Create the Layoutviews
	pcMainLayoutView=new LayoutView(GetBounds(),"", NULL, CF_FOLLOW_ALL);

	// Make the base Vertical LayoutNode
	pcHLayoutNode=new HLayoutNode("main_layout_node");

	// Add a spacer on the far left
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	// Create the InputNode
	pcHLayoutNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	pcLineNoTextView= new TextView(Rect(0,0,0,0), "find_text_view", NULL, CF_FOLLOW_NONE);
	pcLineNoTextView->SetNumeric(true);
	pcLineNoTextView->SetMinPreferredSize( 10, 1 );
	pcHLayoutNode->AddChild(pcLineNoTextView);

	// Add a spacer to force the edit box to the top
	pcHLayoutNode->AddChild(new VLayoutSpacer("spacer", 1));

	// Add a spacer between the Edit box & the Buttons
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	// Create the ButtonNode
	pcButtonNode= new VLayoutNode("button_layout_node");

	pcGotoButton=new Button(Rect(0,0,0,0), "goto_button", MSG_GOTO_GOTO, new Message(M_BUT_GOTO_GOTO), CF_FOLLOW_NONE);
	pcButtonNode->AddChild(pcGotoButton);

	pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, pcButtonNode, 1.0f ) );

	pcCloseButton=new Button(Rect(0,0,0,0), "close_button", MSG_GOTO_CLOSE, new Message(M_BUT_GOTO_CLOSE), CF_FOLLOW_NONE);
	pcButtonNode->AddChild(pcCloseButton);

	pcButtonNode->SameWidth("goto_button","close_button", NULL);

	// Add the ButtonNode to the layout
	pcHLayoutNode->AddChild(pcButtonNode);

	// Add a spacer on the far right
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, pcHLayoutNode, 1.0f));

	// Add the LayoutNodes to the layoutview
	//pcMainLayoutView->AddChild(pcHLayoutNode);
	pcMainLayoutView->SetRoot(pcHLayoutNode);
	
	// Add the View to the Window
	AddChild(pcMainLayoutView);

	// Set the focus
	SetFocusChild(pcLineNoTextView);
	SetDefaultButton(pcGotoButton);

	Point cTopLeft = GetBounds().LeftTop();
	Point cSize = pcMainLayoutView->GetPreferredSize( false );
	Rect cWFrame( cTopLeft.x, cTopLeft.y, cTopLeft.x + cSize.x, cTopLeft.y + cSize.y );
	SetFrame( cWFrame );
}

void GotoDialog::HandleMessage(Message* pcMessage)
{
	switch(pcMessage->GetCode())
	{
		case M_BUT_GOTO_GOTO:
		{	
			pcMessage->AddInt32("goto_lineno",atoi(pcLineNoTextView->GetBuffer()[0].c_str()));
			pcParentWindow->PostMessage(pcMessage,pcParentWindow);	// Send the messages from the dialog back to the parent window

			break;
		}

		default:
		{
			pcParentWindow->PostMessage(pcMessage,pcParentWindow);	// Send the messages from the dialog back to the parent window
			break;
		}
	}
}
