
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wndborder.h"
#include "swindow.h"
#include "sfont.h"
#include "server.h"
#include "sprite.h"
#include "windowdecorator.h"
#include "sapplication.h"
#include "inputnode.h"
#include "bitmap.h"

#include <util/locker.h>
#include <util/message.h>
#include <util/messenger.h>
#include <gui/window.h>

using namespace os;


#define H_POINTER_WIDTH  20
#define H_POINTER_HEIGHT 9
static uint8 g_anHMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
};


#define V_POINTER_WIDTH  9
#define V_POINTER_HEIGHT 20
static uint8 g_anVMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00,
	0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
};

#define DIAG_POINTER_WIDTH  16
#define DIAG_POINTER_HEIGHT 16

static uint8 g_anLTMouseImg[] = {
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
};


static uint8 g_anLBMouseImg[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x02, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x03, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline Rect RectToClient( const Rect & cRect, WindowDecorator * pcDecorator )
{
	Rect cBorders = pcDecorator->GetBorderSize();

	return ( Rect( cRect.left + cBorders.left, cRect.top + cBorders.top, cRect.right - cBorders.right, cRect.bottom - cBorders.bottom ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

WndBorder::WndBorder( SrvWindow * pcWindow, Layer * pcParent, const char *pzName, bool bBackdrop )
:Layer( pcWindow, pcParent, pzName, Rect( 0, 0, 0, 0 ), 0, NULL ),
m_cMinSize( 0, 0 ), m_cMaxSize( INT_MAX, INT_MAX ),
m_cAlignSize( 1, 1 ), m_cAlignSizeOff( 0, 0 ),
m_cAlignPos( 1, 1 ), m_cAlignPosOff( 0, 0 )
{
	m_eHitItem = WindowDecorator::HIT_NONE;
	m_eCursorHitItem = WindowDecorator::HIT_NONE;

	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		m_nButtonDown[i] = 0;
	}

	m_bWndMovePending = false;

	m_bBackdrop = bBackdrop;

//    FontNode* pcNode = new FontNode( AppServer::GetInstance()->GetPlainFont() );
//    SetFont( pcNode );
	SetFont( AppServer::GetInstance()->GetWindowTitleFont(  ) );

	m_pcDecorator = NULL;

	m_pcClient = new Layer( pcWindow, this, "wnd_client", Rect( 0, 0, 0, 0 ), 0, NULL );

}

void WndBorder::SetDecorator( os::WindowDecorator * pcDecorator )
{
	SetShapeRegion( NULL );
	
	if( m_pcDecorator )
		delete m_pcDecorator;

	m_pcDecorator = pcDecorator;
}

os::WindowDecorator * WndBorder::GetDecorator() const
{
	return ( m_pcDecorator );
}

void WndBorder::SetFlags( uint32 nFlags )
{
	m_pcDecorator->SetFlags( nFlags );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WndBorder::DoSetFrame( const Rect & cRect )
{
	Layer::SetFrame( cRect );
	m_pcClient->SetFrame( RectToClient( GetBounds(), m_pcDecorator ) );
	m_pcDecorator->FrameSized( GetBounds() );
}

void WndBorder::SetFrame( const Rect & cRect )
{
	m_cRawFrame = cRect;
	DoSetFrame( cRect );
}

void WndBorder::SetSizeLimits( const Point & cMinSize, const Point & cMaxSize )
{
	m_cMinSize = static_cast < IPoint > ( cMinSize );
	m_cMaxSize = static_cast < IPoint > ( cMaxSize );
}

void WndBorder::GetSizeLimits( Point * pcMinSize, Point * pcMaxSize )
{
	*pcMinSize = static_cast < Point > ( m_cMinSize );
	*pcMaxSize = static_cast < Point > ( m_cMaxSize );
}

void WndBorder::SetAlignment( const IPoint & cSize, const IPoint & cSizeOffset, const IPoint & cPos, const IPoint & cPosOffset )
{
	m_cAlignSize = cSize;
	m_cAlignPos = cPos;

	m_cAlignSizeOff.x = cSizeOffset.x % cSize.x;
	m_cAlignSizeOff.y = cSizeOffset.y % cSize.y;

	m_cAlignPosOff.x = cPosOffset.x % cPos.x;
	m_cAlignPosOff.y = cPosOffset.y % cPos.y;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WndBorder::Paint( const IRect & cUpdateRect, bool bUpdate )
{
	uint32 nFlags = m_pcWindow->GetFlags();

	if( nFlags & WND_NO_BORDER )
	{
		return;
	}
	if( bUpdate )
	{
		BeginUpdate();
	}
	m_pcDecorator->Render( static_cast < Rect > ( cUpdateRect ) );
	if( bUpdate )
	{
		EndUpdate();
	}
}

IRect WndBorder::AlignRect( const IRect & cRect, const IRect & cBorders )
{
	IRect cAFrame;

	int nBorderWidth = cBorders.left + cBorders.right;
	int nBorderHeight = cBorders.top + cBorders.bottom;

	cAFrame.left = ( ( cRect.left + cBorders.left ) / m_cAlignPos.x ) * m_cAlignPos.x + m_cAlignPosOff.x - cBorders.left;
	cAFrame.top = ( ( cRect.top + cBorders.top ) / m_cAlignPos.y ) * m_cAlignPos.y + m_cAlignPosOff.y - cBorders.top;

	cAFrame.right = cAFrame.left + ( ( cRect.Width() - nBorderWidth ) / m_cAlignSize.x ) * m_cAlignSize.x + m_cAlignSizeOff.x + nBorderWidth;
	cAFrame.bottom = cAFrame.top + ( ( cRect.Height() - nBorderHeight ) / m_cAlignSize.y ) * m_cAlignSize.y + m_cAlignSizeOff.y + nBorderHeight;
	return ( cAFrame );
}

static int Align( int nVal, int nAlign )
{
	return ( ( nVal / nAlign ) * nAlign );
}

static int Clamp( int nVal, int nDelta, int nMin, int nMax )
{
	if( nVal + nDelta < nMin )
	{
		return ( nMin - nVal );
	}
	else if( nVal + nDelta > nMax )
	{
		return ( nMax - nVal );
	}
	else
	{
		return ( nDelta );
	}
}

IPoint WndBorder::GetHitPointBase() const
{
	switch ( m_eHitItem )
	{
	case WindowDecorator::HIT_SIZE_LEFT:
		return ( IPoint( 0, 0 ) );
		break;
	case WindowDecorator::HIT_SIZE_RIGHT:
		return ( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 ) );
		break;
	case WindowDecorator::HIT_SIZE_TOP:
		return ( IPoint( 0, 0 ) );
		break;
	case WindowDecorator::HIT_SIZE_BOTTOM:
		return ( IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y ) );
		break;
	case WindowDecorator::HIT_SIZE_LT:
		return ( IPoint( 0, 0 ) );
		break;
	case WindowDecorator::HIT_SIZE_RT:
		return ( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 ) );
		break;
	case WindowDecorator::HIT_SIZE_RB:
		return ( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, m_cRawFrame.Height(  ) + m_cDeltaSize.y ) );
		break;
	case WindowDecorator::HIT_SIZE_LB:
		return ( IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y ) );
		break;
	default:
		return ( IPoint( 0, 0 ) );
		break;
	}
}

