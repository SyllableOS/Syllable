/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2005 Syllable Team
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

#ifndef __F_GUI_TOOLBAR_H__
#define __F_GUI_TOOLBAR_H__

#include <gui/image.h>
#include <gui/layoutview.h>
#include <util/string.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** ToolBar
 * \ingroup gui
 * \par Description:
 * \par Example:
 * \code
 *	ToolBar* pcToolBar = new ToolBar( GetBounds(), "pcToolbar", CF_FOLLOW_ALL );
 *
 *	pcToolBar->AddButton( "", "New", pcNewIcon, NULL );
 *	pcToolBar->AddButton( "", "Open", pcOpenIcon, NULL );
 *	pcToolBar->AddSeparator( "" );
 *	pcToolBar->AddButton( "", "Test", pcTestIcon, NULL );
 *
 *	AddChild( pcToolBar );
 *
 * \endcode
 * \sa
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class ToolBar : public LayoutView
{
public:
	enum ToolBarObjectMode {
		TB_FIXED_WIDTH = 0,		/** Fixed size, equal to all other objects with fixed size. */
		TB_FREE_WIDTH = 1,		/** Free size, fill remaining space. */
		TB_FIXED_MINIMUM = 2	/** Fix at minimum size. */
	};
public:
    ToolBar( const Rect& cFrame, const String& cTitle,
                    uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
                    uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    virtual ~ToolBar();

	LayoutNode* AddButton( const String& cName, const String& cText, Image* pcIcon, Message* pcMsg );
	LayoutNode* AddPopupMenu( const String& cName, const String& cText, Image* pcIcon, Menu* pcMenu );
	LayoutNode* AddSeparator( const String& cName );
	LayoutNode* AddChild( View* pcPanel, ToolBarObjectMode eMode, float vWeight = 100.0f );

private:
	virtual void __TB_reserved1__();
	virtual void __TB_reserved2__();
	virtual void __TB_reserved3__();
	virtual void __TB_reserved4__();
	virtual void __TB_reserved5__();

private:
    ToolBar& operator=( const ToolBar& );
    ToolBar( const ToolBar& );

	class Private;
	Private *m;
};

}

#endif		// __F_GUI_TOOLBAR_H__
