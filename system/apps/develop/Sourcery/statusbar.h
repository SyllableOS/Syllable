/*	CodeEdit - A programmers editor for Atheos
	Copyright (c) 2002 Andreas Engh-Halstvedt
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/


#ifndef _STATUSBAR_H_
#define _STATUSBAR_H_

#include <gui/view.h>
#include <util/string.h>

using namespace os;

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

#endif

