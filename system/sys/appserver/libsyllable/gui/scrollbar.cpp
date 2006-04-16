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
#include <atheos/types.h>

#include <gui/scrollbar.h>
#include <util/looper.h>
#include <util/message.h>
#include <macros.h>

#include "stdbitmaps.h"

using namespace os;

//used when the scrollbar has been *hit*
enum
{ 
	HIT_NONE, 
	HIT_KNOB, 
	HIT_ARROW 
};

//used to set a timer on the scrollbar
enum
{ 
	ID_SCROLL_TIMER = 1 
};


/**internal*/
class ScrollBar::Private
{
public:

	Private()
	{
		m_nHitState = HIT_NONE; //start the state at hitting none
		m_nHitButton = 0;  //start the button hit at none
		m_vProportion = 0.1f; //start the proportiom moved at 0.1
		m_nOrientation = VERTICAL; //start the orientation(i.e., the way the scrollbar is displayed) as vertical
		m_vMin = 0.0f; //start the minimum at 0.0
		m_vMax = FLT_MAX; //start the maximum at FLT_MAX
		m_vSmallStep = 1.0f; //start the smallest step at 1.0
		m_vBigStep = 10.0f; //start the biggest step at 10.0
		m_bChanged = false; //nothing changed
		m_pcTarget = NULL; //the target is null
		m_bFirstTick = true; //first tick is true
		memset( m_abArrowStates, 0, sizeof( m_abArrowStates ) ); //allocate memory for the states of the arrows
	}

	View *m_pcTarget; //the target to send messages to
	float m_vMin; //the min step
	float m_vMax; //the max step
	float m_vProportion; //the proportion
	float m_vSmallStep; //the smallest step
	float m_vBigStep; //the biggest step
	int m_nOrientation; //the orientation
	Rect m_acArrowRects[4]; //the arrow rects
	bool m_abArrowStates[4]; //the states of the arrows
	Rect m_cKnobArea; //the area of the knob (e.g., size and position of the knob) 
	bool m_bChanged; //changed???
	bool m_bFirstTick; //is first tick
	Point m_cHitPos; //the position hit
	int m_nHitButton; //the button hit
	int m_nHitState; //the state hit
};


//should be a static member in os::Color32_s???
static Color32_s Tint( const Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

	if( r < 0 )
		r = 0;
	else if( r > 255 )
		r = 255;
	if( g < 0 )
		g = 0;
	else if( g > 255 )
		g = 255;
	if( b < 0 )
		b = 0;
	else if( b > 255 )
		b = 255;
	return ( Color32_s( r, g, b, sColor.alpha ) );
}


/** libSyllable os::ScrollBar
 * \par Description:
 *	ScrollBar constructor.
 * \param cFrame - The size and position of the ScrollBar
 * \param cName - The name of the ScrollBar.
 * \param pcMsg - The message for the ScrollBar.  Used to pass messages to your element
 * \param vMin - The minimum tick for the ScrollBar
 * \param vMax - The maximum tick for the ScrollBar
 * \param nOrientation - The position of the ScrollBar(use os::VERTICAL or os::HORIZONTAL)
 * \param nResizeMask - Determines what way the ScrollBar will follow the rest of the window.  Default is CF_FOLLOW_LEFT|CF_FOLLOW_TOP.
 * \author	Kurt Skauen with modifications by Rick Caudill(rick@syllable.org)
 *****************************************************************************/
ScrollBar::ScrollBar( const Rect & cFrame, const String& cName, Message * pcMsg, float vMin, float vMax, int nOrientation, uint32 nResizeMask ):Control( cFrame, cName, "", pcMsg, nResizeMask, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE )
{
	m = new Private;
	m->m_nOrientation = nOrientation;
	m->m_vMin = vMin;
	m->m_vMax = vMax;
	FrameSized( Point( 0.0f, 0.0f ) );
}

/** libSyllable os::ScrollBar
 * \par Description:
 *	The destructor for the ScrollBar.  Just sets the target's scrollbar property to NULL
 * \author	Kurt Skauen with modifications by Rick Caudill(rick@syllable.org)
 *****************************************************************************/
