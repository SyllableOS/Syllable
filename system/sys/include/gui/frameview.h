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

#ifndef	__F_GUI_FRAMEVIEW_H__
#define	__F_GUI_FRAMEVIEW_H__

#include <gui/layoutview.h>
#include <gui/font.h>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

/**
 * \ingroup gui
 * \par Description:
 * A view with a border surrounding it, and an optional title. Use this when
 * grouping items that belong together.
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class FrameView : public LayoutView
{
public:
    FrameView( const Rect& cFrame, const String& cName, const String& cLabel,
	       uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	       uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );

    void	SetLabel( const String& cLabel );
    View*	SetLabel( View* pcLabel, bool bResizeToPreferred = true );
    String	GetLabelString() const;
    View*	GetLabelView() const;
    
    virtual void  AttachedToWindow();
    virtual void  FrameSized( const Point& cDelta );
    virtual void  FontChanged( Font* pcNewFont );
    virtual void  Paint( const Rect& cUpdateRect );
    virtual Point GetPreferredSize( bool bLargest ) const;

private:
    virtual void	__FV_reserved1__();
    virtual void	__FV_reserved2__();
    virtual void	__FV_reserved3__();
    virtual void	__FV_reserved4__();
    virtual void	__FV_reserved5__();
private:
    void _CalcStringLabelSize();

    FrameView& operator=( const FrameView& );
    FrameView( const FrameView& );

    Point 	 m_cLabelSize;
    font_height  m_sFontHeight;
    View* 	 m_pcLabelView;
    String  m_cLabelString;

    uint32	__reserver[16];
    
};


} // end of namespace


#endif // __F_GUI_FRAMEVIEW_H__