bool WndBorder::MouseMoved( Messenger * pcAppTarget, const Point & cNewPos, int nTransit )
{
	IPoint cHitPos;

	if( m_eHitItem == WindowDecorator::HIT_NONE && nTransit == MOUSE_EXITED )
	{
		m_eCursorHitItem = WindowDecorator::HIT_NONE;
		SrvApplication::RestoreCursor();
		return ( false );
	}

	// Caclculate the window borders
	IRect cBorders( 0, 0, 0, 0 );
	int nBorderWidth = 0;
	int nBorderHeight = 0;

	if( m_pcDecorator != NULL )
	{
		cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
		nBorderWidth = cBorders.left + cBorders.right;
		nBorderHeight = cBorders.top + cBorders.bottom;
	}

	// Figure out which edges the cursor is relative to
	cHitPos = GetHitPointBase();

	// Calculate the delta movement relative to the hit edge/corner
	IPoint cDelta( static_cast < IPoint > ( cNewPos ) - ( cHitPos + m_cHitOffset ) - m_cDeltaMove + GetILeftTop() - m_cRawFrame.LeftTop(  ) );

	WindowDecorator::hit_item eHitItem;
	if( GetBounds().DoIntersect( cNewPos ) && m_pcClient->m_cFrame.DoIntersect( cNewPos ) == false )
	{
		eHitItem = m_pcDecorator->HitTest( cNewPos );
	}
	else
	{
		eHitItem = WindowDecorator::HIT_NONE;
	}

	if( m_eHitItem == WindowDecorator::HIT_NONE && ( InputNode::GetMouseButtons() == 0 || m_pcWindow->HasFocus(  ) ) )
	{
		// Change the mouse-pointer to indicate that resizeing is possible if above one of the resize-edges
		WindowDecorator::hit_item eCursorHitItem = ( m_eHitItem == WindowDecorator::HIT_NONE ) ? eHitItem : m_eHitItem;
		if( eCursorHitItem != m_eCursorHitItem )
		{
			m_eCursorHitItem = eCursorHitItem;
			switch ( eCursorHitItem )
			{
			case WindowDecorator::HIT_SIZE_LEFT:
			case WindowDecorator::HIT_SIZE_RIGHT:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( H_POINTER_WIDTH / 2, H_POINTER_HEIGHT / 2 ), g_anHMouseImg, H_POINTER_WIDTH, H_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_TOP:
			case WindowDecorator::HIT_SIZE_BOTTOM:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( V_POINTER_WIDTH / 2, V_POINTER_HEIGHT / 2 ), g_anVMouseImg, V_POINTER_WIDTH, V_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_LT:
			case WindowDecorator::HIT_SIZE_RB:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( DIAG_POINTER_WIDTH / 2, DIAG_POINTER_HEIGHT / 2 ), g_anLTMouseImg, DIAG_POINTER_WIDTH, DIAG_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_SIZE_LB:
			case WindowDecorator::HIT_SIZE_RT:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( DIAG_POINTER_WIDTH / 2, DIAG_POINTER_HEIGHT / 2 ), g_anLBMouseImg, DIAG_POINTER_WIDTH, DIAG_POINTER_HEIGHT );
				break;
			case WindowDecorator::HIT_DRAG:
				SrvApplication::SetCursor( MPTR_MONO, IPoint( 0, 0 ), NULL, 0, 0 );
				break;
			default:
				SrvApplication::RestoreCursor();
				break;
			}
		}

		// If we didnt hit anything interesting with the last mouse-click we are done by now.
		if( m_eHitItem == WindowDecorator::HIT_NONE )
		{
			return ( false );
		}
	}

	// Set the state of the various border buttons.
	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		if( m_nButtonDown[i] > 0 )
		{
			int nNewMode = ( eHitItem == i ) ? 1 : 2;

			if( nNewMode != m_nButtonDown[i] )
			{
				m_nButtonDown[i] = nNewMode;
				m_pcDecorator->SetButtonState( i, m_nButtonDown[i] == 1 );
			}
		}
	}

	if( m_eHitItem == WindowDecorator::HIT_DRAG )
	{
		cDelta.x = Align( cDelta.x, m_cAlignPos.x );
		cDelta.y = Align( cDelta.y, m_cAlignPos.y );
		m_cDeltaMove += cDelta;
	}

	uint32 nFlags = m_pcWindow->GetFlags();

	if( ( nFlags & WND_NOT_RESIZABLE ) != WND_NOT_RESIZABLE )
	{
		if( nFlags & WND_NOT_H_RESIZABLE )
		{
			cDelta.x = 0;
		}
		if( nFlags & WND_NOT_V_RESIZABLE )
		{
			cDelta.y = 0;
		}
		IPoint cBorderMinSize( m_pcDecorator->GetMinimumSize() );
		IPoint cMinSize( m_cMinSize );

		if( cMinSize.x < cBorderMinSize.x - nBorderWidth )
		{
			cMinSize.x = cBorderMinSize.x - nBorderWidth;
		}
		if( cMinSize.y < cBorderMinSize.y - nBorderHeight )
		{
			cMinSize.y = cBorderMinSize.y - nBorderHeight;
		}

		switch ( m_eHitItem )
		{
		case WindowDecorator::HIT_SIZE_LEFT:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			m_cDeltaMove.x += cDelta.x;
			m_cDeltaSize.x -= cDelta.x;
			break;
		case WindowDecorator::HIT_SIZE_RIGHT:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			m_cDeltaSize.x += cDelta.x;
			break;
		case WindowDecorator::HIT_SIZE_TOP:
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaMove.y += cDelta.y;
			m_cDeltaSize.y -= cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_BOTTOM:
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaSize.y += cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_LT:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaMove += cDelta;
			m_cDeltaSize -= cDelta;
			break;
		case WindowDecorator::HIT_SIZE_RT:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
			m_cDeltaSize.x += cDelta.x;
			m_cDeltaSize.y -= cDelta.y;
			m_cDeltaMove.y += cDelta.y;
			break;
		case WindowDecorator::HIT_SIZE_RB:
			cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( cDelta.x, m_cAlignSize.x );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaSize += cDelta;
			break;
		case WindowDecorator::HIT_SIZE_LB:
			cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
			cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
			cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
			cDelta.y = Align( cDelta.y, m_cAlignSize.y );
			m_cDeltaMove.x += cDelta.x;
			m_cDeltaSize.x -= cDelta.x;
			m_cDeltaSize.y += cDelta.y;
			break;
		default:
			break;
		}
	}

	if( pcAppTarget != NULL && ( m_cDeltaSize.x != 0 || m_cDeltaSize.y != 0 || m_cDeltaMove.x != 0 || m_cDeltaMove.y != 0 ) )
	{
		m_cRawFrame.right += m_cDeltaSize.x + m_cDeltaMove.x;
		m_cRawFrame.bottom += m_cDeltaSize.y + m_cDeltaMove.y;
		m_cRawFrame.left += m_cDeltaMove.x;
		m_cRawFrame.top += m_cDeltaMove.y;

		IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );


		if( m_bWndMovePending == false )
		{
			Message cMsg( M_WINDOW_FRAME_CHANGED );

			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );

			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "WndBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
			}
			m_bFrameUpdated = false;
