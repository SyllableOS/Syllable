// AEdit -:-  (C)opyright 2004-2006 Jonas Jarvoll
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

#ifndef __MYTABVIEW_H
#define __MYTABVIEW_H

#include <gui/rect.h>
#include <gui/view.h>
#include "appwindow.h"
#include "tabview.h"

class AEditWindow;

class MyTabView : public TabView
{
	public:
  		MyTabView(const os::Rect& cFrame, AEditWindow* pcMain);
		void Paint(const os::Rect& cUpdate);
		void MouseUp(const os::Point& cPosition, uint32 nButtons, os::Message* pcData);
	private:
		AEditWindow* pcMainWindow;
};

#endif
