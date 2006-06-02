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

#ifndef _SPLASH_DISPLAY_H
#define _SPLASH_DISPLAY_H

#include <gui/window.h>
#include <gui/view.h>
#include <gui/image.h>

using namespace os;

class SplashView : public View
{
public:
	SplashView();
	~SplashView();

	virtual void Paint(const Rect& cUpdateRect);
private:
	BitmapImage* pcImage;
};

class SplashWindow : public Window
{
public:
	SplashWindow();
	virtual bool OkToQuit();
private:
	SplashView* pcView;
};
#endif
