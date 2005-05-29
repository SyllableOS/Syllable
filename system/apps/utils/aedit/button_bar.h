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

#ifndef __BUTTON_BAR_H_
#define __BUTTON_BAR_H_

#include <gui/view.h>

#include <vector>

#include <gui/imagebutton.h>
#include <gui/image.h>
#include <storage/memfile.h>
#include <iostream>
#include <stdio.h>

#define BUTTON_WIDTH 30
#define BUTTON_HEIGHT 30

using namespace os;
using namespace std;

typedef vector<ImageButton*> t_Buttons;

class ButtonBar : public View
{
public:
    ButtonBar(const Rect &cFrame, const char* zName);
    ~ButtonBar();
    virtual Point GetPreferredSize(bool bLargest) const;
    virtual void FrameSized(const Point &cDelta);
    virtual void AttachedToWindow(void);
    void AddButton(const char* pzIcon, const char* pzName, Message* pcMessage);

private:

	void ButtonBar::LayoutButtons( void );

    Point cPrefSize;
    bool bIsAttached;

    int nNumButtons;
    t_Buttons vButtons;
};

#endif


