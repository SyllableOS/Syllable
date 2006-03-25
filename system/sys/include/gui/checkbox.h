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

#ifndef	__F_GUI_CHECKBOX_H__
#define	__F_GUI_CHECKBOX_H__

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
    CheckBox( Rect cFrame, const String& cName, const String& cLabel, Message* pcMessage,
	      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE );
    ~CheckBox();

      // From view

    virtual Point GetPreferredSize( bool bLargest ) const;
    virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void  KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void  Paint( const Rect& cUpdateRect );

    virtual void PostValueChange( const Variant& cNewValue );
    virtual void LabelChanged( const String& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
	virtual void Activated( bool bIsActive );

private:
    CheckBox& operator=( const CheckBox& );
    CheckBox( const CheckBox& );

	class Private;
	Private *m;
};

}

#endif	//	GUI_CHECKBOX_HPP

