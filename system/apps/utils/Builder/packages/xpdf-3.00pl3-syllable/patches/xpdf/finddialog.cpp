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

#include "finddialog.h"
#include "XPDFViewer.h"

#include <gui/window.h>
#include <util/message.h>

namespace os
{

FindDialog::FindDialog(const Rect& cFrame, Window* pcParent) : Window(cFrame, "find_dialog", "Find", WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcParentWindow=pcParent;		// We need to know the parent window so we can send messages back to it

	// Create the Layoutviews
	pcMainLayoutView=new LayoutView(GetBounds(),"find_dialog_view", NULL,CF_FOLLOW_ALL);

	// Make the base Vertical LayoutNode
	pcHLayoutNode=new HLayoutNode("main_layout_node");

	// Create the InputNode
	pcInputNode=new VLayoutNode("input_layout_node");

	pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f));

	pcFindLabel=new StringView(Rect(0,0,0,0),"find_label","Find",ALIGN_LEFT,CF_FOLLOW_NONE,WID_WILL_DRAW);
	pcInputNode->AddChild(pcFindLabel);

	pcFindTextView= new TextView(Rect(0,0,0,0), "find_text_view", NULL, CF_FOLLOW_NONE);
	pcFindTextView->SetMinPreferredSize( 15, 1 );
	pcInputNode->AddChild(pcFindTextView);

	pcInputNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f));

	pcInputNode->SameWidth("find_label","find_text_view",NULL);

	// Add a spacer to force the edit box to the top
	pcInputNode->AddChild(new VLayoutSpacer("spacer", 1));

	// Add a spacer on the far left
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f));

	pcHLayoutNode->AddChild(pcInputNode);

	// Add a spacer between the Edit box & the Buttons
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f));

	// Create the ButtonNode
	pcButtonNode= new VLayoutNode("button_layout_node");

	pcFindButton=new Button(Rect(0,0,0,0), "find_button", "Find", new Message(MSG_FIND), CF_FOLLOW_NONE);
	pcFindButton->SetTarget( pcParentWindow );
	pcButtonNode->AddChild(new VLayoutSpacer("spacer" ) );
	pcButtonNode->AddChild(pcFindButton);
	pcButtonNode->AddChild(new VLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f ) );

	pcHLayoutNode->AddChild(pcButtonNode);

	// Add a spacer on the far right
	pcHLayoutNode->AddChild(new HLayoutSpacer("spacer", 5.0f, 5.0f, NULL, 1.0f));

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

	m_bShown = false;
}

bool FindDialog::OkToQuit()
{
	m_bShown = false;
	Hide();
	return( false );
}

void FindDialog::ShowDialog()
{
	if( !m_bShown )
	{
		CenterInWindow( pcParentWindow );
		Show();
		m_bShown = true;
	}
	MakeFocus();
}

os::String FindDialog::GetFindText()
{
	return( pcFindTextView->GetBuffer()[0] );
}

};
