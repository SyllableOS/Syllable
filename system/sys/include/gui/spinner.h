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

#ifndef __F_SPINNER_H__
#define __F_SPINNER_H__

#include <gui/control.h>

#include <string>

namespace os {
#if 0
}	// Fool emacs autoindent
#endif

class TextView;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Spinner : public Control
{
public:
    Spinner( Rect cFrame, const char* pzName, double vValue, Message* pcMessage,
	     uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );


    void		SetEnable( bool bEnabled = true );
    bool		IsEnabled() const;
    
    void	       SetFormat( const char* pzStr );
    const std::string& GetFormat() const;
    void	       SetMinValue( double vValue );
    void	       SetMaxValue( double vValue );
    void	       SetMinMax( double vMin, double vMax ) { SetMinValue( vMin ); SetMaxValue( vMax ); }
    void	       SetStep( double vStep );
    void	       SetScale( double vScale );

    void	       SetMinPreferredSize( int nWidthChars );
    void	       SetMaxPreferredSize( int nWidthChars );
    
    double	GetMinValue() const;
    double	GetMaxValue() const;
    double	GetStep() const;
    double	GetScale() const;


    virtual void PostValueChange( const Variant& cNewValue );
    virtual void LabelChanged( const std::string& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );    
  
    virtual void  MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  WheelMoved( const Point& cDelta );
    virtual void  AllAttached();
    virtual void  HandleMessage( Message* pcMessage );
  
    virtual void  Paint( const os::Rect& cUpdateRect );
    virtual void  FrameSized( const Point& cDelta );
    virtual Point GetPreferredSize( bool bLargest ) const;

protected:
    virtual std::string	FormatString( double vValue );
  
private:
    void UpdateEditBox();
    double	m_vMinValue;
    double	m_vMaxValue;
    double	m_vSpeedScale;
    double	m_vStep;
    double 	m_vHitValue;
    Point 	m_cHitPos;
    std::string	m_cStrFormat;
    Rect	m_cEditFrame;
    Rect	m_cUpArrowRect;
    Rect	m_cDownArrowRect;
    bool	m_bUpButtonPushed;
    bool	m_bDownButtonPushed;
    TextView*   m_pcEditBox;
};

} // end of namespace os

#endif // __F_SPINNER_H__
