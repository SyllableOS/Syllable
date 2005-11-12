// Whisper -:-  (C)opyright 2001-2002 Kristian Van Der Vliet
// AEdit -:-    (C)opyright 2004 Jonas Jarvoll
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
#include "messages.h"

#include <gui/image.h>
#include <gui/imageview.h>
#include <storage/memfile.h>
#include <util/message.h>
#include <gui/window.h>

ButtonBar::ButtonBar(const Rect &cFrame, const char* zName) : View(cFrame, zName, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT)
{
    bIsAttached=false;
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

ImageButton* ButtonBar::AddButton(Image* pcImage, const char* pzName, Message* pcMessage)
{
	ImageButton* pcCurrentButton;
		
    // Get the current number of attached buttons
    int nNumButtons=vButtons.size();

    ImageButton* pcTempButton = new ImageButton(Rect( ( (BUTTON_WIDTH*nNumButtons)+1),0,(BUTTON_WIDTH*(nNumButtons+1)),BUTTON_HEIGHT+4),pzName,"",pcMessage,NULL, ImageButton::IB_TEXT_BOTTOM,true,false,true);
    pcTempButton->SetImage(pcImage);
    vButtons.push_back(pcTempButton);
    pcCurrentButton=vButtons[nNumButtons];

    AddChild(pcCurrentButton);
    
    return pcCurrentButton;
}
