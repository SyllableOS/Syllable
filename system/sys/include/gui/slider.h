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

enum	{ SL_MINSIZE = 8 };

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Slider : public Control
{
public:
    enum { TICKS_ABOVE = 0x0001, TICKS_BELOW = 0x0002, TICKS_LEFT = 0x0001, TICKS_RIGHT = 0x0002 };
    enum knob_mode { KNOB_SQUARE, KNOB_TRIANGLE, KNOB_DIAMOND };
    
    Slider( const Rect& cFrame, const String& cName, Message* pcMsg, uint32 nTickFlags = TICKS_BELOW,
	    int nTickCount = 10, knob_mode eKnobMode = KNOB_SQUARE, orientation eOrientation = HORIZONTAL, uint32 nResizeMask = 0 );
    ~Slider();

      // From Slider:

    virtual void RenderLabels( View* pcRenderView );
    virtual void RenderSlider( View* pcRenderView );
    virtual void RenderKnob( View* pcRenderView );
    virtual void RenderTicks( View* pcRenderView );
    
    virtual float PosToVal( const Point& cPos ) const;
    virtual Point ValToPos( float vVal ) const;
    virtual Rect  GetKnobFrame() const;
    virtual Rect  GetSliderFrame() const;

    virtual void	SetSliderColors( const Color32_s& sColor1, const Color32_s& sColor2 );
    virtual void	GetSliderColors( Color32_s* psColor1, Color32_s* psColor2 ) const;
    virtual void	SetSliderSize( float vSize );
    virtual float	GetSliderSize() const;

    
    virtual void	SetProgStrFormat( const String& cFormat );
    virtual String GetProgStrFormat() const;
    virtual String GetProgressString() const;

    void		SetStepCount( int nCount );
    int			GetStepCount() const;
    
    void		SetTickCount( int nCount );
    int			GetTickCount() const;

    void		SetTickFlags( uint32 nFlags );
    uint32		GetTickFlags() const;
    
    void		SetLimitLabels( const String& cMinLabel, const String& cMaxLabel );
    void		GetLimitLabels( String* pcMinLabel, String* pcMaxLabel );
    
    virtual void	SetSteps( float vSmall, float vBig );
    virtual void	GetSteps( float* pvSmall, float* pvBig ) const;

    virtual void	SetMinMax( float vMin, float vMax );
    
    // From Control:
    virtual void PostValueChange( const Variant& cNewValue );
    virtual void EnableStatusChanged( bool bIsEnabled );
    
	virtual bool Invoked( Message* pcMessage );
//    void		SetCurValue( float nValue );
//    virtual void	SetCurValue( float nValue, bool bInvoke );
//    virtual float	GetCurValue() const;

    //	From View:
    virtual void  AttachedToWindow();
    virtual void  MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void  MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void  FrameSized( const Point& cDelta );
    virtual void  Paint( const Rect& cUpdateRect );
    virtual Point GetPreferredSize( bool bLargest ) const;
   	virtual void Activated( bool bIsActive );

private:
    virtual void	__SL_reserved1__();
    virtual void	__SL_reserved2__();
    virtual void	__SL_reserved3__();
    virtual void	__SL_reserved4__();
    virtual void	__SL_reserved5__();

private:
    Slider& operator=( const Slider& );
    Slider( const Slider& );

    void _RefreshDisplay();

	class Private;
	Private *m;
};

}

#endif	// __F_GUI_SLIDER_H__
