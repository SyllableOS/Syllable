//  AView (C)opyright 2005 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <gui/guidefines.h>
#include <util/application.h>

#include "iview.h"

IView::IView( Rect cFrame, const char *pzName, Window *pcParent ) : View( cFrame, pzName, CF_FOLLOW_ALL )
{
	Rect cBounds = GetBounds();

	m_pcImageView = new ImageView( Rect( cBounds.left, cBounds.top, cBounds.right - IV_SCROLL_WIDTH, cBounds.bottom - IV_SCROLL_HEIGHT ), "aview_ng_image", NULL, ImageView::DEFAULT, CF_FOLLOW_ALL );
	AddChild( m_pcImageView );

	m_pcHScrollBar = new ScrollBar( Rect( 0, cBounds.Height() - IV_SCROLL_HEIGHT, cBounds.Width() - IV_SCROLL_WIDTH, cBounds.Height() ), "aview_ng_hscroll", new Message( ID_SCROLL_H ), 0, FLT_MAX, HORIZONTAL, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_BOTTOM );
	m_pcHScrollBar->SetScrollTarget( m_pcImageView );
	AddChild( m_pcHScrollBar );

	m_pcVScrollBar = new ScrollBar( Rect( cBounds.Width() - IV_SCROLL_WIDTH, 0, cBounds.Width(), cBounds.Height() - IV_SCROLL_HEIGHT ), "aview_ng_vscroll", new Message( ID_SCROLL_V ), 0, FLT_MAX, VERTICAL, CF_FOLLOW_RIGHT | CF_FOLLOW_TOP | CF_FOLLOW_BOTTOM );
	m_pcVScrollBar->SetScrollTarget( m_pcImageView );
	AddChild( m_pcVScrollBar );

	m_nImageX = m_nImageY = 0;
	m_pcParent = pcParent;
}

IView::~IView( )
{
	RemoveChild( m_pcVScrollBar );
	RemoveChild( m_pcHScrollBar );
	RemoveChild( m_pcImageView );

	delete( m_pcVScrollBar );
	delete( m_pcHScrollBar );
	delete( m_pcImageView );
}

void IView::FrameSized( const Point &cDelta )
{
	m_pcHScrollBar->SetValue( 0 );
	m_pcVScrollBar->SetValue( 0 );

	m_pcHScrollBar->SetMinMax( 0.0f, m_nImageX - ( GetBounds().Width() - IV_SCROLL_WIDTH ) );
	m_pcVScrollBar->SetMinMax( 0.0f, m_nImageY - ( GetBounds().Height() - IV_SCROLL_HEIGHT ) );

	m_pcHScrollBar->SetProportion( ( GetBounds().Width() - IV_SCROLL_WIDTH ) / m_nImageX );
	m_pcVScrollBar->SetProportion( ( GetBounds().Height() - IV_SCROLL_HEIGHT ) / m_nImageY );
}

void IView::WheelMoved( const Point &cDelta )
{
	m_pcVScrollBar->WheelMoved( cDelta );
}

void IView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	// Handle a couple of special keys which don't make sense as Shortcuts
	switch( pzRawString[0] )
	{
		case VK_SPACE:
		case VK_RIGHT_ARROW:
		case VK_UP_ARROW:
		{
			m_pcParent->PostMessage( ID_EVENT_NEXT, m_pcParent );
			break;
		}

		case VK_BACKSPACE:
		case VK_LEFT_ARROW:
		case VK_DOWN_ARROW:
		{
			m_pcParent->PostMessage( ID_EVENT_PREV, m_pcParent );
			break;
		}
	}
}

void IView::WindowActivated( bool bIsActive )
{
	if( bIsActive )
		MakeFocus();
}

void IView::SetImage( Image *pcImage )
{
	Rect cBounds, cFrame;

	cBounds = cFrame = GetBounds();

	m_pcHScrollBar->SetValue( 0 );
	m_pcVScrollBar->SetValue( 0 );

	m_pcImageView->SetImage( pcImage );

	m_nImageX = (int32)pcImage->GetSize().x;
	m_nImageY = (int32)pcImage->GetSize().y;

	// Set the ImageView frame to the correct size.  If the image is smaller than
	// the Window on the X or Y axis use the Image sizes.  If the image is bigger
	// than the window is currently, use the Window bounds.
	if( m_nImageX < cFrame.Width() )
	{
		cFrame.left = 0;
		cFrame.right = m_nImageX;
	}
	else
		cFrame.right -= IV_SCROLL_WIDTH;

	if( m_nImageX < cFrame.Height() )
	{
		cFrame.top = 0;
		cFrame.bottom = m_nImageY;
	}
	else
		cFrame.bottom -= IV_SCROLL_HEIGHT;

	m_pcImageView->SetFrame( cFrame );

	// It would be nice to hide or show the scrollbars depending on the size of the
	// image, but Show() doesn't work properly for Scrollbars.

	// Set the Scrollbars to work with the new Image dimensions
	m_pcHScrollBar->SetMinMax( 0.0f, m_nImageX - ( cBounds.Width() - IV_SCROLL_WIDTH ) );
	m_pcVScrollBar->SetMinMax( 0.0f, m_nImageY - ( cBounds.Height() - IV_SCROLL_HEIGHT ) );

	m_pcHScrollBar->SetProportion( ( cBounds.Width() - IV_SCROLL_WIDTH ) / m_nImageX );
	m_pcVScrollBar->SetProportion( ( cBounds.Height() - IV_SCROLL_HEIGHT ) / m_nImageY );

	m_pcHScrollBar->SetSteps( m_nImageX / 50, m_nImageX / 10 );
	m_pcVScrollBar->SetSteps( m_nImageY / 50, m_nImageY / 10 );
}

