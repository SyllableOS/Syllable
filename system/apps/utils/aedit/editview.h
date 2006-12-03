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

#include <string>

#include <gui/menu.h>
#include <gui/view.h>
#include <gui/textview.h>
#include "appwindow.h"

using namespace os;

class EditView : public TextView
{
	public:
		EditView(const Rect& cFrame, AEditWindow* pcMain);
		void AttachedToWindow(void);
		void MouseDown(const Point& cPosition, uint32 nButtons);
		void KeyDown(const char* pzString, const char* pzRawString, uint32 nQualifiers);
		int Find(std::string zOriginal, std::string zFind, bool bCaseSensitive, int nStartPosition);
		void Undo(void);
		void Redo(void);
		int GetLineCount(void);
		std::string GetLine(int32 nLineNo);
		void MouseUp(const Point& cPosition, uint32 nButtons, Message* pcData);

	private:
		AEditWindow *pcMainWindow;
		Menu* pcContextMenu;
};

#endif