//                      m_cClientDeltaSize = IPoint( 0, 0 );
		}
		else
		{
			m_bFrameUpdated = true;
//                      m_cClientDeltaSize += m_cDeltaSize;
		}
		DoSetFrame( cAlignedFrame );
		SrvSprite::Hide();
		m_pcParent->UpdateRegions( false );
		SrvSprite::Unhide();

		m_bWndMovePending = true;

		m_cDeltaMove = IPoint( 0, 0 );
		m_cDeltaSize = IPoint( 0, 0 );
	}

	return ( m_eHitItem != WindowDecorator::HIT_NONE );
}

bool WndBorder::MouseDown( Messenger * pcAppTarget, const Point & cPos, int nButton )
{
	IPoint cHitPos;

	m_cDeltaMove = IPoint( 0, 0 );
	m_cDeltaSize = IPoint( 0, 0 );
	m_cRawFrame = m_cIFrame;

	if( GetBounds().DoIntersect( cPos ) && m_pcClient->m_cFrame.DoIntersect( cPos ) == false )
	{
		m_eHitItem = m_pcDecorator->HitTest( cPos );
	}
	else
	{
		m_eHitItem = WindowDecorator::HIT_NONE;
	}

	cHitPos = GetHitPointBase();

	m_cHitOffset = static_cast < IPoint > ( cPos ) + GetILeftTop() - m_cRawFrame.LeftTop(  ) - cHitPos;

	if( m_eHitItem == WindowDecorator::HIT_DRAG )
	{
		SrvApplication::SetCursor( MPTR_MONO, IPoint( 0, 0 ), NULL, 0, 0 );
	}
	else
	{
		if( m_eHitItem > 0 && m_eHitItem < WindowDecorator::HIT_MAX_ITEMS )
		{
			m_nButtonDown[m_eHitItem] = 1;
			m_pcDecorator->SetButtonState( m_eHitItem, true );
		}
	}

	return ( m_eHitItem != WindowDecorator::HIT_NONE );
}

