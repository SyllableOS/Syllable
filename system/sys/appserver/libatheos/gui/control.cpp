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

#include <gui/control.h>
#include <gui/window.h>
#include <util/message.h>

using namespace os;

class Control::Private
{
public:
    std::string	m_cLabel;
    Variant	m_cValue;
    bool	m_bIsEnabled;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Control::Control( const Rect& cFrame, const std::string& cName, const std::string& cLabel, Message* pcMsg, uint32 nResizeMask, uint32 nFlags )
    : View( cFrame, cName, nResizeMask, nFlags ),
      Invoker( pcMsg, NULL )
{
    m = new Private;
    m->m_bIsEnabled	= true;
    m->m_cLabel		= cLabel;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Control::~Control()
{
    delete m;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void	Control::AttachedToWindow( void )
{
    View::AttachedToWindow();

    if ( GetMessenger().IsValid() == false ) {
	SetTarget( static_cast<Handler*>(GetWindow()) );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Control::Invoked( Message* pcMessage )
{
    pcMessage->AddVariant( "value", m->m_cValue );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Control::SetEnable( bool bEnabled )
{
    if ( m->m_bIsEnabled != bEnabled ) {
	m->m_bIsEnabled = bEnabled;
	EnableStatusChanged( bEnabled );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Control::IsEnabled( void ) const
{
    return( m->m_bIsEnabled );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Control::SetLabel( const std::string& cLabel )
{
    if ( cLabel != m->m_cLabel ) {
	m->m_cLabel = cLabel;
	LabelChanged( m->m_cLabel );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

std::string Control::GetLabel( void ) const
{
    return( m->m_cLabel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Control::SetValue( Variant cValue, bool bInvoke )
{
    if ( m->m_cValue != cValue ) {
	if ( PreValueChange( &cValue ) ) {
	    m->m_cValue = cValue;
	    PostValueChange( cValue );
	    if ( bInvoke ) {
		Invoke();
	    }
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Variant Control::GetValue( void ) const
{
    return( m->m_cValue );
}

void Control::ValueChanged( Variant* pcNewValue )
{
}

bool Control::PreValueChange( Variant* pcNewValue )
{
    return( true );
}

void Control::PostValueChange( const Variant& cNewValue )
{
}

void Control::LabelChanged( const std::string& cNewLabel )
{
}

void Control::__CTRL_reserved1__() {}
void Control::__CTRL_reserved2__() {}
void Control::__CTRL_reserved3__() {}
void Control::__CTRL_reserved4__() {}
void Control::__CTRL_reserved5__() {}
