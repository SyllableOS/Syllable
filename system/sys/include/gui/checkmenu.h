/*  libatheos.so - the highlevel API library for Syllable
 *  Copyright (C) 2002 Kristian Van Der Vliet
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#ifndef	_GUI_CHECKMENU_HPP
#define	_GUI_CHECKMENU_HPP

#include <gui/menu.h>
#include <gui/bitmap.h>
#include <gui/font.h>

using namespace os;

class CheckMenu : public MenuItem
{
	public:
		CheckMenu( const char* pzLabel, Message* pcMsg, bool bChecked=false );
		CheckMenu( Menu* pcMenu, Message* pcMsg, bool bChecked=false );
		~CheckMenu();

		virtual void  Draw();
		virtual void  DrawContent();
		virtual void  Highlight( bool bHighlight );
		virtual Point GetContentSize();

		bool IsChecked();
		void SetChecked(bool bChecked);

		virtual bool Invoked(Message *pcMessage);

    private:
		bool m_Enabled;
		bool m_Highlighted;
		bool m_IsChecked;

		static Bitmap* s_pcCheckBitmap;
};


#endif








