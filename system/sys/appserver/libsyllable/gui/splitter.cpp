/*  Splitter class
 *  Copyright (C) 2002 Sebastien Keim
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <gui/view.h>
#include <gui/rect.h>
#include <gui/splitter.h>
#include <util/application.h>

#define SEPARATOR_WIDTH   7
#define TRACK_BUTTON     1

#define POINTERH_WIDTH  7
#define POINTERH_HEIGHT 14
#define POINTERV_WIDTH  14
#define POINTERV_HEIGHT 7

using namespace os;
using namespace std;

namespace os_priv
{




	static uint8 g_anMouseImgH[] = {
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,

		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,
		0x00, 0x02, 0x00, 0x02, 0x02, 0x02, 0x00,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,

		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x00, 0x02, 0x00, 0x02, 0x02, 0x02, 0x00,
		0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,

		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,

	};

	static uint8 g_anMouseImgV[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00,
		0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

/*
static uint8 g_anMouseImgV[]=
{
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
    
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,    
    0x00,0x02,0x02,0x00,0x02,0x02,0x00,    
    0x00,0x02,0x02,0x00,0x02,0x02,0x00,
    
    0x02,0x02,0x02,0x00,0x02,0x02,0x02,
    0x02,0x02,0x02,0x00,0x02,0x02,0x02,
     
    0x00,0x02,0x02,0x00,0x02,0x02,0x00,
    0x00,0x02,0x02,0x00,0x02,0x02,0x00,
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
     
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,
    0x00,0x00,0x02,0x00,0x02,0x00,0x00,    
};

*/

//**********************************************************************
//   SplitterSeparator
//**********************************************************************

	class SplitterSeparator:public View
	{
	      private:
		orientation m_eOrientation;
		Splitter *m_pSplitter;

	      public:
	        SplitterSeparator( const Rect & cFrame, const String & cName, Splitter * splitter, uint32 nResizeMask = CF_FOLLOW_TOP | CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT ):View( cFrame, cName, nResizeMask ), m_eOrientation( HORIZONTAL ), m_pSplitter( splitter )
		{
		}


		void Paint( const Rect & cUpdateRect )
		{
			View::Paint( cUpdateRect );
			Rect r = GetBounds();

			if( m_eOrientation == HORIZONTAL )
			{
				float x1 = std::max( r.left, cUpdateRect.left );
				float x2 = std::min( r.right, cUpdateRect.right );
				float y1 = ( r.bottom - r.top ) / 2 - 2;
				float y2 = ( r.bottom - r.top ) / 2 + 2;

				if( y1 >= cUpdateRect.top && y1 <= cUpdateRect.bottom )
				{
					SetFgColor( get_default_color( COL_SHADOW ) );
					DrawLine( Point( x1, y1 ), Point( x2, y1 ) );
				}

				if( y2 >= cUpdateRect.top && y2 <= cUpdateRect.bottom )
				{
					SetFgColor( get_default_color( COL_SHINE ) );
					DrawLine( Point( x1, y2 ), Point( x2, y2 ) );
				}
			}
			else
			{
				float y1 = std::max( r.top, cUpdateRect.top );
				float y2 = std::min( r.bottom, cUpdateRect.bottom );
				float x1 = ( r.right - r.left ) / 2 - 2;
				float x2 = ( r.right - r.left ) / 2 + 2;

				if( x1 >= cUpdateRect.left && x1 <= cUpdateRect.right )
				{
					SetFgColor( get_default_color( COL_SHADOW ) );
					DrawLine( Point( x1, y1 ), Point( x1, y2 ) );
				}

				if( x2 >= cUpdateRect.left && x2 <= cUpdateRect.right )
				{
					SetFgColor( get_default_color( COL_SHINE ) );
					DrawLine( Point( x2, y1 ), Point( x2, y2 ) );
				}
			}
		}


		void MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
		{


			if( nCode == MOUSE_ENTERED )
			{
				if( m_eOrientation == HORIZONTAL )
				{
					Application::GetInstance()->PushCursor( MPTR_MONO, g_anMouseImgH, POINTERH_WIDTH, POINTERH_HEIGHT, IPoint( POINTERH_WIDTH / 2, POINTERH_HEIGHT / 2 ) );
				}
				else
				{
					Application::GetInstance()->PushCursor( MPTR_MONO, g_anMouseImgV, POINTERV_WIDTH, POINTERV_HEIGHT, IPoint( POINTERV_WIDTH / 2, POINTERV_HEIGHT / 2 ) );

				}
			/*	if( ( nButtons == TRACK_BUTTON ) && ( pcData == NULL ) )
				{
					m_pSplitter->StartTracking( cNewPos );
				}*/
			}
			else if( nCode == MOUSE_EXITED )
			{
				Application::GetInstance()->PopCursor(  );
			}


			View::MouseMove( cNewPos, nCode, nButtons, pcData );
		}

		void MouseDown( const Point & cPosition, uint32 nButtons )
		{
			if( nButtons == TRACK_BUTTON )
			{
				m_pSplitter->StartTracking( cPosition );
				MakeFocus();
			}
			View::MouseDown( cPosition, nButtons );
		}



		void SetOrientation( orientation eOrientation )
		{
			m_eOrientation = eOrientation;
			Invalidate();
		}
	};
}				//namespace os_priv