ScrollBar::~ScrollBar()
{
	if( m->m_pcTarget != NULL )
	{
		if( m->m_nOrientation == HORIZONTAL )
		{
			m->m_pcTarget->_SetHScrollBar( NULL );
		}
		else
		{
			m->m_pcTarget->_SetVScrollBar( NULL );
		}
	}
	delete m;
}

/** Sets the propotion...
 * \par Description:
 *	Sets the proportion on how the Scrollbar moves
 * \param vProp - the new proportion
 * \author	Kurt Skauen
 *****************************************************************************/
void ScrollBar::SetProportion( float vProp )
{
	m->m_vProportion = vProp;
	Invalidate( m->m_cKnobArea );
	Flush();
}

/** Gets the propotion...
 * \par Description:
 *	Gets the proportion on how the Scrollbar moves
 * \author	Kurt Skauen
 *****************************************************************************/
float ScrollBar::GetProportion() const
{
	return ( m->m_vProportion );
}

/** Sets the steps...
 * \par Description:
 *	Sets the steps on how much/little the Scrollbar moves
 * \param vSmall - the new smallest step
 * \param vBig   - the new largest step
 * \author	Kurt Skauen
 *****************************************************************************/
void ScrollBar::SetSteps( float vSmall, float vBig )
{
	m->m_vSmallStep = vSmall;
	m->m_vBigStep = vBig;
}

/** Gets the steps...
 * \par Description:
 *	Gets the steps on how much/little the Scrollbar moves
 * \param pvSmall - pointer to the smallest step
 * \param pvBig   - pointer to the bigest step
 * \author	Kurt Skauen
 *****************************************************************************/
void ScrollBar::GetSteps( float *pvSmall, float *pvBig ) const
{
	*pvSmall = m->m_vSmallStep;
	*pvBig = m->m_vBigStep;
}


/** Sets the minimum/maximum step...
 * \par Description:
 *	Sets the minimum and the maximum step that can be done in one click
 * \param vMin - the minimum step
 * \param vMax - the maximum step
 * \author	Kurt Skauen
 *****************************************************************************/
void ScrollBar::SetMinMax( float vMin, float vMax )
{
	m->m_vMin = vMin;
	m->m_vMax = vMax;
}


//Should we not have GetMinMax here???



/** Sets the target that the ScrollBar sends messages to.
 * \par Description:
 *	Sets the target that the ScrollBar sends messages to.
 * \param pcTarget - the target os::View 
 * \author	Kurt Skauen
 *****************************************************************************/
void ScrollBar::SetScrollTarget( View * pcTarget )
{
	if( m->m_pcTarget != NULL )
	{
		if( m->m_nOrientation == HORIZONTAL )
		{
			assert( m->m_pcTarget->GetHScrollBar() == this );
			m->m_pcTarget->_SetHScrollBar( NULL );
		}
		else
		{
			assert( m->m_pcTarget->GetVScrollBar() == this );
			m->m_pcTarget->_SetVScrollBar( NULL );
		}
	}
	m->m_pcTarget = pcTarget;
	if( m->m_pcTarget != NULL )
	{
		if( m->m_nOrientation == HORIZONTAL )
		{
			assert( m->m_pcTarget->GetHScrollBar() == NULL );
			m->m_pcTarget->_SetHScrollBar( this );
			SetValue( m->m_pcTarget->GetScrollOffset().x );
		}
		else
		{
			assert( m->m_pcTarget->GetVScrollBar() == NULL );
			m->m_pcTarget->_SetVScrollBar( this );
			SetValue( m->m_pcTarget->GetScrollOffset().y );
		}
	}
}


/** Gets the target that the ScrollBar sends messages to.
 * \par Description:
 *	Gets the target that the ScrollBar sends messages to. 
 * \author	Kurt Skauen
 *****************************************************************************/
View *ScrollBar::GetScrollTarget( void )
{
	return ( m->m_pcTarget );
}

//look at os::View
Point ScrollBar::GetPreferredSize( bool bLargest ) const
{
	float w = 16.0f; //hard-coded width?
	float l = ( bLargest ) ? COORD_MAX : 0;

	if( m->m_nOrientation == HORIZONTAL )
	{
		return ( Point( l, w ) );
	}
	else
	{
		return ( Point( w, l ) );
	}
}

