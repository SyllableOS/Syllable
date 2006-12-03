// AEdit -:-  (C)opyright 2004 Jonas Jarvoll
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

#include <gui/view.h>
#include <util/invoker.h>
#include "mytabview.h"

using namespace os;

MyTabView::MyTabView(const Rect& cFrame, AEditWindow* pcMain) : TabView(cFrame, "appwindow_tab_view", CF_FOLLOW_ALL, WID_WILL_DRAW)
{
	pcMainWindow=pcMain;
}

void MyTabView::Paint(const Rect& cUpdate)
{
	// Check if there is any tabs attached
	if(TabView::GetTabCount()<1)
	{
		// Draw nice background if there is not any tabs attached
		FillRect(cUpdate, get_default_color(COL_NORMAL));
	}
	else
		TabView::Paint(cUpdate);
}

// We implement this function to be able to do drag and drop of files
void MyTabView::MouseUp(const Point& cPosition, uint32 nButtons, Message* pcData)
{
	if(pcData!=NULL)
		pcMainWindow->DragAndDrop(pcData);
	else
		TabView::MouseUp(cPosition, nButtons, pcData);
}
