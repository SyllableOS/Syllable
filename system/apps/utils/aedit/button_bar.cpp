// Whisper -:-  (C)opyright 2001-2002 Kristian Van Der Vliet
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

#include "button_bar.h"

#include <util/message.h>
#include <gui/window.h>
#include <gui/view.h>

ButtonBar::ButtonBar(const Rect &cFrame, const char* zName) : View(cFrame, zName, CF_FOLLOW_ALL)
{
	bIsAttached=false;
	nNumButtons=-1;
}

ButtonBar::~ButtonBar()
{

}

Point ButtonBar::GetPreferredSize(bool bLargest) const
{
	return(cPrefSize);
}

void ButtonBar::FrameSized(const Point &cDelta)
{
	if(bIsAttached==true)
	{
		cPrefSize.x=(GetWindow()->GetBounds().Width());
	}
}

void ButtonBar::AttachedToWindow(void)
{
	bIsAttached=true;

	cPrefSize.x=(GetWindow()->GetBounds().Width());
	cPrefSize.y=BUTTON_HEIGHT+4;	// Max button height, plus two pixel frame
}

void ButtonBar::AddButton(uint8* pnData, uint nNumBytes, const char* pzName, Message* pcMessage)
{
	ImageButton* pcCurrentButton;

	// Increment the count
	nNumButtons++;

	// Now create & add the button
	if(nNumButtons==0)
	{
		vButtons.push_back(new ImageButton(Rect(1,3,BUTTON_WIDTH,BUTTON_HEIGHT),pzName,"",pcMessage));
		pcCurrentButton=vButtons[nNumButtons];
		pcCurrentButton->SetBitmap(pnData,nNumBytes);

		AddChild(pcCurrentButton);
	}
	else
	{
		vButtons.push_back(new ImageButton(Rect(((BUTTON_WIDTH*nNumButtons)+1),3,(BUTTON_WIDTH*(nNumButtons+1)),BUTTON_HEIGHT),pzName,"",pcMessage));
		pcCurrentButton=vButtons[nNumButtons];
		pcCurrentButton->SetBitmap(pnData,nNumBytes);

		AddChild(pcCurrentButton);
	}

	return;
}