//look at os::Control
void ScrollBar::PostValueChange( const Variant & cNewValue )
{
	if( m->m_pcTarget != NULL )
	{
		Point cPos = m->m_pcTarget->GetScrollOffset();

		if( m->m_nOrientation == HORIZONTAL )
		{
			cPos.x = -floor( cNewValue.AsFloat() );
		}
		else
		{
			cPos.y = -floor( cNewValue.AsFloat() );
		}
		m->m_pcTarget->ScrollTo( cPos );
		m->m_pcTarget->Flush();
	}
	Invalidate( m->m_cKnobArea );
	Flush();
}

//look at os::Control
void ScrollBar::LabelChanged( const String & cNewLabel )
{
}

//
void ScrollBar::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}

//look at os::Control
bool ScrollBar::Invoked( Message * pcMessage )
{
	Control::Invoked( pcMessage );
	if( m->m_nHitState == HIT_KNOB )
	{
		pcMessage->AddBool( "final", false );
	}
	else
	{
		pcMessage->AddBool( "final", true );
	}
	return ( true );
}


//look at os::Control
void ScrollBar::TimerTick( int nID )
{
	if( nID == ID_SCROLL_TIMER )
	{
		float vValue = GetValue();

		if( m->m_nHitButton & 0x01 )
		{
			vValue += m->m_vSmallStep;
		}
		else
		{
			vValue -= m->m_vSmallStep;
		}
		if( vValue < m->m_vMin )
		{
			vValue = m->m_vMin;
		}
		else if( vValue > m->m_vMax )
		{
			vValue = m->m_vMax;
		}
		SetValue( vValue );
		if( m->m_bFirstTick )
		{
			GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 30000LL, false );
			m->m_bFirstTick = false;
		}
	}
	else
	{
		Control::TimerTick( nID );
	}
}

//look at os::View
void ScrollBar::MouseDown( const Point & cPosition, uint32 nButtons )
{
	if( IsEnabled() == false ) //lets not let any mousedown if disabled
	{
		View::MouseDown( cPosition, nButtons );
		return;
	}

	MakeFocus( true );  //focus on this gui element
	m->m_nHitState = HIT_NONE; //make sure the hit state starts at none
	m->m_bChanged = false; //make sure changed is false


	/*iterate through the list of arrows seeing if one has been hit
      if so, tell what button has been hit, hit state will be hit_arrow,
      invalidate that rect, flush and add a timer.
    */
	for( int i = 0; i < 4; ++i )
	{
		if(m->m_acArrowRects[i].DoIntersect(cPosition)) //add && (i == 0 || i==3) for two button scrollbars
		{
			m->m_abArrowStates[i] = true;
			m->m_nHitButton = i;
			m->m_nHitState = HIT_ARROW;
			Invalidate( m->m_acArrowRects[i] );
			Flush();
			m->m_bFirstTick = true;
			GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 300000LL, true );
			return;
		}
	}

	/*has the knob been hit?*/
	if( m->m_cKnobArea.DoIntersect( cPosition ) )
	{
		Rect cKnobFrame = GetKnobFrame();

		/*has the knob's frame been hit*/
		if( cKnobFrame.DoIntersect( cPosition ) )
		{
			/*update the position and the hit state*/
			m->m_cHitPos = cPosition - cKnobFrame.LeftTop();
			m->m_nHitState = HIT_KNOB;
			return;
		}

		float vValue = GetValue(); //get the current value of the scrollbar

		//if horizontal
		if( m->m_nOrientation == HORIZONTAL )
		{
			if( cPosition.x < cKnobFrame.left )  //if pos is smaller than the knob frame's left corner
			{
				vValue -= m->m_vBigStep; //take away a big step
			}
			else if( cPosition.x > cKnobFrame.right ) //if pos is > knob frame's right corner
			{
				vValue += m->m_vBigStep; //add a big step
			}
		}
		else //we must be vertical
		{
			if( cPosition.y < cKnobFrame.top ) //if pos is smaller than the knob frame's top corner
			{
				vValue -= m->m_vBigStep; //take away a big step
			}
			else if( cPosition.y > cKnobFrame.bottom ) //if pos is greater than the knob frame's bottom corner
			{
				vValue += m->m_vBigStep; //add a big step
			}
		}

		if( vValue < m->m_vMin ) //if the value is less than the min value, the min value becomes new value 
		{
			vValue = m->m_vMin;
		}
		else if( vValue > m->m_vMax ) //if the value is greater than the max value, the max value becomes new value
		{
			vValue = m->m_vMax;
		}
		SetValue( vValue ); //set the value
	}

}