void WndBorder::MouseUp( Messenger * pcAppTarget, const Point & cPos, int nButton )
{
	if( m_nButtonDown[WindowDecorator::HIT_CLOSE] == 1 )
	{
		if( pcAppTarget != NULL )
		{
			Message cMsg( M_QUIT );

			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "Error: WndBorder::MouseUp() failed to send M_QUIT to window\n" );
			}
		}
	}
	else if( m_nButtonDown[WindowDecorator::HIT_DEPTH] == 1 )
	{
		ToggleDepth();
		if( m_pcParent != NULL )
		{
			m_pcParent->UpdateRegions( false );
		}
		SrvWindow::HandleMouseTransaction();
	}
	else if( m_nButtonDown[WindowDecorator::HIT_MINIMIZE] == 1 )
	{
		if( m_pcWindow != NULL && !m_pcWindow->IsMinimized() )
		{
			m_pcWindow->SetMinimized( true );
			Show( false );
			remove_from_focusstack( m_pcWindow );
		}
		if( m_pcParent != NULL )
		{
			m_pcParent->UpdateRegions( false );
		}
		SrvWindow::HandleMouseTransaction();
	}
	else if( m_nButtonDown[WindowDecorator::HIT_ZOOM] == 1 )
	{
		/* Maximize window */
		uint32 nFlags = m_pcWindow->GetFlags();

		if( ( nFlags & WND_NOT_RESIZABLE ) != WND_NOT_RESIZABLE )
		{

			IRect cBorders( 0, 0, 0, 0 );

			if( m_pcDecorator != NULL )
			{
				cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
			}
			os::Rect cMaxFrame = os::Rect( 0, 0, m_pcBitmap->m_nWidth - 1, m_pcBitmap->m_nHeight - 1 );

			if( nFlags & WND_NOT_H_RESIZABLE )
			{
				cMaxFrame.right = GetFrame().Width(  );
			}
			if( nFlags & WND_NOT_V_RESIZABLE )
			{
				cMaxFrame.bottom = GetFrame().Height(  );
			}

			if( cMaxFrame.right > m_cMaxSize.x )
				cMaxFrame.right = m_cMaxSize.x - 1;
			if( cMaxFrame.bottom > m_cMaxSize.y )
				cMaxFrame.bottom = m_cMaxSize.y - 1;

			IRect cAlignedFrame = AlignRect( cMaxFrame, cBorders );

			if( m_bWndMovePending == false )
			{
				Message cMsg( M_WINDOW_FRAME_CHANGED );

				cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );

				if( pcAppTarget->SendMessage( &cMsg ) < 0 )
				{
					dbprintf( "WndBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
				}
				m_bFrameUpdated = false;
			}
			else
			{
				m_bFrameUpdated = true;
			}
			SetFrame( cAlignedFrame );
			SrvSprite::Hide();
			m_pcParent->UpdateRegions( false );
			SrvSprite::Unhide();

			m_bWndMovePending = true;
		}
	}

	m_eHitItem = WindowDecorator::HIT_NONE;

	for( int i = 0; i < WindowDecorator::HIT_MAX_ITEMS; i++ )
	{
		if( m_nButtonDown[i] != 0 )
		{		// == 1
			m_pcDecorator->SetButtonState( i, false );
		}
		m_nButtonDown[i] = 0;
	}

	m_cRawFrame = m_cIFrame;

	SrvApplication::RestoreCursor();
}