//**********************************************************************
//   Splitter
//**********************************************************************

class Splitter::Private
{
    public:
	View*	m_pView1;
	View*	m_pView2;
	float m_vLimit1;
	float m_vLimit2;
	bool m_Tracking;
	Point m_OldPosition;
	float m_SeparatorWidth;

	float m_vSplitPos;

	orientation m_eOrientation;
	os_priv::SplitterSeparator * m_pSeparator;
};


/** Spliter constructor.
* 
* \par Description:  
* \par Notes:
* \par Warning:
* \param  cFrame The position and size of the splitter view
* \param  cTitle The title of the splitter
* \param  pView1 The top(left) subview
* \param  pView2 The bottom(right) subview
* \param  eOrientation The direction of the separator
* \param  nResizeMask View resize mask
* \param  nFlags  View flags 
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
Splitter::Splitter( const Rect & cFrame, const String & cTitle, View * pView1, View * pView2, orientation eOrientation, uint32 nResizeMask, uint32 nFlags ):View( cFrame, cTitle, nResizeMask, nFlags )
{

	m = new Private;
	m->m_pView1 = pView1;
	m->m_pView2 = pView2;
	m->m_vLimit1 = 0.0;
	m->m_vLimit2 = 0.0;
	m->m_vSplitPos = 0.5;

	m->m_SeparatorWidth = SEPARATOR_WIDTH;
	m->m_Tracking = false;

	// place the first view
	m->m_pView1->SetResizeMask( CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
	AddChild( m->m_pView1 );

	//place the separator
	Rect rs = Rect( 0, 0, 10, 10 );

	m->m_pSeparator = new os_priv::SplitterSeparator( rs, "s", this );
	AddChild( m->m_pSeparator );

	// place the second view
	m->m_pView2->SetResizeMask( CF_FOLLOW_ALL );
	AddChild( m->m_pView2 );

	SetOrientation( eOrientation );

	AdjustLayout();
}

/** Refresh the layout of the splitter.
* 
* \par Description:
* This method refreshes the subviews' positions. It tries to keep
* the relative sizes of the subviews when the separator position is computed.
* \par Notes:
* You should call this method when you change the subviews' frames.
* \par Warning:
* \param  
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::AdjustLayout()
{
	// adjust frame views to Splitter to the splitter bounds
	Rect r = GetBounds();
	Rect f1 = m->m_pView1->GetFrame();
	Rect f2 = m->m_pView2->GetFrame();
	Rect rs;

	float k = m->m_vSplitPos;

	if( m->m_eOrientation == VERTICAL )
	{
		float	vLimit1 = std::max( m->m_vLimit1, m->m_pView1->GetPreferredSize( false ).y );
		float	vLimit2 = std::max( m->m_vLimit2, m->m_pView2->GetPreferredSize( false ).y );
		float	vMinSpace = vLimit1 + vLimit2 + m->m_SeparatorWidth;
		float	vFreeSpace = r.Height() - vMinSpace;
		float	vSize1 = vLimit1 + k * vFreeSpace;

		f1 = Rect( r.left, r.top, r.right, r.top + vSize1 );
		rs = Rect( r.left, f1.bottom + 1, r.right, f1.bottom + m->m_SeparatorWidth );
		f2 = Rect( r.left, rs.bottom + 1, r.right, r.bottom );

		m->m_pView1->SetFrame( f1.Floor() );
		m->m_pSeparator->SetFrame( rs.Floor() );
		m->m_pView2->SetFrame( f2.Floor() );
	}
	else
	{
		float	vLimit1 = std::max( m->m_vLimit1, m->m_pView1->GetPreferredSize( false ).x );
		float	vLimit2 = std::max( m->m_vLimit2, m->m_pView2->GetPreferredSize( false ).x );
		float	vMinSpace = vLimit1 + vLimit2 + m->m_SeparatorWidth;
		float	vFreeSpace = r.Width() - vMinSpace;
		float	vSize1 = vLimit1 + k * vFreeSpace;

		f1 = Rect( r.left, r.top, r.left + vSize1, r.bottom );
		rs = Rect( f1.right + 1, r.top, f1.right + m->m_SeparatorWidth, r.bottom );
		f2 = Rect( rs.right + 1, r.top, r.right, r.bottom );

		m->m_pView1->SetFrame( f1.Floor() );
		m->m_pSeparator->SetFrame( rs.Floor() );
		m->m_pView2->SetFrame( f2.Floor() );
	}
}

Splitter::~Splitter()
{
	delete m;
}

/** Change the position of the split bar.
*
* \par Description:
* Move the separator bar. For instance, a value of -100 will move the
* separator bar 100 pixels to the left (or up, if the splitter is vertical).
* \par Notes:
* \par Warning:
* This method works with pixel values, normally you should use SetSplitRatio().
* \param  fValue relative value of the deplacement
* \return
* \sa SetSplitRatio()
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SplitBy( float fValue )
{
	Rect f1 = m->m_pView1->GetFrame();
	Rect f2 = m->m_pView2->GetFrame();

	if( m->m_eOrientation == VERTICAL )
	{
		if( fValue < 0 )
		{
			float	vLimit = m->m_pView1->GetPreferredSize( false ).y;
			vLimit = std::max( m->m_vLimit1, vLimit );
			
			if( f1.bottom - f1.top + fValue < vLimit )
			{
				fValue = vLimit - f1.bottom + f1.top;
				//m->m_Tracking = false;
			}
			m->m_pView1->ResizeBy( Point( 0, (int)fValue ) );
			m->m_pSeparator->MoveBy( Point( 0, (int)fValue ) );
			f2.top += (int)fValue;
			m->m_pView2->SetFrame( f2 );
		}
		else if( fValue > 0 )
		{
			float	vLimit = m->m_pView2->GetPreferredSize( false ).y;
			vLimit = std::max( m->m_vLimit2, vLimit );
			
			if( f2.bottom - f2.top - fValue < vLimit )
			{
				fValue = -vLimit + f2.bottom - f2.top;
				//m->m_Tracking = false;
			}
			f2.top += (int)fValue;
			m->m_pView2->SetFrame( f2 );
			m->m_pSeparator->MoveBy( Point( 0, (int)fValue ) );
			m->m_pView1->ResizeBy( Point( 0, (int)fValue ) );
		}

		float	vLimit1 = std::max( m->m_vLimit1, m->m_pView1->GetPreferredSize( false ).y );
		float	vLimit2 = std::max( m->m_vLimit2, m->m_pView2->GetPreferredSize( false ).y );
		m->m_vSplitPos = (f1.Height() - vLimit1) / (GetBounds().Height() - vLimit1 - vLimit2 - m->m_SeparatorWidth);
	}
	else
	{
		if( fValue < 0 )
		{
			float	vLimit = m->m_pView1->GetPreferredSize( false ).x;
			vLimit = std::max( m->m_vLimit1, vLimit );

			if( f1.right - f1.left + fValue < vLimit )
			{
				fValue = vLimit - f1.right + f1.left;
				//m->m_Tracking = false;
			}
			m->m_pView1->ResizeBy( Point( (int)fValue, 0 ) );
			m->m_pSeparator->MoveBy( Point( (int)fValue, 0 ) );
			f2.left += (int)fValue;
			m->m_pView2->SetFrame( f2 );
		}
		else if( fValue > 0 )
		{
			float	vLimit = m->m_pView2->GetPreferredSize( false ).x;
			vLimit = std::max( m->m_vLimit2, vLimit );
			
			if( f2.right - f2.left - fValue < vLimit )
			{
				fValue = -vLimit + f2.right - f2.left;
				//m->m_Tracking = false;
			}
			f2.left += (int)fValue;
			m->m_pView2->SetFrame( f2 );
			m->m_pSeparator->MoveBy( Point( (int)fValue, 0 ) );
			m->m_pView1->ResizeBy( Point( (int)fValue, 0 ) );
		}

		float	vLimit1 = std::max( m->m_vLimit1, m->m_pView1->GetPreferredSize( false ).x );
		float	vLimit2 = std::max( m->m_vLimit2, m->m_pView2->GetPreferredSize( false ).x );
		m->m_vSplitPos = (f1.Width() - vLimit1) / (GetBounds().Width() - vLimit1 - vLimit2 - m->m_SeparatorWidth);
	}
	Flush();
}

/** Set the position of the split bar.
* 
* \par Description:   
* Move the separator.
* \par Notes:
* \par Warning:
* This method works with pixel values, normally you should use SetSplitRatio().
* \param  fValue new height(width) of the top(left) view
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SplitTo( float fValue )
{
	Rect f1 = m->m_pView1->GetFrame();

	if( m->m_eOrientation == VERTICAL )
		SplitBy( fValue - f1.bottom + f1.top );
	else
		SplitBy( fValue - f1.right + f1.left );
}



/** Change the direction of the split bar.
*
* \par Description:
* This method keep constant the relative position of the separator.
* \par Notes:
* \par Warning:
* \param eOrientation The new direction of the separator
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SetOrientation( orientation eOrientation )
{
	if( eOrientation != m->m_eOrientation )
	{
		m->m_eOrientation = eOrientation;

		Rect f1 = m->m_pView1->GetFrame();
		Rect f2 = m->m_pView2->GetFrame();

		if( eOrientation == HORIZONTAL )
		{
			f1.right = f1.left + f1.bottom - f1.top;
			f2.right = f2.left + f2.bottom - f2.top;
			m->m_pView1->SetResizeMask( CF_FOLLOW_LEFT | CF_FOLLOW_BOTTOM | CF_FOLLOW_TOP );
			m->m_pSeparator->SetOrientation( VERTICAL );
			m->m_pSeparator->SetResizeMask( CF_FOLLOW_LEFT | CF_FOLLOW_BOTTOM | CF_FOLLOW_TOP );
		}
		else
		{
			f1.bottom = f1.top + f1.right - f1.left;
			f2.bottom = f2.top + f2.right - f2.left;
			m->m_pView1->SetResizeMask( CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
			m->m_pSeparator->SetOrientation( HORIZONTAL );
			m->m_pSeparator->SetResizeMask( CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP );
		}
		m->m_pView1->SetFrame( f1 );
		m->m_pView2->SetFrame( f2 );
		AdjustLayout();
	}
}

/** Get the separator view.
*
* \par Description:
* Return the separator view. This can be usefull for sample if you
* want to place icons on the separator.
* \par Notes:
* \par Warning:
* \param
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
View *Splitter::SeparatorView() const
{
	return m->m_pSeparator;
}

/** Return the separator position.
*
* \par Description:
* Return the separator position 
* \par Notes:
* \par Warning:
* \param
* \return the top(left) view height(width)
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
float Splitter::GetSplitPosition() const
{
	Rect f1 = m->m_pView1->GetFrame();

	if( m->m_eOrientation == VERTICAL )
		return f1.bottom - f1.top;
	else
		return f1.right - f1.left;
}

/** Return the separator width.
*
* \par Description:
* \par Notes:
* \par Warning:
* \param
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
float Splitter::GetSeparatorWidth() const
{
	return m->m_SeparatorWidth;
}

/** Set limits in the position of the split bar
* \par Description:
* By default the split bar can be moved to the boders of the splitter.
* this method allow to limit the range of the split bar moves.
* \par Notes: 
* \par Warning:
* This limits are only used for user interaction. They are never checked
* when you change the layout programaticaly.
* \param nMinSize1 minimal size that can take the top(left) view
* \param nMinSize2 minimal size that can take the bottom(right) view
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SetSplitLimits( float fMinSize1, float fMinSize2 )
{
	m->m_vLimit1 = fMinSize1;
	m->m_vLimit2 = fMinSize2;
}

/** Set the separator width.
*
* \par Description:
* The layout is automatically refreshed.
* \par Notes:
* \par Warning:
* \param
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SetSeparatorWidth( float fWidth )
{
	m->m_SeparatorWidth = fWidth;
	AdjustLayout();
}

/** Hook called when the split bar position has changed.
* 
* \par Description:   
* \par Notes:
* The hook is only called when the split bar was moved by the user, not
* when the object was modified by a call to the Splitter::SplitBy or 
* Splitter::SplitTo methods.
* \par Warning:
* \return
* \sa
* \bug
* \author Sebastien Keim (s.keim@laposte.net)
*/
void Splitter::SplitChanged()
{
}

