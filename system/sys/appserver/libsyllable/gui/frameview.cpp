/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <gui/frameview.h>
#include <gui/font.h>

using namespace os;


/** FrameView constructor
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


FrameView::FrameView( const Rect & cFrame, const String & cName, const String & cLabel, uint32 nResizeMask, uint32 nFlags ):LayoutView( cFrame, cName, NULL, nResizeMask, nFlags ), m_cLabelString( cLabel )
{
	m_pcLabelView = NULL;
	_CalcStringLabelSize();
}

void FrameView::SetLabel( const String & cLabel )
{
	m_cLabelString = cLabel;
	_CalcStringLabelSize();
	Rect cDamage = GetBounds();

	cDamage.bottom = m_cLabelSize.y;
	Invalidate( cDamage );
	Flush();
}

View *FrameView::SetLabel( View * pcLabel, bool bResizeToPreferred )
{
	View *pcOldLabel = m_pcLabelView;

	m_pcLabelView = pcLabel;
	if( pcOldLabel != NULL )
	{
		RemoveChild( pcOldLabel );
	}
	if( pcLabel != NULL )
	{
		pcLabel->Show( false );
		AddChild( pcLabel );
		if( bResizeToPreferred )
		{
			pcLabel->ResizeTo( pcLabel->GetPreferredSize( false ) );
		}
		pcLabel->MoveTo( 10.0f, 0.0f );
		pcLabel->Show( true );
		m_cLabelSize = pcLabel->GetFrame().Size(  );
	}
	return ( pcOldLabel );
}

String FrameView::GetLabelString() const
{
	return ( m_cLabelString );
}

View *FrameView::GetLabelView() const
{
	return ( m_pcLabelView );
}

void FrameView::AttachedToWindow()
{
	View *pcParent = GetParent();

	if( pcParent != NULL )
	{
		SetEraseColor( pcParent->GetEraseColor() );
	}
}

void FrameView::FrameSized( const Point & cDelta )
{
	Rect cBounds( GetNormalizedBounds() );
	bool bNeedFlush = false;

	LayoutNode *pcRoot = GetRoot();

	if( pcRoot != NULL )
	{
		Rect cClientFrame = cBounds;

		cClientFrame.Resize( 3.0f, m_cLabelSize.y + 1.0f, -3.0f, -3.0f );
		pcRoot->SetFrame( cClientFrame );
	}

	if( cDelta.x != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.left = cDamage.right - std::max( 2.0f, cDelta.x + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}
	if( cDelta.y != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.top = cDamage.bottom - std::max( 2.0f, cDelta.y + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}
	if( bNeedFlush )
	{
		Flush();
	}
}

void FrameView::FontChanged( Font * pcNewFont )
{
	if( m_pcLabelView != NULL )
	{
		return;
	}
	float vOldHeight = m_cLabelSize.y;

	_CalcStringLabelSize();

	Rect cDamage = GetBounds().Bounds(  );

	cDamage.bottom = cDamage.top + std::max( vOldHeight, m_cLabelSize.y ) + 1.0f;

	Invalidate( cDamage );
	Flush();
}

void FrameView::_CalcStringLabelSize()
{
	if( m_cLabelString.size() == 0 )
	{
		return;
	}
	GetFontHeight( &m_sFontHeight );

	m_cLabelSize = GetTextExtent( m_cLabelString );
}

void FrameView::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetNormalizedBounds();

	cBounds.Floor();

	bool bHasStrLabel = m_pcLabelView == NULL && m_cLabelString.size() > 0;
	bool bHasLabel = bHasStrLabel || m_pcLabelView != NULL;

	cBounds.top = m_cLabelSize.y * 0.5f - 1.0f;
	float x1 = 5.0f;
	float x2 = x1 + m_cLabelSize.x + 10.0f;

	Rect cInnerBounds( ( bHasLabel ) ? GetNormalizedBounds() : cBounds );

	cInnerBounds.Resize( 2.0f, ( bHasLabel ) ? 0.0f : 2.0f, -2.0f, -2.0f );
	FillRect( cInnerBounds, GetBgColor() );

	if( bHasLabel )
	{
		FillRect( Rect( 0.0f, 0.0f, 1.0f, cBounds.top - 1.0f ), GetBgColor() );
		FillRect( Rect( cBounds.right - 1.0f, 0.0f, cBounds.right, cBounds.top - 1.0f ), GetBgColor() );
	}

	SetFgColor( 0, 0, 0 );


	if( bHasStrLabel )
	{
		DrawText( Rect( x1 + 5.0f, 0, m_cLabelSize.x, m_cLabelSize.y ), m_cLabelString );
//		DrawString( m_cLabelString.c_str(), Point( x1 + 5.0f, m_sFontHeight.ascender ) );
	}

	Color32_s sFgCol = get_default_color( COL_SHINE );
	Color32_s sBgCol = get_default_color( COL_SHADOW );

	SetFgColor( sBgCol );

	// Left
	DrawLine( Point( cBounds.left, cBounds.bottom ), Point( cBounds.left, cBounds.top ) );
	// Top
	if( bHasLabel )
	{
		DrawLine( Point( cBounds.left, cBounds.top ), Point( x1, cBounds.top ) );
		DrawLine( Point( x2, cBounds.top ), Point( cBounds.right, cBounds.top ) );
	}
	else
	{
		DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
	}
	// Right
	DrawLine( Point( cBounds.right - 1.0f, cBounds.top ), Point( cBounds.right - 1.0f, cBounds.bottom ) );
	// Bottom
	DrawLine( Point( cBounds.left + 2.0f, cBounds.bottom - 1.0f ), Point( cBounds.right - 1.0f, cBounds.bottom - 1.0f ) );


	SetFgColor( sFgCol );
	// Left
	DrawLine( Point( cBounds.left + 1.0f, cBounds.bottom ), Point( cBounds.left + 1.0f, cBounds.top + 1.0f ) );
	// Top
	if( bHasLabel )
	{
		DrawLine( Point( 1.0f, cBounds.top + 1.0f ), Point( x1, cBounds.top + 1.0f ) );
		DrawLine( Point( x2, cBounds.top + 1.0f ), Point( cBounds.right, cBounds.top + 1.0f ) );
	}
	else
	{
		DrawLine( Point( 1.0f, cBounds.top + 1.0f ), Point( cBounds.right, cBounds.top + 1.0f ) );
	}
	// Right
	DrawLine( Point( cBounds.right, cBounds.top + 1.0f ), Point( cBounds.right, cBounds.bottom ) );
	// Bottom
	DrawLine( Point( cBounds.left + 1.0f, cBounds.bottom ), Point( cBounds.right, cBounds.bottom ) );

}

Point FrameView::GetPreferredSize( bool bLargest ) const
{
	Point cSize = LayoutView::GetPreferredSize( bLargest );

	cSize.x += 6.0f;
	cSize.y += m_cLabelSize.y + 1.0f + 3.0f;
	return ( cSize );
}

void FrameView::__FV_reserved1__() {}
void FrameView::__FV_reserved2__() {}
void FrameView::__FV_reserved3__() {}
void FrameView::__FV_reserved4__() {}
void FrameView::__FV_reserved5__() {}
