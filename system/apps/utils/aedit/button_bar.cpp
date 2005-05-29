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
#include "messages.h"

#include <gui/image.h>
#include <gui/imageview.h>
#include <storage/memfile.h>
#include <util/message.h>
#include <gui/window.h>

ButtonBar::ButtonBar(const Rect &cFrame, const char* zName) : View(cFrame, zName, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT)
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

	LayoutButtons();

    cPrefSize.x=(GetWindow()->GetBounds().Width());
//    cPrefSize.y=BUTTON_HEIGHT+4;	// Max button height, plus two pixel frame
}

void ButtonBar::LayoutButtons( void )
{
	uint i;

	cPrefSize.x = 0;
	cPrefSize.y = 0;

	for( i = 0; i < vButtons.size(); i++ ) {
		ImageButton* ib = vButtons[i];
		
		if( ib->GetPreferredSize( false ).y > cPrefSize.y )
			cPrefSize.y = ib->GetPreferredSize( false ).y;
	}


	for( i = 0; i < vButtons.size(); i++ ) {
		ImageButton* ib = vButtons[i];
		
		ib->SetFrame( Rect( cPrefSize.x, 0, cPrefSize.x + ib->GetPreferredSize( false ).x, cPrefSize.y ) );

		cPrefSize.x += ib->GetPreferredSize( false ).x + 1;
	}
}

void ButtonBar::AddButton(const char* pzIcon, const char* pzName, Message* pcMessage)
{
	BitmapImage* pcImage = NULL;
    os::Resources cRes(get_image_id());
	os::ResStream* pcStream = cRes.GetResourceStream(pzIcon);
	if(pcStream != NULL) {
		pcImage = new BitmapImage( pcStream );

		Rect	frame = pcImage->GetBounds();
	} else {
		printf("Failed to load image resource: \"%s\" for %s\n", pzIcon, pzName );
	}
	delete pcStream;

    // Increment the count
    nNumButtons++;

	ImageButton* pcTempButton = new ImageButton(Rect(),pzName,"",pcMessage,NULL, ImageButton::IB_TEXT_BOTTOM,true,false,true);

	if( pcImage ) {
		pcTempButton->SetImage(pcImage);
	}

	vButtons.push_back(pcTempButton);
	
	AddChild(pcTempButton);

	LayoutButtons();

    return;
}