//look at os::View
void ScrollBar::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( IsEnabled() == false ) //if disabled, do nothing
	{
		View::MouseUp( cPosition, nButtons, pcData );
		return;
	}

	if( m->m_nHitState == HIT_ARROW ) //if the arrow has been hit
	{
		float vValue = GetValue();  //get the current value
		bool bChanged = false;  //changed, nope

		if( m->m_bFirstTick ) //if first tick
		{
			for( int i = 0; i < 4; ++i )
			{
				if( m->m_acArrowRects[i].DoIntersect( cPosition ) )
				{
					if( i == m->m_nHitButton )
					{
						bChanged = true;

						//add reverse polarity here, when click left button add, and when click any other button, subtract
						//pretty cool, eh?
						if( i & 0x01 )
						{
							vValue += m->m_vSmallStep;
						}
						else
						{
							vValue -= m->m_vSmallStep;
						}
					}
					break;
				}
			}
		}
		//if the arrow state for the hit button is true
		if( m->m_abArrowStates[m->m_nHitButton] )
		{
			//remove timer and make false
			GetLooper()->RemoveTimer( this, ID_SCROLL_TIMER );
			m->m_abArrowStates[m->m_nHitButton] = false;
		}

		//invalidate the correct rect
		Invalidate( m->m_acArrowRects[m->m_nHitButton] );
		
		//if changed, check values and set correct value
		if( bChanged )
		{
			if( vValue < m->m_vMin )
			{
				vValue = m->m_vMin;
			}
			else if( vValue > m->m_vMax )
			{
				vValue = m->m_vMax;
			}
			SetValue( vValue );
		}
		Flush();
	}

	//if hit knob and changed invoke final message and set changed to false
	else if( m->m_nHitState == HIT_KNOB )
	{
		if( m->m_bChanged )
		{
			Invoke();	// Send a 'final' message
			m->m_bChanged = false;
		}
	}

	//reset to hit none
	//and then focus somewhere else
	m->m_nHitState = HIT_NONE;
	MakeFocus( false );
}

//look at os::View
void ScrollBar::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	//if not enabled then disable mousemove
	if( IsEnabled() == false )
	{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
		return;
	}

	//check to see if we hit an arrow
	if( m->m_nHitState == HIT_ARROW )
	{
		int i;

		//iterate through rects and see if we hit one
		for( i = 0; i < 4; ++i )
		{
			if( m->m_acArrowRects[i].DoIntersect( cNewPos ) )
			{
				break;
			}
		}
		//bHit becomes the button we hit
		bool bHit = m->m_nHitButton == i;


		//if bHit is not equal to the arrow state that was hit???
		if( bHit != m->m_abArrowStates[m->m_nHitButton] )
		{
			//if the arrow state that was hit is true, remove the timer
			if( m->m_abArrowStates[m->m_nHitButton] )
			{
				GetLooper()->RemoveTimer( this, ID_SCROLL_TIMER );
			}
			else //if not true
			{
				//would it not be easier to create a bool here and then if m_bFirstTick, true???
				if( m->m_bFirstTick )  
				{
					GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 300000LL, true );
				}
				else
				{
					GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 30000LL, false );
				}
			}

			//sync everything up
			m->m_abArrowStates[m->m_nHitButton] = bHit;
			Invalidate( m->m_acArrowRects[m->m_nHitButton] );
			Flush();
		}

	}
	
	//else we hit the knob so lets move :)
	else if( m->m_nHitState == HIT_KNOB )
	{
		SetValue( _PosToVal( cNewPos ) );
	}
}

//look at os::View
void ScrollBar::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
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

