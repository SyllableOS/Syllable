//  AEdit -:-  (C)opyright 2005 Jonas Jarvoll
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

#include "aboutdialog.h"
#include "appwindow.h"
#include "buffer.h"
#include "messages.h"
#include "resources/aedit.h"
#include "version.h"

#include <util/message.h>
#include <gui/stringview.h>

AboutDialog::AboutDialog(const Rect& cFrame, AEditWindow* pcParent) : Window(cFrame, "about_dialog", MSG_ERROR_TITLE, WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_RESIZABLE)
{
	pcTarget=pcParent;		// We need to know the parent window so we can send messages back to it

	// Create the AEdit image
	pcAEditImage=new BitmapImage();
	Resources cRes(get_image_id());
	ResStream* pcStream=cRes.GetResourceStream("about");

    if(pcStream!=NULL)
        pcAEditImage->Load( pcStream );

	// Get size of image
	Point size=pcAEditImage->GetSize();

	ImageView* pcAEditImageView = new ImageView(Rect(10, 5, 5+size.x, 5+size.y), "aedit_image", pcAEditImage);
	AddChild(pcAEditImageView);

	// Create version label
	pcVersionLabel=new StringView(Rect(size.x+30, 20, size.x+30+100, 35), "version_label", MSG_ABOUT_VERSION);
	AddChild(pcVersionLabel);

	// Create version number label
	pcVersionNumberLabel=new StringView(Rect(size.x+30+25, 35, size.x+30+100+25, 50), "version_number_label", AEDIT_VERSION);
	AddChild(pcVersionNumberLabel);

	// Create syllable label
	pcTextEditorLabel=new StringView(Rect(10+50, 5+size.y+15, 200, 10+size.y+15+15), "texteditor_label", MSG_ABOUT_LINE_1);
	AddChild(pcTextEditorLabel);
	pcTextEditorLabel=new StringView(Rect(10+40+5, 5+size.y+15+20, 200+5, 10+size.y+15+35), "texteditor_label", MSG_ABOUT_LINE_2);
	AddChild(pcTextEditorLabel);
	pcTextEditorLabel=new StringView(Rect(10+50, 5+size.y+15+40, 200, 10+size.y+15+50), "texteditor_label", MSG_ABOUT_LINE_3);
	AddChild(pcTextEditorLabel);

	// Create close button
	pcCloseButton=new Button(Rect(10+75, size.y+5+82, 100+75,size.y+5+82+25), "close_button", MSG_ABOUT_CLOSE, new Message(M_BUT_ABOUT_CLOSE));
	AddChild(pcCloseButton);
}

void AboutDialog::HandleMessage(Message* pcMessage)
{
	switch(pcMessage->GetCode())
	{
		case M_BUT_ABOUT_CLOSE:
		{
			// Close the About window
			Hide();

			if(pcTarget->pcCurrentBuffer!=NULL)
				pcTarget->pcCurrentBuffer->MakeFocus();

			break;
		}
	
	}
}

bool AboutDialog::OkToQuit(void)
{
	Message *msg=new Message(M_BUT_ABOUT_CLOSE);
	HandleMessage(msg);
	delete msg;
	return (false);
}

void AboutDialog::Raise()
{
	if(IsVisible())
		Show(false);

	Show(true);

	MakeFocus();
}
