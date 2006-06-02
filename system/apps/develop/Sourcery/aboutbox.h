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

#ifndef _ABOUT_BOX_H
#define _ABOUT_BOX_H

#include <gui/view.h>
#include <gui/window.h>
#include <gui/font.h>
#include <gui/imagebutton.h>

#include "messages.h"
#include "link.h"

using namespace os;

class AboutBoxView;

class AboutBoxWindow : public Window
{
public:
	AboutBoxWindow(Window* pcParent);
	
	virtual void HandleMessage(Message*);
	virtual bool OkToQuit();
private:
	AboutBoxView* pcView;
	Window* pcParentWindow;

};

class AboutBoxView : public View
{
public:
	AboutBoxView();
	~AboutBoxView();
	virtual	void Paint(const Rect&);
private:
	void DrawArc(int r);
	Image* pcBitmap;
};

#endif //_ABOUT_BOX_H