//look at os::View
void ScrollBar::WheelMoved( const Point & cDelta )
{
	float vValue = GetValue();

	//is disabled, do nothing
	if( IsEnabled() == false )
	{
		View::WheelMoved( cDelta );
		return;
	}

	//if vertical and pos is not equal to 0
	if( m->m_nOrientation == VERTICAL && cDelta.y != 0.0f )
	{
		//change the value
		vValue += cDelta.y * m->m_vSmallStep;
	}
	//if horizontal and not equal to 0. change value again
	else if( m->m_nOrientation == HORIZONTAL && cDelta.x != 0.0f )
	{
		vValue += cDelta.y * m->m_vSmallStep;
	}
	
	//check to see if value is less than min, if so then make it min
	if( vValue < m->m_vMin )
	{
		vValue = m->m_vMin;
	}
	//check to see if value is greater than max, if so then make it max
	else if( vValue > m->m_vMax )
	{
		vValue = m->m_vMax;
	}

	//set the value to the current value
	SetValue( vValue ); 
}

//look at os::View
void ScrollBar::Activated( bool bIsActive )
{
	Invalidate();
	Flush();
}

//look at os::View
void ScrollBar::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();
	Rect cKnobFrame = GetKnobFrame();

	if( IsEnabled() )
	{
		SetEraseColor( get_default_color( COL_SCROLLBAR_KNOB ) );
	}
	else
	{
		SetEraseColor( get_default_color( COL_NORMAL ) );
	}

	if( IsEnabled() && HasFocus() )
	{
		SetFgColor( get_default_color( COL_FOCUS ) );
		DrawLine( Point( cKnobFrame.left, cKnobFrame.top ), Point( cKnobFrame.right, cKnobFrame.top ) );
		DrawLine( Point( cKnobFrame.right, cKnobFrame.bottom ) );
		DrawLine( Point( cKnobFrame.left, cKnobFrame.bottom ) );
		DrawLine( Point( cKnobFrame.left, cKnobFrame.top ) );
		cKnobFrame.Resize(1, 1, -1, -1);
	}

	DrawFrame( cKnobFrame, FRAME_RAISED );

	if( m->m_nOrientation == HORIZONTAL )
	{
		if( cKnobFrame.Width() > 30.0f )
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.left + ( cKnobFrame.Width() + 1.0f ) * 0.5f;

			SetFgColor( get_default_color( COL_SHINE ) );
			DrawLine( Point( vCenter - vDist, cKnobFrame.top + 4.0f ), Point( vCenter - vDist, cKnobFrame.bottom - 4.0f ) );
			DrawLine( Point( vCenter, cKnobFrame.top + 4.0f ), Point( vCenter, cKnobFrame.bottom - 4.0f ) );
			DrawLine( Point( vCenter + vDist, cKnobFrame.top + 4.0f ), Point( vCenter + vDist, cKnobFrame.bottom - 4.0f ) );

			SetFgColor( get_default_color( COL_SHADOW ) );
			vCenter -= 1.0f;
			DrawLine( Point( vCenter - vDist, cKnobFrame.top + 4.0f ), Point( vCenter - vDist, cKnobFrame.bottom - 4.0f ) );
			DrawLine( Point( vCenter, cKnobFrame.top + 4.0f ), Point( vCenter, cKnobFrame.bottom - 4.0f ) );
			DrawLine( Point( vCenter + vDist, cKnobFrame.top + 4.0f ), Point( vCenter + vDist, cKnobFrame.bottom - 4.0f ) );
		}
	}
	else
	{
		if( cKnobFrame.Height() > 30.0f )
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.top + ( cKnobFrame.Height() + 1.0f ) * 0.5f;

			SetFgColor( get_default_color( COL_SHINE ) );
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter - vDist ), Point( cKnobFrame.right - 4.0f, vCenter - vDist ) );
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter ), Point( cKnobFrame.right - 4.0f, vCenter ) );
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter + vDist ), Point( cKnobFrame.right - 4.0f, vCenter + vDist ) );

			SetFgColor( get_default_color( COL_SHADOW ) );
			vCenter -= 1.0f;
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter - vDist ), Point( cKnobFrame.right - 4.0f, vCenter - vDist ) );
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter ), Point( cKnobFrame.right - 4.0f, vCenter ) );
			DrawLine( Point( cKnobFrame.left + 4.0f, vCenter + vDist ), Point( cKnobFrame.right - 4.0f, vCenter + vDist ) );
		}
	}

	if( IsEnabled() )
	{
		SetFgColor( get_default_color( COL_SCROLLBAR_BG ) );
	}
	else
	{
		SetFgColor( get_default_color( COL_NORMAL ) );
	}

	cBounds.left += 1;
	cBounds.top += 1;
	cBounds.right -= 1;
	cBounds.bottom -= 1;

	Rect cFrame( m->m_cKnobArea );

	if( m->m_nOrientation == HORIZONTAL )
	{
		cFrame.right = cKnobFrame.left - 1.0f;
	}
	else
	{
		cFrame.bottom = cKnobFrame.top - 1.0f;
	}
	FillRect( cFrame );

	cFrame = m->m_cKnobArea;
	if( m->m_nOrientation == HORIZONTAL )
	{
		cFrame.left = cKnobFrame.right + 1.0f;
	}
	else
	{
		cFrame.top = cKnobFrame.bottom + 1.0f;
	}
	FillRect( cFrame );
	if( m->m_nOrientation == HORIZONTAL )
	{
		SetFgColor( Tint( get_default_color( COL_SHADOW ), 0.5f ) );
		DrawLine( Point( m->m_cKnobArea.left, m->m_cKnobArea.top - 1.0f ), Point( m->m_cKnobArea.right, m->m_cKnobArea.top - 1.0f ) );
		SetFgColor( Tint( get_default_color( COL_SHINE ), 0.6f ) );
		DrawLine( Point( m->m_cKnobArea.left, m->m_cKnobArea.bottom + 1.0f ), Point( m->m_cKnobArea.right, m->m_cKnobArea.bottom + 1.0f ) );
	}
	else
	{
		SetFgColor( Tint( get_default_color( COL_SHADOW ), 0.5f ) );
		DrawLine( Point( m->m_cKnobArea.left - 1.0f, m->m_cKnobArea.top ), Point( m->m_cKnobArea.left - 1.0f, m->m_cKnobArea.bottom ) );
		SetFgColor( Tint( get_default_color( COL_SHINE ), 0.6f ) );
		DrawLine( Point( m->m_cKnobArea.right + 1.0f, m->m_cKnobArea.top ), Point( m->m_cKnobArea.right + 1.0f, m->m_cKnobArea.bottom ) );
	}


	for( int i = 0; i < 4; ++i )
	{
		if( m->m_acArrowRects[i].IsValid()) //add && (i == 0 || i==3) to for only two button scrollbars
		{
			Rect cBmRect;
			Bitmap *pcBitmap;

			if( m->m_nOrientation == HORIZONTAL )
			{
				pcBitmap = get_std_bitmap( ( ( i & 0x01 ) ? BMID_ARROW_RIGHT : BMID_ARROW_LEFT ), 0, &cBmRect );
			}
			else
			{
				pcBitmap = get_std_bitmap( ( ( i & 0x01 ) ? BMID_ARROW_DOWN : BMID_ARROW_UP ), 0, &cBmRect );
			}
			DrawFrame( m->m_acArrowRects[i], ( m->m_abArrowStates[i] ? FRAME_RECESSED : FRAME_RAISED ) | FRAME_THIN );

			SetDrawingMode( DM_OVER );

			DrawBitmap( pcBitmap, cBmRect, cBmRect.Bounds() + Point( ( m->m_acArrowRects[i].Width(  ) + 1.0f ) * 0.5 - ( cBmRect.Width(  ) + 1.0f ) * 0.5, ( m->m_acArrowRects[i].Height(  ) + 1.0f ) * 0.5 - ( cBmRect.Height(  ) + 1.0f ) * 0.5 ) + m->m_acArrowRects[i].LeftTop(  ) );

			SetDrawingMode( DM_COPY );

		}
	}
}

