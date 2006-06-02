//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#ifndef _F_GUI_LINK_H_
#define _F_GUI_LINK_H_

#include <gui/view.h>
#include <gui/control.h>
#include <util/string.h>
#include <gui/font.h>

using namespace os;

//simple generic link class(definitely not the way to do it)
class Link : public Control
{
public:
	Link(os::Message* pcMessage=NULL);
	Link(const String& cLinkName, const String& cLink, os::Message*);
	Link(const String& cLinkName, const String& cLink, os::Message*, Color32_s sBgColor, Color32_s sFgColor);
	~Link();
	
	virtual void 			Paint(const Rect&);
	virtual void 			MouseDown(const Point& cPos, uint32 nButtons);
	virtual void 			MouseUp(const Point& cPos, uint32 nButtons, Message* pcData);
	virtual void 			MouseMove(const Point& cPos, int nCode, uint32 nButtons, Message* pcData);
	
	virtual Point 			GetPreferredSize(bool bSize) const;
	virtual void			EnableStatusChanged(bool);
private:
	class Private;
	Private* m;

	String		_DrawUnderlines();
};

#endif