void WndBorder::WndMoveReply( Messenger * pcAppTarget )
{
	g_cLayerGate.Close();
//    if ( m_cDeltaSize.x != 0 || m_cDeltaSize.y != 0 || m_cDeltaMove.x != 0 || m_cDeltaMove.y != 0 )
	if( m_bFrameUpdated )
	{
		m_bFrameUpdated = false;
		if( pcAppTarget != NULL )
		{
			IRect cBorders( 0, 0, 0, 0 );

			if( m_pcDecorator != NULL )
			{
				cBorders = static_cast < IRect > ( m_pcDecorator->GetBorderSize() );
			}

/*	    
	    m_cRawFrame.right  += m_cDeltaSize.x + m_cDeltaMove.x;
	    m_cRawFrame.bottom += m_cDeltaSize.y + m_cDeltaMove.y;
	    m_cRawFrame.left   += m_cDeltaMove.x;
	    m_cRawFrame.top    += m_cDeltaMove.y;
	    */
			IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );

//          DoSetFrame( cAlignedFrame );
//          SrvSprite::Hide();
//          m_pcParent->UpdateRegions( false );
//          SrvSprite::Unhide();

			Message cMsg( M_WINDOW_FRAME_CHANGED );

			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame, m_pcDecorator ) );

			if( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				dbprintf( "Error: WndBorder::WndMoveReply() failed to send M_WINDOW_FRAME_CHANGED to window\n" );
			}

//          if ( m_cClientDeltaSize.x > 0 ) {
//              m_pcClient
//          }

//          m_cClientDeltaSize = IPoint( 0, 0 );

			m_cDeltaMove = IPoint( 0, 0 );
			m_cDeltaSize = IPoint( 0, 0 );
			m_bWndMovePending = true;
		}
	}
	else
	{
		m_bWndMovePending = false;
	}
	if( m_bWndMovePending == false )
	{
		m_pcClient->UpdateIfNeeded( false );
	}
	g_cLayerGate.Open();
}

bool WndBorder::HasPendingSizeEvents() const
{
	return ( m_bWndMovePending );
}