/**internal*/
float ScrollBar::_PosToVal( Point cPos ) const
{
	float vValue = m->m_vMin; //lets start with the min

	cPos -= m->m_cHitPos; //pos - hitPos

	//if horizontal
	if( m->m_nOrientation == HORIZONTAL )
	{
		float vSize = ( m->m_cKnobArea.Width() + 1.0f ) * m->m_vProportion; //size becomes (width + 1) * proportion
		float vLen = ( m->m_cKnobArea.Width() + 1.0f ) - vSize; //length becomes (width + 1) - size

		if( vLen > 0.0f ) //so if length is greater than 0
		{
			//calculate pos
			//value = min + (max - min) * pos
			float vPos = ( cPos.x - m->m_cKnobArea.left ) / vLen;
			vValue = m->m_vMin + ( m->m_vMax - m->m_vMin ) * vPos;
		}
	}
	else //if vertical
	{

		//size is (height + 1) * proportion
		//len is (height + 1) - size;
		float vSize = ( m->m_cKnobArea.Height() + 1.0f ) * m->m_vProportion;
		float vLen = ( m->m_cKnobArea.Height() + 1.0f ) - vSize;

		if( vLen > 0.0f ) //if length is greater than 0
		{
			//calculate pos
			//value = min + (max-min) * pos
			float vPos = ( cPos.y - m->m_cKnobArea.top ) / vLen;
			vValue = m->m_vMin + ( m->m_vMax - m->m_vMin ) * vPos;
		}
	}
	//return the miniumum of the max of (the value and the min) or the max
	return ( std::min( std::max( vValue, m->m_vMin ), m->m_vMax ) );
}

