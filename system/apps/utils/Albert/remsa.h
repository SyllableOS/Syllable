/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef REMSA_H
#define REMSA_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/textview.h>
#include <util/message.h>

using namespace os;

class PaperRoll : public TextView {
	public:
	PaperRoll(const Rect& cWindowBounds, char* pzTitle, char* pzBuffer, uint32 nResizeMask , uint32 nFlags);

	void AttachedToWindow(void) { m_ContextMenu->SetTargetForItems(GetWindow()); }

	void MouseDown(const Point& cPosition, uint32 nButtons) {
		if(nButtons == 2){
			m_ContextMenu->Open(ConvertToScreen(cPosition));
		} else {
			TextView::MouseDown(cPosition, nButtons);
		}
	}

	private:
	Menu* m_ContextMenu;
};

class Remsa: public Window {
	public:
	Remsa(const Rect & r);
	~Remsa();

	bool OkToQuit();

	void HandleMessage(Message *);

	void Show(void);

	private:
	void _AddText( const char * );

	TextView	*m_TextView;
	bool		m_Visible;
};

#endif
