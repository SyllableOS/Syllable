/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef	__F_GUI_STRINGVIEW_H__
#define	__F_GUI_STRINGVIEW_H__

#include <atheos/types.h>
#include <gui/view.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class StringView : public View
{
public:
    StringView( Rect cFrame, const String& cName, const String& cString,
		alignment eAlign = ALIGN_LEFT,
		uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		uint32 nFlags  = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~StringView();

    void	SetMinPreferredSize( int nWidthChars );
    void	SetMaxPreferredSize( int nWidthChars );
    
    virtual Point GetPreferredSize( bool bLargest ) const;

    virtual void  Paint( const Rect& cUpdateRect );

      // From StringView:

    void	 SetString( const String& cString );
    const String& GetString( void ) const;

    void	 SetAlignment( alignment eAlign );
    alignment	 GetAlignment( void ) const;
    virtual void AttachedToWindow();

private:
    struct data {
	String m_cString;
	int	    m_nMinSize;
	int	    m_nMaxSize;
    }* m;
    alignment	m_eAlign;
};

}

#endif	//	__F_GUI_STRINGVIEW_H__