//see os::View
void ScrollBar::FrameSized( const Point & cDelta )
{
	Rect cBounds = GetBounds(); //get the current bounds
	Rect cArrowRect = cBounds; //make an arrow rect using bounds

	m->m_cKnobArea = cBounds; // the knob area is the same as the whole bounds

	//if horizontal
	if( m->m_nOrientation == HORIZONTAL )
	{
		//the right part of the rect will become the height +1 * 0.7??? -1.0f   have no clue why such a number  lol
		cArrowRect.right = ceil( ( cArrowRect.Height() + 1.0f ) * 0.7 ) - 1.0f;  //ahhh, hard-coded numbers
		float vWidth = cArrowRect.Width() + 1.0f; //the width will become the whole width + 1

		m->m_acArrowRects[0] = cArrowRect; // the first arrow's rect will be the current arrow rect
		m->m_acArrowRects[1] = cArrowRect + Point( vWidth, 0.0f );  //the second arrow's rect will be current rect plus the width
		m->m_acArrowRects[2] = cArrowRect + Point( cBounds.Width() - vWidth * 2.0f + 1.0f, 0.0f ); //for the next two, do the same thing
		m->m_acArrowRects[3] = cArrowRect + Point( cBounds.Width() - vWidth + 1.0f, 0.0f );

		if( m->m_acArrowRects[0].right > m->m_acArrowRects[3].left )  //if the first arrow's right is greater than the last left
		{
			//adjust values
			m->m_acArrowRects[0].right = floor( ( cBounds.Width() + 1.0f ) * 0.5f - 1.0f );
			m->m_acArrowRects[3].left = m->m_acArrowRects[0].right + 1.0f;
			m->m_acArrowRects[1].left = m->m_acArrowRects[0].right + 1.0f;
			m->m_acArrowRects[1].right = m->m_acArrowRects[1].left - 1.0f;
			m->m_acArrowRects[2].right = m->m_acArrowRects[3].left - 1.0f;
			m->m_acArrowRects[2].left = m->m_acArrowRects[2].right + 1.0f;
		}
		else if( m->m_acArrowRects[1].right + 16.0f > m->m_acArrowRects[2].left ) //if second arrow's right + 16 is greater than third's left
		{
			//adjust values
			m->m_acArrowRects[1].right = m->m_acArrowRects[1].left - 1.0f;
			m->m_acArrowRects[2].left = m->m_acArrowRects[2].right + 1.0f;
		}
		//adjust values
		m->m_cKnobArea.left = m->m_acArrowRects[1].right + 1.0f;
		m->m_cKnobArea.right = m->m_acArrowRects[2].left - 1.0f;
		m->m_cKnobArea.top += 1.0f;
		m->m_cKnobArea.bottom -= 1.0f;
	}
	else //must be vertical
	{
		//again, a hardcoded mess
		cArrowRect.bottom = ceil( ( cArrowRect.Width() + 1.0f ) * 0.7 ) - 1.0f;

		float vWidth = cArrowRect.Height() + 1.0f; //width becomes height + 1

		//adjust arrow rects again
		m->m_acArrowRects[0] = cArrowRect;
		m->m_acArrowRects[1] = cArrowRect + Point( 0.0f, vWidth );
		m->m_acArrowRects[2] = cArrowRect + Point( 0.0f, cBounds.Height() - vWidth * 2.0f + 1.0f );
		m->m_acArrowRects[3] = cArrowRect + Point( 0.0f, cBounds.Height() - vWidth + 1.0f );

		//if first arrow's bottom is greater than lasts' top
		if( m->m_acArrowRects[0].bottom > m->m_acArrowRects[3].top )
		{
			//adjust rects
			m->m_acArrowRects[0].bottom = floor( ( cBounds.Width() + 1.0f ) * 0.5f - 1.0f );
			m->m_acArrowRects[3].top = m->m_acArrowRects[0].bottom + 1.0f;
			m->m_acArrowRects[1].top = m->m_acArrowRects[0].bottom + 1.0f;
			m->m_acArrowRects[1].bottom = m->m_acArrowRects[1].top - 1.0f;
			m->m_acArrowRects[2].bottom = m->m_acArrowRects[3].top - 1.0f;
			m->m_acArrowRects[2].top = m->m_acArrowRects[2].bottom + 1.0f;
		}
		//else if seconds' bottom +16 is > third's top
		else if( m->m_acArrowRects[1].bottom + 16.0f > m->m_acArrowRects[2].top )
		{
			//adjust rects
			m->m_acArrowRects[1].bottom = m->m_acArrowRects[1].top - 1.0f;
			m->m_acArrowRects[2].top = m->m_acArrowRects[2].bottom + 1.0f;
		}
		//adjust rects
		m->m_cKnobArea.top = m->m_acArrowRects[1].bottom + 1.0f;
		m->m_cKnobArea.bottom = m->m_acArrowRects[2].top - 1.0f;
		m->m_cKnobArea.left += 1.0f;
		m->m_cKnobArea.right -= 1.0f;
	}
}

