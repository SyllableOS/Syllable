//  Archiver 0.3 -:-  (C)opyright 2000-2001 Rick Caudill
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

// Code is based on Vanders AEdit code  Thanks Vanders!

#include <gui/view.h>
#include "imagebutton.cpp"
#include "buttonbar.h"

const unsigned int BUTTON_HEIGHT=40;
const unsigned int BUTTON_WIDTH=40;
const unsigned int RECENT_WIDTH=10;
using namespace os;

class ButtonBar : public View
{
public:
    ButtonBar(const Rect &cFrame, const char* cName);

private:

    ImageButton* sNewButton;
    ImageButton* sOpenButton;
    ImageButton* sExtractButton;
    ImageButton* sDeleteButton;
    ImageButton* sRecentButton;
};

ButtonBar::ButtonBar(const Rect &cFrame, const char* cName) : View(cFrame, cName, CF_FOLLOW_TOP | CF_FOLLOW_LEFT | CF_FOLLOW_LEFT, WID_WILL_DRAW | WID_CLEAR_BACKGROUND)
{

    //Email me if you want to know the math on this.  It is too long to explain here.

    sOpenButton=new ImageButton(Rect((BUTTON_WIDTH)-40,0,(BUTTON_WIDTH*2)-40,BUTTON_HEIGHT),"Open","Open",new Message(ID_OPEN));
    sOpenButton->SetBitmap(ButtonOpen, sizeof(ButtonOpen));
    sOpenButton->SetDrawingMode(DM_BLEND);
    AddChild(sOpenButton);

    sRecentButton=new ImageButton(Rect((RECENT_WIDTH)+30,0,(RECENT_WIDTH*2)+30,BUTTON_HEIGHT),"RECENT","RECENT", new Message(ID_RECENT));
    sRecentButton->SetBitmap(ButtonRecent,sizeof(ButtonRecent));
    AddChild(sRecentButton);

    sNewButton=new ImageButton(Rect((BUTTON_WIDTH*2)-25,0,(BUTTON_WIDTH*3)-25,BUTTON_HEIGHT),"New","New",new Message(ID_NEW));
    sNewButton->SetBitmap(ButtonNew, sizeof(ButtonNew));
    AddChild(sNewButton);

    sExtractButton = new ImageButton(Rect((BUTTON_WIDTH*4)-60,0,((BUTTON_WIDTH*5)-60),BUTTON_HEIGHT),"Extract","Extract",new Message(ID_EXTRACT));
    sExtractButton->SetBitmap(ButtonExtract,sizeof(ButtonExtract));
    AddChild(sExtractButton);

    sDeleteButton = new ImageButton(Rect((BUTTON_WIDTH*5)-55,0,((BUTTON_WIDTH*6)-55),BUTTON_HEIGHT),"Delete","Delete",new Message(ID_DELETE));
    sDeleteButton->SetBitmap(ButtonDelete,sizeof(ButtonDelete));
    AddChild(sDeleteButton);
}


