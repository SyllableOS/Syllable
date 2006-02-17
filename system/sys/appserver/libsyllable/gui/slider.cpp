/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
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

#include <assert.h>
#include <stdio.h>

#include <atheos/types.h>

#include <gui/slider.h>
#include <gui/font.h>
#include <gui/bitmap.h>

#include <util/message.h>

#include <macros.h>

using namespace os;

class Slider::Private
{
	public:
	Private() {
		m_vMin = 0.0f;
		m_vMax = 1.0f;
		m_vSmallStep = 0.05f;
		m_vBigStep = 0.1f;
		m_bTrack = false;
		m_bChanged = false;
		m_vSliderSize = 5.0f;
		m_nNumSteps = 0;
		m_pcRenderBitmap = NULL;
	}

	public:
    String m_cMinLabel;
    String m_cMaxLabel;
    String m_cProgressFormat;

    Color32_s	m_sSliderColor1;
    Color32_s	m_sSliderColor2;
    View*	m_pcRenderView;
    Bitmap*	m_pcRenderBitmap;
    float	m_vSliderSize;
    int		m_nNumSteps;
    int		m_nNumTicks;
    int		m_nTickFlags;
    knob_mode	m_eKnobMode;
    float	m_vMin;
    float	m_vMax;
    float	m_vSmallStep;
    float	m_vBigStep;
    orientation	m_eOrientation;
    
    bool	m_bChanged;
    bool	m_bTrack;
    Point	m_cHitPos;
};