/** Gets the frame of the Knob
 * \par Description:
 *	Gets the frame of the Knob(i.e., the part of the scrollbar that moves)
 * \author	Kurt Skauen
 *****************************************************************************/
Rect ScrollBar::GetKnobFrame( void ) const
{
	Rect cBounds = GetBounds();
	Rect cRect;

	cBounds.left += 1.0f;
	cBounds.top += 1.0f;
	cBounds.right -= 1.0f;
	cBounds.bottom -= 1.0f;

	cRect = m->m_cKnobArea;

	float vValue = GetValue().AsFloat(  );

	if( vValue > m->m_vMax )
	{
		vValue = m->m_vMax;
	}
	else if( vValue < m->m_vMin )
	{
		vValue = m->m_vMin;
	}
	if( m->m_nOrientation == HORIZONTAL )
	{
		float vViewLength = m->m_cKnobArea.Width() + 1.0f;
		float vSize = std::max( float ( SB_MINSIZE ), vViewLength * m->m_vProportion );
		float vLen = vViewLength - vSize;
		float vDelta = m->m_vMax - m->m_vMin;

		if( vDelta > 0.0f )
		{
			cRect.left = cRect.left + vLen * ( vValue - m->m_vMin ) / vDelta;
			cRect.right = cRect.left + vSize - 1.0f;
		}
	}
	else
	{
		float vViewLength = m->m_cKnobArea.Height() + 1.0f;
		float vSize = std::max( float ( SB_MINSIZE ), vViewLength * m->m_vProportion );
		float vLen = vViewLength - vSize;
		float vDelta = m->m_vMax - m->m_vMin;

		if( vDelta > 0.0f )
		{
			cRect.top = cRect.top + vLen * ( vValue - m->m_vMin ) / vDelta;
			cRect.bottom = cRect.top + vSize - 1.0f;
		}
	}
	return ( cRect & m->m_cKnobArea );
}

/**internal*/
void ScrollBar::__SB_reserved1__()
{
}

/**internal*/
void ScrollBar::__SB_reserved2__()
{
}

/**internal*/
void ScrollBar::__SB_reserved3__()
{
}

/**internal*/
void ScrollBar::__SB_reserved4__()
{
}

/**internal*/
void ScrollBar::__SB_reserved5__()
{
}
