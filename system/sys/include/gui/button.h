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

#ifndef	__F_GUI_BUTTON_H__
#define	__F_GUI_BUTTON_H__

#include <atheos/types.h>

#include <gui/control.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** Simple push-button class.
 * \ingroup gui
 * \par Description:
 *	A button object renders a labeled button and accept user-input
 *	through the mouse or keyboard. The button will Invoke() the
 *	Control if the user click it with the mouse or hit space or
 *	enter while the button have focus (it will take focus when
 *	clicked). Each window can have a "default" button assigned
 *	with the os::Window::SetDefaultButton() member. The default
 *	button can be invoked with the Enter key even when it don't
 *	have the focus.
 *	
 * \sa Control, Invoker, View
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


class Button : public Control
{
public:
	enum InputMode {
		InputModeNormal,
		InputModeToggle,
		InputModeRadio
	};
public:
    Button( Rect cFrame, const String& cName, const String& cLabel, Message* pcMessage,
	    uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE );
    ~Button();

    virtual Point GetPreferredSize( bool bLargest ) const;
    virtual void  MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void  KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void  Activated( bool bIsActive );
    virtual void  Paint( const Rect& cUpdateRect );


    virtual void PostValueChange( const Variant& cNewValue );
    virtual void LabelChanged( const String& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    
	void SetInputMode( InputMode eInputMode );
	InputMode GetInputMode() const;


private:
    Button& operator=( const Button& );
    Button( const Button& );

	class Private;
	Private *m;
};

}

#endif	//	__F_GUI_BUTTON_H__
