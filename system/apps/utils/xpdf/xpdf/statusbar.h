//  AView (C)opyright 2005 Kristian Van Der Vliet
//        (C)opyright 2002 Andreas Engh-Halstvedt
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

#ifndef __F_AVIEW_NG_STATUSBAR_H_
#define __F_AVIEW_NG_STATUSBAR_H_

#include <gui/view.h>
#include <util/string.h>

namespace os
{

class StatusPanel;

class StatusBar : public os::View
{
public:
    enum panelmode{ CONSTANT=0x0002, RELATIVE=0x0003, FILL=0x0004 };

public:
    StatusBar(const os::Rect&, const String&, int,uint32=os::CF_FOLLOW_BOTTOM|os::CF_FOLLOW_LEFT|os::CF_FOLLOW_RIGHT,uint32=os::WID_WILL_DRAW|os::WID_CLEAR_BACKGROUND|os::WID_FULL_UPDATE_ON_RESIZE);
    ~StatusBar();

	//inherited methods
    os::Point 		GetPreferredSize(bool largest) const;
	void 			Paint(const os::Rect&);
    void 			TimerTick(int);

	//statusbar specific
    void 			ConfigurePanel(int panel, panelmode mode, float width);
    void 			SetText(const String&, int panel=0, bigtime_t timeout=0);

private:
    int nPanelCount;
    StatusPanel* m_pcPanels;
	void DrawPanel(const String& cText, float, float, float, float, float);
};

};

#endif		// __F_AVIEW_NG_STATUSBAR_H_
