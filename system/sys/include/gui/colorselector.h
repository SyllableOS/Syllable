/*  colorselector.h - a colour selection widget.
 *  Copyright (C) 2004 Syllable Team
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
#ifndef __F_GUI_COLORSELECTOR_H__
#define __F_GUI_COLORSELECTOR_H__

#include <gui/control.h>

namespace os
{

/** ColorSelector
 * \ingroup gui
 * \par Description:
 * ColorSelector, a colour selection widget. Use the Get/SetValue() methods to
 * retrieve the currently selected colour.
 * \sa Control::SetValue(), Control::GetValue()
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class ColorSelector : public Control
{
	public:
	ColorSelector( const Rect& cRect, const String& cName, Message* pcMsg, Color32_s cColor = 0xFFFFFFFF, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
	~ColorSelector();

	virtual void HandleMessage( Message *pcMsg );
	virtual void AttachedToWindow();
	virtual void PostValueChange( const Variant& cNewValue );
    virtual void SetValue( Variant cValue, bool bInvoke = true );
	virtual void EnableStatusChanged( bool );

	private:
	virtual void 	_reserved1();
	virtual void 	_reserved2();
	virtual void 	_reserved3();
	virtual void 	_reserved4();
	virtual void 	_reserved5();

	private:

	ColorSelector& operator=( const ColorSelector& );
	ColorSelector( const ColorSelector& );

	class Private;
	Private* m;
};

}

#endif	/* __F_GUI_COLORSELECTOR_H__*/


