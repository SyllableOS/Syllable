/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	GUI_STRINGVIEW_HPP
#define	GUI_STRINGVIEW_HPP

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
    StringView( Rect cFrame, const char* pzName, const char* pzString,
		alignment eAlign = ALIGN_LEFT,
		uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		uint32 nFlags  = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~StringView();

    void	SetMinPreferredSize( int nWidthChars );
    void	SetMaxPreferredSize( int nWidthChars );
    
    virtual Point GetPreferredSize( bool bLargest ) const;

    virtual void  Paint( const Rect& cUpdateRect );

      // From StringView:

    void	 SetString( const char* pzString );
    const char*	 GetString( void ) const;

    void	 SetAlignment( alignment eAlign );
    alignment	 GetAlignment( void ) const;
    virtual void AttachedToWindow();

private:
    struct data {
//	char*       m_pzString;
	std::string m_cString;
	int	    m_nMinSize;
	int	    m_nMaxSize;
    }* m;
    alignment	m_eAlign;
};

}

#endif	//	GUI_STRINGVIEW_HPP
