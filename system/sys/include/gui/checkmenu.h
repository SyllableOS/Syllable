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

#ifndef	__F_GUI_CHECKMENU_H__
#define	__F_GUI_CHECKMENU_H__

#include <gui/menu.h>
#include <gui/bitmap.h>
#include <gui/font.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class CheckMenu;

/** 
 * \ingroup gui
 * \par Description:
 * A menuitem which can either be "On" or "Off"
 * \since 0.4.0
 * \sa os::MenuItem, os::Menu
 * \author Kristian Van Der Vliet (vanders@blueyonder.co.uk)
 *****************************************************************************/

class CheckMenu : public MenuItem
{
	public:
		CheckMenu( const String& cLabel, Message* pcMsg, bool bChecked=false );
		CheckMenu( Menu* pcMenu, Message* pcMsg, bool bChecked=false );
		~CheckMenu();

		virtual void  Draw();
		virtual void  SetHighlighted( bool bHighlighted );
		virtual Point GetContentSize() const;

		bool IsChecked() const;
		void SetChecked(bool bChecked);

		virtual bool Invoked(Message *pcMessage);
		virtual bool IsSelectable() const;
    private:
		CheckMenu& operator=( const CheckMenu& );
		CheckMenu( const CheckMenu& );

		bool m_Enabled;
		bool m_Highlighted;
		bool m_IsChecked;
		bool m_bIsSelectable;

		static Bitmap* s_pcCheckBitmap;
};

}
#endif
