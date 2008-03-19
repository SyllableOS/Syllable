/*
 *  ATail - Graphic replacement for tail
 *  Copyright (C) 2001 Henrik Isaksson
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details. 
 *
 *  You should have received a copy of the GNU Library General Public 
 *  License along with this library; if not, write to the Free 
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef TAILWIN_H
#define TAILWIN_H

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/textview.h>
#include <util/message.h>
#include <storage/nodemonitor.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ATAIL_VERSION	"1.0"

using namespace os;

class TailView : public TextView {
	public:
	TailView(const Rect& cWindowBounds, char* pzTitle, char* pzBuffer, uint32 nResizeMask , uint32 nFlags);

	void AttachedToWindow(void) { m_ContextMenu->SetTargetForItems(GetWindow()); }

	void MouseDown(const Point& cPosition, uint32 nButtons) {
		if(nButtons == 2){
			m_ContextMenu->Open(ConvertToScreen(cPosition));
		} else {
			TextView::MouseDown(cPosition, nButtons);
		}
	}

	void KeyDown(const char* cString, const char* cRawString, uint32 nQualifiers);

	private:
	Menu* m_ContextMenu;
};

class TailWin: public Window {
	public:
	TailWin(const Rect & r, const String & file, uint32 desktopmask);
	~TailWin();

	bool OkToQuit();

	void HandleMessage(Message *);
	void TimerTick(int nID);

	protected:

	void Tail(void);
	void Save(const char *file);

	private:
	TextView		*m_TextView;
	String			*m_FileName;
	int				m_LastSize;
};


enum {
	ID_COPY = 5, ID_CLEAR, ID_SAVE, ID_ABOUT, ID_OPEN,ID_NOP,
	ID_QUIT
};

#endif










