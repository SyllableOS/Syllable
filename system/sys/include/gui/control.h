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

#ifndef	GUI_CONTROL_HPP
#define	GUI_CONTROL_HPP

#include <gui/view.h>
#include <util/invoker.h>
#include <util/variant.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Base class for GUI controls.
 * \ingroup gui
 * \par Description:
 *	
 * \sa os::Invoker
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


class Control	: public View, public Invoker
{
public:
    Control( const Rect& cFrame, const std::string& cName, const std::string& cLabel, Message* pcMessage,
	     uint32 nResizeMask, uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
    ~Control();

      // From View:
    virtual void AttachedToWindow( void );

      // From Control:
    virtual void ValueChanged( Variant* pcNewValue ); // OBSOLETE
    virtual bool PreValueChange( Variant* pcNewValue );
    virtual void PostValueChange( const Variant& cNewValue );

    virtual void LabelChanged( const std::string& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled ) = 0;
    virtual bool Invoked( Message* pcMessage );
    
    virtual void SetEnable( bool bEnabled );
    virtual bool IsEnabled( void ) const;

    virtual void	SetLabel( const std::string& cLabel );
    virtual std::string	GetLabel( void ) const;

    virtual void    SetValue( Variant cValue, bool bInvoke = true );
    virtual Variant GetValue() const;


    virtual void	__CTRL_reserved1__();
    virtual void	__CTRL_reserved2__();
    virtual void	__CTRL_reserved3__();
    virtual void	__CTRL_reserved4__();
    virtual void	__CTRL_reserved5__();
    
private:
    class Private;
    Private* m;
};

}

#endif	//	GUI_CONTROL_HPP
