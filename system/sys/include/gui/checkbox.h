/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#ifndef	GUI_CHECKBOX_HPP
#define	GUI_CHECKBOX_HPP

#include <gui/control.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** 2-state check box.
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class CheckBox : public Control
{
public:
    CheckBox( Rect cFrame, const char* pzName, const char* pzLabel, Message* pcMessage,
	      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
    ~CheckBox();

      // From view

    virtual Point GetPreferredSize( bool bLargest ) const;
    virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  Paint( const Rect& cUpdateRect );

    virtual void PostValueChange( const Variant& cNewValue );
    virtual void LabelChanged( const std::string& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    

private:

};

}

#endif	//	GUI_CHECKBOX_HPP