void Splitter::FrameSized( const Point & cDelta )
{
	AdjustLayout();
	View::FrameSized( cDelta );
}


void Splitter::StartTracking( Point position )
{
	if( !m->m_Tracking )
	{
		if( !m->m_pView1->HasFocus() && !m->m_pView2->HasFocus(  ) )
		{
			m->m_pSeparator->MakeFocus();
		}
		m->m_Tracking = true;
		Rect f = m->m_pSeparator->GetFrame();

		position.x += f.left;
		position.y += f.top;
		m->m_OldPosition = position;
	}
}

void Splitter::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
/*	if( nCode & MOUSE_EXITED )
	{
		m->m_Tracking = false;
	}
	else */
	if( m->m_Tracking )
	{
		float delta;

		if( m->m_eOrientation == VERTICAL )
			delta = cNewPos.y - m->m_OldPosition.y;
		else
			delta = cNewPos.x - m->m_OldPosition.x;
		SplitBy( delta );
		m->m_OldPosition = cNewPos;
		SplitChanged();
	}
	View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void Splitter::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	m->m_Tracking = false;
	View::MouseUp( cPosition, nButtons, pcData );
}


Point Splitter::GetPreferredSize( bool bLargest ) const
{
	Point p1 = m->m_pView1->GetPreferredSize( bLargest );
	Point p2 = m->m_pView2->GetPreferredSize( bLargest );

	if( m->m_eOrientation == VERTICAL )
		return Point( p1.x + p2.x, p1.y + p2.y + m->m_SeparatorWidth );
	else
		return Point( p1.x + p2.x + m->m_SeparatorWidth, p1.y + p2.y );
}