#define TICK_LENGTH	4.0f
#define TICK_SPACING	3.0f
#define VLABEL_SPACING	3.0f
#define HLABEL_SPACING	2.0f

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Slider::Slider( const Rect & cFrame, const String & cName, Message * pcMsg, uint32 nTickFlags, int nTickCount, knob_mode eKnobMode, orientation eOrientation, uint32 nResizeMask ):Control( cFrame, cName.c_str(), "", pcMsg, nResizeMask, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE )
{
	m = new Private;

	m->m_eOrientation = eOrientation;
	m->m_eKnobMode = eKnobMode;
	m->m_nNumTicks = nTickCount;
	m->m_nTickFlags = nTickFlags;

	m->m_sSliderColor1 = get_default_color( COL_SCROLLBAR_BG );;
	m->m_sSliderColor2 = m->m_sSliderColor1;

	SetValue( m->m_vMin, false );

	m->m_pcRenderView = new View( cFrame.Bounds(), cName );
	if( cFrame.IsValid() )
	{
		m->m_pcRenderBitmap = new Bitmap( int ( cFrame.Width() ) + 1, int ( cFrame.Height(  ) ) + 1, CS_RGB16, Bitmap::ACCEPT_VIEWS );

		m->m_pcRenderBitmap->AddChild( m->m_pcRenderView );

		_RefreshDisplay();
	}
	else
	{
		m->m_pcRenderBitmap = NULL;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Slider::~Slider()
{
	delete m;

	if( m->m_pcRenderBitmap != NULL )
	{
		delete m->m_pcRenderBitmap;
	}
	else
	{
		delete m->m_pcRenderView;
	}
}

/** Sets the background color to the one of the parent view
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::AttachedToWindow()
{
	SetBgColor( GetParent()->GetBgColor(  ) );
	_RefreshDisplay();
}

void Slider::FrameSized( const Point & cDelta )
{
	Sync();
	if( m->m_pcRenderBitmap != NULL )
	{
		m->m_pcRenderBitmap->RemoveChild( m->m_pcRenderView );
		delete m->m_pcRenderBitmap;
	}

	Rect cBounds( GetNormalizedBounds() );

	if( cBounds.IsValid() )
	{
		m->m_pcRenderView->SetFrame( cBounds );
		m->m_pcRenderBitmap = new Bitmap( int ( cBounds.Width() ) + 1, int ( cBounds.Height(  ) ) + 1, CS_RGB16, Bitmap::ACCEPT_VIEWS );

		m->m_pcRenderBitmap->AddChild( m->m_pcRenderView );

		RenderSlider( m->m_pcRenderView );
		m->m_pcRenderBitmap->Sync();
		Invalidate();
	}
	else
	{
		m->m_pcRenderBitmap = NULL;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Slider::GetPreferredSize( bool bLargest ) const
{
	font_height sHeight;

	GetFontHeight( &sHeight );

	if( m->m_eOrientation == HORIZONTAL )
	{
		Point cSize( 100.0f, GetSliderSize() );
		Rect cSliderFrame = GetSliderFrame();
		Rect cKnobFrame = GetKnobFrame();

		float vCenter = ceil( cSliderFrame.top + ( cSliderFrame.Height() + 1.0f ) * 0.5f );

		if( m->m_cMinLabel.size() > 0 && m->m_cMaxLabel.size(  ) > 0 )
		{
			float vStrWidth = GetStringWidth( m->m_cMinLabel ) + GetStringWidth( m->m_cMaxLabel ) + 20.0f;

			if( vStrWidth > cSize.x )
			{
				cSize.x = vStrWidth;
			}
			cSliderFrame.bottom += sHeight.ascender + sHeight.descender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING;
		}
		else if( m->m_nTickFlags & TICKS_BELOW )
		{
			cSliderFrame.bottom += TICK_SPACING + TICK_LENGTH;
		}
		if( vCenter + ( cKnobFrame.Height() + 1.0f ) * 0.5f > cSliderFrame.bottom )
		{
			cSliderFrame.bottom = vCenter + ( cKnobFrame.Height() + 1.0f ) * 0.5f;
		}
		cSize.y = cSliderFrame.bottom + 1.0f;
		if( bLargest )
		{
			cSize.x = 2000.0f;
		}
		return ( Point( ceil( cSize.x ), ceil( cSize.y ) ) );
	}
	else
	{
		Point cSize( GetSliderSize(), 100.0f );

		if( m->m_nTickFlags & TICKS_LEFT )
		{
			cSize.x += TICK_SPACING + TICK_LENGTH + 1.0f;
		}
		if( m->m_nTickFlags & TICKS_RIGHT )
		{
			cSize.x += TICK_SPACING + TICK_LENGTH + 1.0f;
		}

		if( m->m_cMinLabel.size() > 0 )
		{
			float vStrWidth = GetStringWidth( m->m_cMinLabel );

			if( vStrWidth > cSize.x )
			{
				cSize.x = vStrWidth;
			}
			cSize.y += sHeight.ascender + sHeight.descender + VLABEL_SPACING;
		}
		if( m->m_cMaxLabel.size() > 0 )
		{
			float vStrWidth = GetStringWidth( m->m_cMaxLabel );

			if( vStrWidth > cSize.x )
			{
				cSize.x = vStrWidth;
			}
			cSize.y += sHeight.ascender + sHeight.descender + VLABEL_SPACING;
		}
		if( cSize.x < GetKnobFrame().Width(  ) + 1.0f )
		{
			cSize.x = GetKnobFrame().Width(  ) + 1.0f;
		}
		if( bLargest )
		{
			cSize.y = 2000.0f;
		}
		return ( Point( ceil( cSize.x ), ceil( cSize.y ) ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Slider::Invoked( Message * pcMsg )
{
	if( m->m_bTrack )
	{
		pcMsg->AddBool( "final", false );
	}
	else
	{
		pcMsg->AddBool( "final", true );
	}
	return ( Control::Invoked( pcMsg ) );
//	return ( true );

/*    
    if ( NULL == a_pcMessage ) {
	a_pcMessage = GetMessage();
    }
    if ( NULL != a_pcMessage )
    {
	Message* pcMsg = new Message( *a_pcMessage );

	if ( m->m_bTrack ) {
	    pcMsg->AddBool( "final", false );
	} else {
	    pcMsg->AddBool( "final", true );
	}
	return( Control::Invoke( pcMsg ) );
    }
    return( -1 );*/
}


void Slider::PostValueChange( const Variant & cNewValue )
{
	m->m_bChanged = true;
	_RefreshDisplay();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

/*
void Slider::SetCurValue( float vValue, bool bInvoke )
{
    if ( GetCurValue() == vValue ) {
	return;
    }
//    Control::SetValue( nValue );

    m->m_vValue = vValue;
    m->m_bChanged = true;
    _RefreshDisplay();
    
    if ( bInvoke == false ) {
	return;
    }
    
    Invoke();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::SetCurValue( float vValue )
{
    SetCurValue( vValue, true );
}

float Slider::GetCurValue() const
{
    return( m->m_vValue );
}
*/

/** Modify the fill color of the slider bar.
 * \par Description:
 *	Set the two colors used to fill the slider bar.
 *	The first color is used to fill the "lesser" part
 *	of the slider while the seccond color is used in
 *	the "greater" part of the slider.
 * \param sColor1 - Color used left/below the knob
 * \param sColor2 - Color used right/abow the knob
 * \sa GetSliderColors(), SetSliderSize(), SetLimitLabels(), SetProgStrFormat()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetSliderColors( const Color32_s & sColor1, const Color32_s & sColor2 )
{
	m->m_sSliderColor1 = sColor1;
	m->m_sSliderColor2 = sColor2;
	_RefreshDisplay();
}

/** Get the slider-bar fill color.
 * \par Description:
 *	Return the two slider-bar colors. Each of the two pointers might
 *	be NULL.
 * \param psColor1 - Color used left/below the knob
 * \param psColor2 - Color used right/abow the knob
 * \sa SetSliderColors(), SetLimitLabels(), GetSliderSize()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::GetSliderColors( Color32_s * psColor1, Color32_s * psColor2 ) const
{
	if( psColor1 != NULL )
	{
		*psColor1 = m->m_sSliderColor1;
	}
	if( psColor2 != NULL )
	{
		*psColor2 = m->m_sSliderColor2;
	}
}

/** Set the thickness of the slider bar.
 * \par Description:
 *	Set the size in slider bar in pixels. For a horizontal slider it
 *	sets the heigth and for a vertical it sets the width.
 * \par Note:
 * \par Warning:
 * \param vSize - Slider-bar thickness in pixels.
 * \sa GetSliderSize(), GetSliderFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetSliderSize( float vSize )
{
	m->m_vSliderSize = vSize;
	_RefreshDisplay();
}

/** Get the current slider thickness.
 * \par Description:
 *	Returns the width for a horizontal and the height for a vertical
 *	slider of the slider-bar.
 * \return The thickness of the slider-bar in pixels.
 * \sa SetSliderSize(), GetSliderFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

float Slider::GetSliderSize() const
{
	return ( m->m_vSliderSize );
}

/** Set format string for the progress label.
 * \par Description:
 *	Set a printf() style string that will be used to format the
 *	"progress" label printed above the slider. The format function
 *	will be passed the sliders value as a parameter so if you throw
 *	in an %f somewhere it will be replaced with the current value.
 * \par Note:
 *	If you need some more advanced formatting you can inherit a new
 *	class from os::Slider and overload the GetProgressString() and
 *	return the string to be used as a progress label.
 * \param cFormat - The printf() style format string used to generate the
 *		    progress label.
 * \sa GetProgStrFormat(), GetProgressString(), SetLimitLabels(), GetLimitLabels()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetProgStrFormat( const String & cFormat )
{
	m->m_cProgressFormat = cFormat;
	_RefreshDisplay();
}

String Slider::GetProgStrFormat() const
{
	return ( m->m_cProgressFormat );
}

String Slider::GetProgressString() const
{
	if( m->m_cProgressFormat.size() == 0 )
	{
		return ( "" );
	}
	else
	{
		char *pzString = new char[m->m_cProgressFormat.size() + 32];

		sprintf( pzString, m->m_cProgressFormat.c_str(), GetValue(  ).AsDouble(  ) );
		String cLabel( pzString );
		delete[]pzString;
		return ( cLabel );
	}
}

/** Set the number of possible knob positions.
 * \par Description:
 *	The step count is used to limit the number of possible knob
 *	positions. The points are equaly spread out over the length
 *	of the slider and the knob will "snap" to these points.
 *	If set to the same value as SetTickCount() the knob will snap
 *	to the ticks.
 *
 *	If the value is less than 2 the knob is not snapped.
 * \param nCound - Number of possible knob positions. 0 to disable snapping.
 * \sa GetStepCount(), SetTickCount(), GetTickCount()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetStepCount( int nCount )
{
	m->m_nNumSteps = nCount;
}

/** Obtain the step-count as set by SetStepCount()
 * \return The number of possible knob positions.
 * \sa SetStepCount()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Slider::GetStepCount() const
{
	return ( m->m_nNumSteps );
}

/** Set number of "ticks" rendered along the slider.
 * \par Description:
 *	The slider can render "ticks" along both sides of the slider-bar.
 *	With SetTickCount() you can set how many such ticks should be rendered
 *	and with SetTickFlags() you can set on which side (if any) the ticks
 *	should be rendered. To make the knob "snap" to the ticks you can set
 *	the same count with SetStepCount().
 * \param nCount - The number of ticks to be rendered.
 * \sa GetTickCount(), SetTickFlags(), GetTickFlags(), SetStepCount()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetTickCount( int nCount )
{
	m->m_nNumTicks = nCount;
	_RefreshDisplay();
}

/** Obtain the tick count as set with SetTickCount()
 * \return The number of ticks to be rendered
 * \sa SetTickCount()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Slider::GetTickCount() const
{
	return ( m->m_nNumTicks );
}

/** Configure where the slider ticks should be rendered.
 * \par Description:
 *	The flag can be any combination of TICKS_ABOVE and TICKS_BELOW for
 *	horizontal sliders and TICKS_LEFT and TICKS_RIGHT for vertical
 *	sliders. The render ticks on both side you can bitwize "or" the
 *	flags together.
 * \par Warning:
 * \param nFlags - The new tick flags.
 * \sa GetTickFlags(), SetTickCount(), GetTickCount()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetTickFlags( uint32 nFlags )
{
	m->m_nTickFlags = nFlags;
	_RefreshDisplay();
}

/** Obtain the tick-flags as set with SetTickFlags()
 * \return The current tick-flags
 * \sa SetTickFlags()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

uint32 Slider::GetTickFlags() const
{
	return ( m->m_nTickFlags );
}

/** Set the static labels rendere at each end of the slider.
 * \par Description:
 *	The min/max labels are rendered below a horizontal slider
 *	and to the right of a vertical slider.
 * \par Note:
 *	Both label must be set for the labels to be rendered.
 * \param cMin - The label to be rendered at the "smaller" end of the slider.
 * \param cMax - The label to be rendered at the "greater" end of the slider.
 * \sa GetLimitLabels(), SetProgStrFormat()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::SetLimitLabels( const String & cMinLabel, const String & cMaxLabel )
{
	m->m_cMinLabel = cMinLabel;
	m->m_cMaxLabel = cMaxLabel;
	_RefreshDisplay();
}

/** Obtain the limit-labels as set with SetLimitLabels()
 * \param pcMinLabel - Pointer to a string receiving the min-label or NULL.
 * \param pcMaxLabel - Pointer to a string receiving the max-label or NULL.
 * \sa SetLimitLabels()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Slider::GetLimitLabels( String * pcMinLabel, String * pcMaxLabel )
{
	if( pcMinLabel != NULL )
	{
		*pcMinLabel = m->m_cMinLabel;
	}
	if( pcMaxLabel != NULL )
	{
		*pcMaxLabel = m->m_cMaxLabel;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::MouseDown( const Point & cPosition, uint32 nButtons )
{
	if( nButtons != 1 || IsEnabled() == false )
	{
		View::MouseDown( cPosition, nButtons );
		return;
	}

	m->m_cHitPos = cPosition - ValToPos( GetValue() );;
	m->m_bTrack = true;
	m->m_bChanged = false;
	MakeFocus( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( nButtons != 1 || IsEnabled() == false )
	{
		View::MouseUp( cPosition, nButtons, pcData );
		return;
	}

	m->m_bTrack = false;
	if( m->m_bChanged )
	{
		Invoke();	// Send a 'final' message
		m->m_bChanged = false;
	}
	//MakeFocus( false );
}

void Slider::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
		return;
	}
	if( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) )
	{
		MakeFocus();
	} else if( pzString[1] == 0 && ( pzString[0] == 0x1E || pzString[0] == 0x1D ) ) {
		SetValue( std::min( GetValue().AsFloat() + m->m_vSmallStep, m->m_vMax ) );
	} else if( pzString[1] == 0 && ( pzString[0] == 0x1F || pzString[0] == 0x1C ) ) {
		SetValue( std::max( GetValue().AsFloat() - m->m_vSmallStep, m->m_vMin ) );
	} else {
		Control::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( IsEnabled() == false )
	{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
		return;
	}

	if( m->m_bTrack )
	{
		SetValue( PosToVal( cNewPos - m->m_cHitPos ) );
	}
}

void Slider::RenderSlider( View * pcRenderView )
{
	Rect cBounds = GetNormalizedBounds();

	pcRenderView->FillRect( cBounds, GetBgColor() );

	pcRenderView->SetEraseColor( get_default_color( COL_SCROLLBAR_KNOB ) );

	pcRenderView->SetFgColor( get_default_color( COL_SCROLLBAR_BG ) );

	Rect cSliderBounds = GetSliderFrame();

	pcRenderView->DrawFrame( cSliderBounds, FRAME_RECESSED | FRAME_TRANSPARENT );

	cSliderBounds.Resize( 2.0f, 2.0f, -2.0f, -2.0f );

	if( cSliderBounds.IsValid() )
	{
		Point cPos = ValToPos( GetValue() );

		Rect cSliderBounds2 = cSliderBounds;

		if( m->m_eOrientation == HORIZONTAL )
		{
			cSliderBounds2.left = cPos.x;
			if( cPos.x + 1.0f < cSliderBounds.right )
			{
				cSliderBounds.right = cPos.x + 1.0f;
			}
			if( cSliderBounds2.left < cSliderBounds.left )
			{
				cSliderBounds2.left = cSliderBounds.left;
			}
		}
		else
		{
			if( cPos.y + 1.0f > cSliderBounds2.top )
			{
				cSliderBounds.top = cPos.y + 1.0f;
			}
			if( cPos.y < cSliderBounds2.bottom )
			{
				cSliderBounds2.bottom = cPos.y;
			}
		}
		if( cSliderBounds.IsValid() )
		{
			pcRenderView->FillRect( cSliderBounds, m->m_sSliderColor1 );
		}
		if( cSliderBounds2.IsValid() )
		{
			pcRenderView->FillRect( cSliderBounds2, m->m_sSliderColor2 );
		}
	}
	RenderTicks( pcRenderView );
	RenderLabels( pcRenderView );
	RenderKnob( pcRenderView );
}

void Slider::RenderKnob( View * pcRenderView )
{
	Rect cKnobFrame = GetKnobFrame() + ValToPos( GetValue(  ) );

	if( m->m_eKnobMode == KNOB_SQUARE )
	{
		if( IsEnabled() && HasFocus() )
		{
			pcRenderView->SetFgColor( get_default_color( COL_FOCUS ) );
			pcRenderView->DrawLine( Point( cKnobFrame.left, cKnobFrame.top ), Point( cKnobFrame.right, cKnobFrame.top ) );
			pcRenderView->DrawLine( Point( cKnobFrame.right, cKnobFrame.bottom ) );
			pcRenderView->DrawLine( Point( cKnobFrame.left, cKnobFrame.bottom ) );
			pcRenderView->DrawLine( Point( cKnobFrame.left, cKnobFrame.top ) );
			cKnobFrame.Resize(1, 1, -1, -1);
		}

		pcRenderView->SetEraseColor( get_default_color( COL_SCROLLBAR_KNOB ) );
		pcRenderView->DrawFrame( cKnobFrame, FRAME_RAISED );
	}
	else
	{
		Point cCenter = ValToPos( GetValue() );

		if( m->m_eOrientation == HORIZONTAL )
		{
			pcRenderView->SetFgColor( GetBgColor() );
			for( int i = 1; i < 6; ++i )
			{
				pcRenderView->DrawLine( Point( cCenter.x - i + 1.0f, cCenter.y + i ), Point( cCenter.x + i - 1.0f, cCenter.y + i ) );
			}
			pcRenderView->SetFgColor( 0, 0, 0 );
			pcRenderView->DrawLine( cCenter, Point( cCenter.x + 7.0f, cCenter.y + 7.0f ) );
			pcRenderView->DrawLine( Point( cCenter.x - 7.0f, cCenter.y + 7.0f ) );
			pcRenderView->DrawLine( cCenter );
			pcRenderView->DrawLine( Point( cCenter.x - 6.0f, cCenter.y + 6 ), Point( cCenter.x + 6.0f, cCenter.y + 6 ) );
		}
		else
		{
			pcRenderView->SetFgColor( GetBgColor() );
			for( int i = 1; i < 6; ++i )
			{
				pcRenderView->DrawLine( Point( cCenter.x - i, cCenter.y - i + 1.0f ), Point( cCenter.x - i, cCenter.y + i - 1.0f ) );
			}
			pcRenderView->SetFgColor( 0, 0, 0 );
			pcRenderView->DrawLine( cCenter, Point( cCenter.x - 7.0f, cCenter.y - 7.0f ) );
			pcRenderView->DrawLine( Point( cCenter.x - 7.0f, cCenter.y + 7.0f ) );
			pcRenderView->DrawLine( cCenter );
		}
	}
}

void Slider::RenderLabels( View * pcRenderView )
{
	Rect cBounds = GetNormalizedBounds();
	Rect cSliderFrame = GetSliderFrame();
	Point cCenter( cBounds.Width() * 0.5f, cBounds.Height(  ) * 0.5f );
	font_height sHeight;

	pcRenderView->GetFontHeight( &sHeight );

	pcRenderView->SetFgColor( 0, 0, 0 );
	if( m->m_eOrientation == HORIZONTAL )
	{
		if( m->m_cMinLabel.size() > 0 && m->m_cMaxLabel.size(  ) > 0 )
		{
			pcRenderView->MovePenTo( 0.0f, cSliderFrame.bottom + sHeight.ascender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING );
			pcRenderView->DrawString( m->m_cMinLabel );
			pcRenderView->MovePenTo( cBounds.right - GetStringWidth( m->m_cMaxLabel.c_str() ), cSliderFrame.bottom + sHeight.ascender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING );
			pcRenderView->DrawString( m->m_cMaxLabel );
		}
		String cProgress = GetProgressString();
		if( cProgress.size() > 0 )
		{
			pcRenderView->MovePenTo( 0.0f, cSliderFrame.top - sHeight.descender - TICK_SPACING - TICK_LENGTH - HLABEL_SPACING );
			pcRenderView->DrawString( cProgress );
		}
	}
	else
	{
		Rect cSliderBounds = GetSliderFrame();
		float vOffset = ceil( GetKnobFrame().Height(  ) * 0.5f ) + VLABEL_SPACING;

		if( m->m_cMaxLabel.size() > 0 )
		{
			pcRenderView->MovePenTo( cCenter.x - GetStringWidth( m->m_cMaxLabel ) * 0.5f, cSliderBounds.top + sHeight.descender - vOffset );
			pcRenderView->DrawString( m->m_cMaxLabel );
		}
		if( m->m_cMinLabel.size() > 0 )
		{
			pcRenderView->MovePenTo( cCenter.x - GetStringWidth( m->m_cMinLabel ) * 0.5f, cSliderBounds.bottom + sHeight.ascender + vOffset );
			pcRenderView->DrawString( m->m_cMinLabel );
		}
	}

}

void Slider::RenderTicks( View * pcRenderView )
{
	if( m->m_nNumTicks > 0 )
	{
		Rect cSliderFrame( GetSliderFrame() );

		float vScale = 1.0f / float ( m->m_nNumTicks - 1 );

		if( m->m_eOrientation == HORIZONTAL )
		{
			float vWidth = floor( cSliderFrame.Width() - 1.0f );

			float y1 = floor( cSliderFrame.top - TICK_SPACING );
			float y2 = y1 - TICK_LENGTH;
			float y4 = floor( cSliderFrame.bottom + TICK_SPACING );
			float y3 = y4 + TICK_LENGTH;

			pcRenderView->SetFgColor( get_default_color( COL_SHADOW ) );
			for( float i = 0; i < m->m_nNumTicks; i += 1.0f )
			{
				float x = floor( cSliderFrame.left + vWidth * i * vScale );

				if( m->m_nTickFlags & TICKS_ABOVE )
				{
					pcRenderView->DrawLine( Point( x, y1 ), Point( x, y2 ) );
				}
				if( m->m_nTickFlags & TICKS_BELOW )
				{
					pcRenderView->DrawLine( Point( x, y3 ), Point( x, y4 ) );
				}
			}
			pcRenderView->SetFgColor( get_default_color( COL_SHINE ) );
			for( float i = 0; i < m->m_nNumTicks; i += 1.0f )
			{
				float x = floor( cSliderFrame.left + vWidth * i * vScale ) + 1.0f;

				if( m->m_nTickFlags & TICKS_ABOVE )
				{
					pcRenderView->DrawLine( Point( x, y1 ), Point( x, y2 ) );
				}
				if( m->m_nTickFlags & TICKS_BELOW )
				{
					pcRenderView->DrawLine( Point( x, y3 ), Point( x, y4 ) );
				}
			}
		}
		else
		{
			float vHeight = cSliderFrame.Height() - 1.0f;

			float x1 = floor( cSliderFrame.left - TICK_SPACING );
			float x2 = x1 - TICK_LENGTH;
			float x4 = floor( cSliderFrame.right + TICK_SPACING );
			float x3 = x4 + TICK_LENGTH;

			pcRenderView->SetFgColor( get_default_color( COL_SHADOW ) );
			for( float i = 0; i < m->m_nNumTicks; i += 1.0f )
			{
				float y = floor( cSliderFrame.top + vHeight * i * vScale );

				if( m->m_nTickFlags & TICKS_ABOVE )
				{
					pcRenderView->DrawLine( Point( x1, y ), Point( x2, y ) );
				}
				if( m->m_nTickFlags & TICKS_BELOW )
				{
					pcRenderView->DrawLine( Point( x3, y ), Point( x4, y ) );
				}
			}
			pcRenderView->SetFgColor( get_default_color( COL_SHINE ) );
			for( float i = 0; i < m->m_nNumTicks; i += 1.0f )
			{
				float y = floor( cSliderFrame.top + vHeight * i * vScale ) + 1.0f;

				if( m->m_nTickFlags & TICKS_ABOVE )
				{
					pcRenderView->DrawLine( Point( x1, y ), Point( x2, y ) );
				}
				if( m->m_nTickFlags & TICKS_BELOW )
				{
					pcRenderView->DrawLine( Point( x3, y ), Point( x4, y ) );
				}
			}
		}

	}
}

float Slider::PosToVal( const Point & a_cPos ) const
{
	Rect cSliderFrame = GetSliderFrame();
	float vValue;

	Point cPos = a_cPos - cSliderFrame.LeftTop();

	if( m->m_eOrientation == HORIZONTAL )
	{
		if( m->m_nNumSteps > 1 )
		{
			float vAlign = ( cSliderFrame.Width() + 1.0f ) / float ( m->m_nNumSteps - 1 );

			cPos.x = floor( ( cPos.x + vAlign * 0.5f ) / vAlign ) * vAlign;
		}
		vValue = m->m_vMin + ( cPos.x / ( cSliderFrame.Width() + 1.0f ) ) * ( m->m_vMax - m->m_vMin );
	}
	else
	{
		if( m->m_nNumSteps > 1 )
		{
			float vAlign = ( cSliderFrame.Height() + 1.0f ) / float ( m->m_nNumSteps - 1 );

			cPos.y = floor( ( cPos.y + vAlign * 0.5f ) / vAlign ) * vAlign;
		}
		vValue = m->m_vMax + ( cPos.y / ( cSliderFrame.Height() + 1.0f ) ) * ( m->m_vMin - m->m_vMax );
	}
	if( vValue < m->m_vMin )
	{
		return ( m->m_vMin );
	}
	else if( vValue > m->m_vMax )
	{
		return ( m->m_vMax );
	}
	else
	{
		return ( vValue );
	}
}

Point Slider::ValToPos( float vVal ) const
{
	Rect cBounds = GetSliderFrame();
	Rect cSliderFrame = GetSliderFrame();
	float vPos = ( vVal - m->m_vMin ) / ( m->m_vMax - m->m_vMin );

	if( m->m_eOrientation == HORIZONTAL )
	{
		return ( Point( cSliderFrame.left + ( cSliderFrame.Width() ) * vPos, cBounds.top + ( cBounds.Height(  ) + 1.0f ) * 0.5f ) );
	}
	else
	{
		return ( Point( cBounds.left + ( cBounds.Width() + 1.0f ) * 0.5f, cSliderFrame.top + ( cSliderFrame.Height(  ) ) * ( 1.0f - vPos ) ) );
	}
}


Rect Slider::GetKnobFrame() const
{
	if( m->m_eOrientation == HORIZONTAL )
	{
		return ( Rect( -5.0f, -7.0f, 5.0f, 7.0f ) );
	}
	else
	{
		return ( Rect( -7.0f, -5.0f, 7.0f, 5.0f ) );
	}
}

Rect Slider::GetSliderFrame() const
{
	Rect cBounds( GetNormalizedBounds() );
	Rect cKnobFrame = GetKnobFrame();
	Rect cSliderBounds;
	Point cCenter( ( cBounds.Width() + 1.0f ) * 0.5f, ( cBounds.Height(  ) + 1.0f ) * 0.5f );

	float vSize = ceil( GetSliderSize() * 0.5f );

	if( m->m_eOrientation == HORIZONTAL )
	{
		float vKWidth = ceil( ( cKnobFrame.Width() + 1.0f ) * 0.5f );
		float vKHeight = ceil( ( cKnobFrame.Height() + 1.0f ) * 0.5f );
		float vCenter = vSize;

		if( m->m_nTickFlags & TICKS_ABOVE )
		{
			vCenter += TICK_SPACING + TICK_LENGTH;
		}


		cSliderBounds.top = 0;
		String cProgress = GetProgressString();
		if( cProgress.size() > 0 )
		{
			font_height sHeight;

			GetFontHeight( &sHeight );

			float vHeight = vSize + sHeight.ascender + sHeight.descender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING;

			if( vHeight > vCenter )
			{
				vCenter = vHeight;
			}
		}
		if( vCenter < vKHeight )
		{
			vCenter = vKHeight;
		}
//      cSliderBounds.bottom = cSliderBounds.top + ceil(GetSliderSize() * 0.5f);
//      cSliderBounds.top -= ceil( GetSliderSize() * 0.5f );
		cSliderBounds = Rect( vKWidth, vCenter - vSize, cBounds.right - vKWidth, vCenter + vSize );
	}
	else
	{
		float vKHeight = ceil( ( cKnobFrame.Height() + 1.0f ) * 0.5f );

		float vTFHeight = 0.0f;
		float vBFHeight = 0.0f;

		font_height sHeight;

		GetFontHeight( &sHeight );

		if( m->m_cMinLabel.size() > 0 )
		{
			vBFHeight = sHeight.ascender + sHeight.descender + VLABEL_SPACING;
		}
		if( m->m_cMaxLabel.size() > 0 )
		{
			vTFHeight = sHeight.ascender + sHeight.descender + VLABEL_SPACING;
		}
		cSliderBounds = Rect( cCenter.x - vSize, vKHeight + vTFHeight, cCenter.x + vSize, cBounds.bottom - vKHeight - vBFHeight );
	}
	cSliderBounds.Floor();
	return ( cSliderBounds );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Slider::Paint( const Rect & cUpdateRect )
{
	if( m->m_pcRenderBitmap != NULL )
	{
		DrawBitmap( m->m_pcRenderBitmap, cUpdateRect, cUpdateRect );
	}
}

void Slider::_RefreshDisplay()
{
	if( GetWindow() == NULL )
	{
		return;
	}
	Sync();
	RenderSlider( m->m_pcRenderView );
	if( m->m_pcRenderBitmap ) {
		m->m_pcRenderBitmap->Sync();
	}
	Invalidate();
	Flush();
}

void Slider::SetSteps( float vSmall, float vBig )
{
	m->m_vSmallStep = vSmall; m->m_vBigStep = vBig;
}

void Slider::GetSteps( float* pvSmall, float* pvBig ) const
{
	*pvSmall = m->m_vSmallStep; *pvBig = m->m_vBigStep;
}

void Slider::SetMinMax( float vMin, float vMax )
{
	m->m_vMin = vMin; m->m_vMax = vMax;
}

void Slider::Activated( bool bIsActive )
{
	_RefreshDisplay();
}

void Slider::__SL_reserved1__()
{
}

void Slider::__SL_reserved2__()
{
}

void Slider::__SL_reserved3__()
{
}

void Slider::__SL_reserved4__()
{
}

void Slider::__SL_reserved5__()
{
}
