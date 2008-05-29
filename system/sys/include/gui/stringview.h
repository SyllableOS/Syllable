/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2005 Syllable Team
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
 * Display a non-editable text string (text label)-
 * \sa TextView
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class StringView : public View
{
public:
    StringView( Rect cFrame, const String& cName, const String& cString,
		uint32 nAlign = DTF_DEFAULT,
		uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		uint32 nFlags  = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

	/* Deprecated constructor using alignment eAlign instead of uint32 nAlign */
	StringView( Rect cFrame, const String& cName, const String& cString,
		alignment eAlign,
		uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		uint32 nFlags  = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

    ~StringView();

    void	SetMinPreferredSize( int nWidthChars, int nHeightChars = 1 );
    void	SetMaxPreferredSize( int nWidthChars, int nHeightChars = 1 );
    
    virtual Point GetPreferredSize( bool bLargest ) const;

    virtual void  Paint( const Rect& cUpdateRect );

    // From StringView:
    void	 SetString( const String& cString );
    const String& GetString( void ) const;

    void	 SetAlignment( uint32 nAlign );
    uint32	 GetAlignment( void ) const;

    bool HasBorder() const;
    void SetRenderBorder( bool bRender );

    virtual void AttachedToWindow();

private:
    virtual void	__SV_reserved1__();
    virtual void	__SV_reserved2__();
    virtual void	__SV_reserved3__();
    virtual void	__SV_reserved4__();
    virtual void	__SV_reserved5__();

private:
    StringView& operator=( const StringView& );
    StringView( const StringView& );

	class Private;
	Private* m;
};

}

#endif	//	__F_GUI_STRINGVIEW_H__
