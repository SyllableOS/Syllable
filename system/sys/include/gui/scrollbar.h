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

#ifndef	__F_GUI_SLIDER_H__
#define	__F_GUI_SLIDER_H__

#include <limits.h>

#include <atheos/types.h>
#include <gui/control.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent
/*
enum
{
    O_HORIZONTAL,
    O_VERTICAL
};
*/
enum	{ SB_MINSIZE = 12 };

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ScrollBar : public Control
{
public:
    ScrollBar( const Rect& cFrame, const char* pzName, Message* pcMsg,
	       float vMin = 0, float vMax = FLT_MAX, int nOrientation = VERTICAL, uint32 nResizeMask = 0 );
    ~ScrollBar();

    void  SetScrollTarget( View* pcTarget );
    View* GetScrollTarget( void );


      // From Control:

    virtual void PostValueChange( const Variant& cNewValue );
    virtual void LabelChanged( const std::string& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    
      // From View

    virtual void	FrameSized( const Point& cDelta );
    
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	WheelMoved( const Point& cDelta );

    virtual void	Paint( const Rect& cUpdateRect );

    virtual Point	GetPreferredSize( bool bLargest ) const;

    virtual void TimerTick( int nID );
    
      // From ScrollBar:

    void	SetSteps( float vSmall, float vBig );
    void	GetSteps( float* pvSmall, float* pvBig ) const;

    void	SetMinMax( float vMin, float vMax );

    void	SetProportion( float vProp );
    float	GetProportion() const;


private:
    Rect	GetKnobFrame() const;
    float	_PosToVal( Point cPos ) const;

    class Private;
    Private* m;    
};

}

#endif	// __F_GUI_SLIDER_H__