void Splitter::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{

	switch ( pzString[0] )
	{

	case VK_LEFT_ARROW:
	case VK_RIGHT_ARROW:
	case VK_UP_ARROW:
	case VK_DOWN_ARROW:
	case VK_HOME:
	case VK_END:
	case VK_PAGE_UP:
	case VK_PAGE_DOWN:
		if( nQualifiers & QUAL_ALT )
		{
			View *v = m->m_pView1;

			if( m->m_pView1->HasFocus() )
			{
				v = m->m_pView2;
			}
			v->KeyDown( pzString, pzRawString, nQualifiers & ~QUAL_ALT );
			return;
		}
	default:
		break;
	}
	View::KeyDown( pzString, pzRawString, nQualifiers );
}

/** Set split ratio.
*
* \par Description:
* Set the ratio between the two views of the splitter, eg. a ratio of 0.5 will
* make the two views share the size equally and position the splitter bar in
* the middle. Note that min/max sizes still apply, so you can't set the ratio to
* values that would violate those sizes.
* \param vRatio The ratio View1 / (View1 + View2)
* \author Henrik Isaksson (syllable@isaksson.tk)
*/
void Splitter::SetSplitRatio( float vRatio )
{
	m->m_vSplitPos = vRatio;
	AdjustLayout();
	Invalidate();
	Flush();
}

/** Get split ratio.
*
* \par Description:
* Get the split ratio.
* \sa SetSplitRatio()
* \author Henrik Isaksson (syllable@isaksson.tk)
*/
float Splitter::GetSplitRatio() const
{
	return m->m_vSplitPos;
}

void Splitter::__SS_reserved1__()
{
}

void Splitter::__SS_reserved2__()
{
}

void Splitter::__SS_reserved3__()
{
}

void Splitter::__SS_reserved4__()
{
}

void Splitter::__SS_reserved5__()
{
}

