//  AEdit 0.6 -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
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

#ifndef __EDITVIEW_H_
#define __EDITVIEW_H_

#include <gui/menu.h>
#include <gui/view.h>

#include <codeview/CodeView.h>

using namespace os;

class EditView : public CodeView
{
	public:
		EditView(const Rect& cFrame);
		void AttachedToWindow(void);
		void MouseDown(const Point& cPosition, uint32 nButtons); 
		int Find(std::string zOriginal, std::string zFind, bool bCaseSensitive, int nStartPosition);

	private:
		Menu* pcContextMenu;
};

#endif

