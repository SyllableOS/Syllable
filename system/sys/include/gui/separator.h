/*  Separator - Separator bar for separation of widgets
 *  Copyright (C) 2005 Syllable Team
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

#ifndef __F_GUI_SEPARATOR_H__
#define __F_GUI_SEPARATOR_H__

#include <gui/view.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Separator bar
 * \ingroup gui
 * \par Description:
 *	The Separator shows a vertical or horizontal bar that is used to
 *	separate widgets from each other. It is commonly used in tool bars, to
 *	group similar commands.
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class Separator : public View
{
	public:
	Separator( Rect cFrame, const String& cName, orientation eOrientation = VERTICAL,
	             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
	~Separator();

	void Paint( const Rect &cUpdateRect );
	Point GetPreferredSize( bool bLargest ) const;

	void SetOrientation( orientation eOrientation );
	orientation GetOrientation() const;

	private:
    virtual void	__SV_reserved1__();
    virtual void	__SV_reserved2__();
    virtual void	__SV_reserved3__();
    virtual void	__SV_reserved4__();
    virtual void	__SV_reserved5__();

	private:
	Separator& operator=( const Separator& );
	Separator( const Separator& );

	class Private;
	Private*	m;
};

}

#endif /* __F_GUI_SEPARATOR_H__ */
