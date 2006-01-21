/*  libatheos.so - the highlevel API library for Syllable
 *  Copyright (C) 2002 - 2003 Rick Caudill
 *  Copyright (C) 2004 Syllable Team
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
 
/* THIS IS A HACK OF THE LIBSYLLABLE IMAGEBUTTON CLASS WITH SUPPORT FOR MOUSEOVER IMAGES */
 
 
#include "cimagebutton.h"
#include <storage/memfile.h>
#include <typeinfo>
#include <iostream>
using namespace os;

/** \internal */
class CImageButton::Private
{
      public:
	Private()
	{
		m_pcBitmap = NULL;
		m_pcSelectedBitmap = NULL;
		m_pcGrayBitmap = NULL;
		m_nTextPosition = 0;
		bMouseOver = false;
		bShowFrame = false;
		bMouseTwo = false;
	}

	~Private()
	{
		if( m_pcBitmap )
			delete m_pcBitmap;
			
		if( m_pcSelectedBitmap )
			delete m_pcSelectedBitmap;

		if( m_pcGrayBitmap )
			delete m_pcGrayBitmap;
	}

	void SetImage( Image * pcBitmap )
	{
		if( m_pcBitmap )
			delete m_pcBitmap;

		m_pcBitmap = pcBitmap;
	}
	
	void SetSelectedImage( Image * pcBitmap )
	{
		if( m_pcSelectedBitmap )
			delete m_pcSelectedBitmap;

		m_pcSelectedBitmap = pcBitmap;
	}

	Image *GetImage()
	{
		return m_pcBitmap;
	}
	
	Image *GetSelectedImage()
	{
		if( m_pcSelectedBitmap )
			return m_pcSelectedBitmap;
		return m_pcBitmap;
	}

	Image *GetGrayImage()
	{
		if( !m_pcGrayBitmap )
		{
			/* Note: This is not a good method, since it will only work on Bitmaps. */
			/* We have to add some method to Image for copying Images of unknown type! */
			if( m_pcBitmap && ( typeid( *m_pcBitmap ) == typeid( BitmapImage ) ) )
			{
				m_pcGrayBitmap = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcBitmap ) ) );
				m_pcGrayBitmap->ApplyFilter( Image::F_GRAY );
			}
		}
		return m_pcGrayBitmap ? m_pcGrayBitmap : m_pcBitmap;
	}

	Image *m_pcBitmap;
	Image *m_pcSelectedBitmap;
	Image *m_pcGrayBitmap;
	uint32 m_nTextPosition;
	bool bMouseOver;	// true for mouse over mode
	bool bShowFrame;	// true when frames are shown
	bool bMouseTwo;		// true if the mouse is actually over the frame
};

/** Initialize the imagebutton.
 * \par Description:
 *	The imagebutton contructor initialize the local object.
 * \param cFrame - The size and position of the imagebutton.
 * \param pzName - The name of the imagebutton.
 * \param pzLabel - The label of the imagebutton.
 * \param pcMessage - The message passed to the imagebutton.
 * \param pcImage - The bitmapimage for the imagebutton.
 * \param nTextPosition - The Position of the label and the image for the imagebutton.
 * \param bShowFrames - Determines whether the imagebutton will have frames or not.
 * \param bShowText - Determines whether the imagebutton will show the label or not.
 * \param bMouse - Determines whether the imagebutton will have mouseover abilities(this is not working yet).
 * \param nResizeMask - Determines what way the imagebutton will follow the rest of the window.  Default is CF_FOLLOW_LEFT|CF_FOLLOW_TOP.
 * \param nFlags - Deteremines what flags will be sent to the imagebutton.
 * \author	Rick Caudill (cau0730@cup.edu) based on Andrew Keenan's ImageButton class
 *****************************************************************************/

CImageButton::CImageButton( Rect cFrame, const String& cName, const String& cLabel, Message * pcMessage, Image * pcBitmap, uint32 nTextPosition, bool bShowFrames, bool bShowText, bool bMouse, uint32 nResizeMask, uint32 nFlags ):Button( cFrame, cName, cLabel, pcMessage, nResizeMask, nFlags )
{
	m = new Private;

	m->m_nTextPosition = nTextPosition;
	m->bShowFrame = bShowFrames;
	m->bMouseOver = bMouse;

	if( bShowText == true )
		SetLabel( cLabel );
	else
	{
		SetLabel( "" );
	}
	m->SetImage( pcBitmap );

}				/*end ImageButton Constructor */

/** \internal */
CImageButton::~CImageButton()
{
	delete m;
}				/*end ImageButton Destructor */


void CImageButton::Activated( bool bIsActive )
{
	if( ( bIsActive == true ) && ( GetWindow()->IsActive(  ) == true ) )
	{
		Paint( GetBounds() );
	}
}				/*end Activated() */


/** Clears the image...
 * \par Description:
 *	Clears the BitmapImage that is passed to the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void CImageButton::ClearImage( void )
{
	m->SetImage( NULL );
	Invalidate();
	Flush();
}				/*end ClearImage() */

/** Gets the Image...
 * \par Description:
 * Gets the Image that is passed to the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
Image *CImageButton::GetImage( void )
{
	return m->m_pcBitmap;
}				/*end GetImage() */

/* GetPreferredSize()*/
Point CImageButton::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )
	{
		return Point( COORD_MAX, COORD_MAX );
	} else {
		Point cSize;
/*		if( !( GetLabel() == "" ) ) {
			cSize = GetTextExtent( GetLabel() );
			cSize.x += 16;
			cSize.y += 8;
		}*/
		if( m->m_pcBitmap ) {
			Point cImgSize( m->m_pcBitmap->GetSize() );

			if( cSize.x > 0 ) {
				if( ( m->m_nTextPosition == IB_TEXT_RIGHT ) || ( m->m_nTextPosition == IB_TEXT_LEFT ) ) {
					cSize.x += cImgSize.x + 2;
					cSize.y = std::max( cSize.y, cImgSize.y );
				} else {
					cSize.y += cImgSize.y + 2;
					cSize.x = std::max( cSize.x, cImgSize.x );
				}
			} else {
				cSize.x = cImgSize.x + 4;
				cSize.y = cImgSize.y + 4;
			}
		}
		return cSize;
	}
/*
	font_height sHeight;

	GetFontHeight( &sHeight );
	float vCharHeight = sHeight.ascender + sHeight.descender + 8;
	float vStrWidth = GetStringWidth( GetLabel() ) + 16;

	float x = vStrWidth;
	float y = vCharHeight;

	if( m->m_pcBitmap )
	{
		Rect cBitmapRect = m->m_pcBitmap->GetBounds();

		if( vStrWidth > 16 )
		{
			if( ( m->m_nTextPosition == IB_TEXT_RIGHT ) || ( m->m_nTextPosition == IB_TEXT_LEFT ) )
			{
				x += cBitmapRect.Width() + 2;
				y = ( ( cBitmapRect.Height() + 8 ) > y ) ? cBitmapRect.Height(  ) + 8 : y;
			}
			else
			{
				x = ( ( cBitmapRect.Width() + 8 ) > x ) ? cBitmapRect.Width(  ) + 8 : x;
				y += cBitmapRect.Height() + 2;
			}
		}
		else
		{
			x = cBitmapRect.Width() + 8;
			y = cBitmapRect.Height() + 8;
		}
	}

	return Point( x, y );*/
}				/*end GetPreferredSize() */

/** Gets the text postion...
 * \par Description:
 *	Gets the text positon of the ImageButton.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
uint32 CImageButton::GetTextPosition( void )
{
	return m->m_nTextPosition;
}				/*end GetTextPosition() */

/* MouseDown*/
void CImageButton::MouseDown( const Point & cPosition, uint32 nButton )
{
	Button::MouseDown( cPosition, nButton );
}				/*end MouseDown() */

/* MouseMove */
void CImageButton::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	Button::MouseMove( cNewPos, nCode, nButtons, pcData );

	if( m->bMouseOver )
	{
		bool mouseover = GetBounds().DoIntersect( cNewPos );

		if( mouseover != m->bMouseTwo )
		{
			m->bMouseTwo = mouseover;
			Invalidate();
			Flush();
		}
	}

	return;
}				/*end MouseMove() */

/*MouseUp*/
void CImageButton::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	Button::MouseUp( cPosition, nButton, pcData );
}				/*ehd MouseUp() */

void CImageButton::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();
	Rect cTextBounds = GetBounds();
	float vStrWidth = GetStringWidth( GetLabel() );
	Point cBitPoint;
	font_height sHeight;

	GetFontHeight( &sHeight );
	float vCharHeight = sHeight.ascender + sHeight.descender;
	Image *pcImg = IsEnabled()? m->GetImage(  ) : m->GetGrayImage(  );
	
	if( m->bMouseOver == true && m->bMouseTwo == true && IsEnabled() )
		pcImg = m->GetSelectedImage();

	if( pcImg )
	{

		switch ( m->m_nTextPosition )
		{
		case IB_TEXT_RIGHT:
			cTextBounds.left -= pcImg->GetSize().x - 2;
			cBitPoint.x = 4;
			cBitPoint.y = GetBounds().top + 3;
			break;

		case IB_TEXT_LEFT:
			cTextBounds.left = 0;
			cTextBounds.right = vStrWidth;
			cBitPoint.x = cTextBounds.right + 2;
			cBitPoint.y = GetBounds().top + 3;
			break;

		case IB_TEXT_BOTTOM:
			cTextBounds.top -= pcImg->GetSize().y;
			cBitPoint.x = ( GetBounds().Width(  ) - pcImg->GetSize(  ).x ) / 2;
			cBitPoint.y = 4;
			break;

		case IB_TEXT_TOP:
			cTextBounds.bottom = vCharHeight;
			cTextBounds.left = 0;
			cTextBounds.top = 0;
			cBitPoint.y = cTextBounds.bottom + 2;
			cBitPoint.x = ( GetBounds().Width(  ) / 2 - pcImg->GetSize(  ).x / 2 );
			break;
		default:
			break;
		}

	}

	SetEraseColor( get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetFgColor( get_default_color( COL_NORMAL ) );

	Rect border = GetBounds();

	if( m->bShowFrame == false )
	{
		
		if( m->bMouseOver == true && m->bMouseTwo == true )
		{
			os::Rect cFillRect = GetBounds();			
			SetFgColor( 230, 230, 230 );
			cFillRect.bottom = 3;
			
			FillRect( cBounds, get_default_color( COL_NORMAL ) );
			cFillRect = GetBounds();
			cFillRect.Resize( 1, 4, -1 , -4 );
			FillRect( cFillRect );
			
			SetFgColor( 180, 180, 180 );
			DrawLine( os::Point( 0, cFillRect.top ), os::Point( 0, cFillRect.bottom ) );
			DrawLine( os::Point( cFillRect.right + 1, cFillRect.top ), os::Point( cFillRect.right + 1, cFillRect.bottom ) );
			DrawLine( os::Point( 4, 0 ), os::Point( GetBounds().right - 4, 0 ) );
			DrawLine( os::Point( 2, 1 ), os::Point(	3, 1 ) );
			DrawLine( os::Point( 1, 2 ), os::Point(	1, 3 ) );
			DrawLine( os::Point( GetBounds().right - 2, 1 ), os::Point(	GetBounds().right - 3, 1 ) );
			DrawLine( os::Point( GetBounds().right - 1, 2 ), os::Point(	GetBounds().right - 1, 3 ) );
			SetFgColor( 230, 230, 230 );
			DrawLine( os::Point( 4, 1 ), os::Point( GetBounds().right - 4, 1 ) );
			DrawLine( os::Point( 2, 2 ), os::Point( GetBounds().right - 2, 2 ) );
			DrawLine( os::Point( 2, 3 ), os::Point( GetBounds().right - 2, 3 ) );
	
			cFillRect.top = GetBounds().bottom - 3;
			cFillRect.bottom = GetBounds().bottom;
	
			FillRect( cFillRect, os::get_default_color( os::COL_NORMAL ) );
			SetFgColor( 180, 180, 180 );
			DrawLine( os::Point( 4, cFillRect.bottom - 0 ), os::Point( GetBounds().right - 4, cFillRect.bottom - 0 ) );
			DrawLine( os::Point( 2, cFillRect.bottom - 1 ), os::Point( 3, cFillRect.bottom - 1 ) );
			DrawLine( os::Point( 1, cFillRect.bottom - 2 ), os::Point( 1, cFillRect.bottom - 3 ) );
			DrawLine( os::Point( GetBounds().right - 2, cFillRect.bottom - 1 ), os::Point(	GetBounds().right - 3, cFillRect.bottom - 1 ) );
			DrawLine( os::Point( GetBounds().right - 1, cFillRect.bottom - 2 ), os::Point(	GetBounds().right - 1, cFillRect.bottom - 3 ) );
			DrawLine( os::Point( 4, cFillRect.bottom ), os::Point( GetBounds().right - 4, cFillRect.bottom ) );
			SetFgColor( 230, 230, 230 );
			DrawLine( os::Point( 4, cFillRect.bottom - 1 ), os::Point( GetBounds().right - 4, cFillRect.bottom - 1 ) );
			DrawLine( os::Point( 2, cFillRect.bottom - 2 ), os::Point( GetBounds().right - 2, cFillRect.bottom - 2 ) );
			DrawLine( os::Point( 2, cFillRect.bottom - 3 ), os::Point( GetBounds().right - 2, cFillRect.bottom - 3 ) );

		}
		else
		{
			FillRect( cBounds, get_default_color( COL_NORMAL ) );
			SetFgColor( 0, 255, 0 );
			Rect cBounds2 = ConvertToScreen( GetBounds() );
			Rect cWinBounds = GetWindow()->GetFrame(  );
		}
	}
	else
	{
		if( m->bMouseOver == false || ( m->bMouseTwo == true && IsEnabled() ) )
		{
			uint32 nFrameFlags = ( GetValue().AsBool(  ) && IsEnabled(  ) )? FRAME_RECESSED : FRAME_RAISED;

			DrawFrame( cBounds, nFrameFlags );
		}
		else
		{
			FillRect( cBounds, get_default_color( COL_NORMAL ) );
		}
	}

	float y = floor( 2.0f + ( cTextBounds.Height() + 1.0f ) * 0.5f - vCharHeight * 0.5f + sHeight.ascender );
	float x = floor( ( cTextBounds.Width() + 1.0f ) / 2.0f - vStrWidth / 2.0f );

	if( GetWindow()->GetDefaultButton(  ) == this && IsEnabled(  ) )
	{
		SetFgColor( 0, 0, 0 );
		DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
		DrawLine( Point( cBounds.right, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.top ) );
		cBounds.left += 1;
		cBounds.top += 1;
		cBounds.right -= 1;
		cBounds.bottom -= 1;
	}

	if( !GetLabel().empty() )
	{
		if( IsEnabled() )
		{
			if( HasFocus() )
			{
				SetFgColor( 0, 0, 0 );
				DrawLine( Point( ( cTextBounds.Width() + 1.0f ) * 0.5f - vStrWidth * 0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ), Point( ( cTextBounds.Width(  ) + 1.0f ) * 0.5f + vStrWidth * 0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );
			}

			SetFgColor( 0, 0, 0 );
			MovePenTo( x, y );
			DrawString( GetLabel() );
		}
		else
		{
			MovePenTo( x, y );
			DrawString( GetLabel() );
			MovePenTo( x - 1.0f, y - 1.0f );
			SetFgColor( 100, 100, 100 );
			SetDrawingMode( DM_OVER );
			DrawString( GetLabel() );
			SetDrawingMode( DM_COPY );
		}
	}

	if( pcImg )
	{
		SetDrawingMode( DM_BLEND );	//DM_BLEND
		pcImg->Draw( cBitPoint, this );
		SetDrawingMode( DM_OVER );	//sets the mode to dm_over, so that hovering works completely
	}
}				/*end Paint() */

/** Resests the text postion...
 * \par Description:
 *	Resests the text postion and then flushes the imagebutton.
 * \param nTextPosition - int that holds the text position.  Should be: ID_TEXT_BOTTOM,
 	ID_TEXT_RIGHT, ID_TEXT_TOP, or ID_TEXT_LEFT.
 * \author	Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void CImageButton::SetTextPosition( uint32 nTextPosition )
{
	m->m_nTextPosition = nTextPosition;
	Invalidate();
	Flush();
}				/*end SetTextPosition() */

/**Load an image from a stream.
 * \par Description:
 *	Reads the BitmapImage for the ImageButton from a data stream.
 * \param pcStream - A StreamableIO data source for the Bitmap data.
 * \author	Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/
void CImageButton::SetImage( StreamableIO* pcStream )
{
	Image *vImage = new BitmapImage( pcStream );
	m->SetImage( vImage );
}


/**Load an image from a stream.
 * \par Description:
 *	Reads the BitmapImage for the ImageButton from a data stream.
 * \param pcStream - A StreamableIO data source for the Bitmap data.
 * \author	Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/
void CImageButton::SetSelectedImage( StreamableIO* pcStream )
{
	Image *vImage = new BitmapImage( pcStream );
	m->SetSelectedImage( vImage );
}

/** Copy an ImageButton
 * \par Description:
 * Copy the Image from one ImageButton to another
 * \param cImageButton - The ImageButton to copy from.
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/
CImageButton& CImageButton::operator=( CImageButton& cImageButton )
{
	m->SetImage( cImageButton.GetImage() );
	m->m_nTextPosition = cImageButton.m->m_nTextPosition;
	SetLabel( cImageButton.GetLabel() );
	return( *this );
}

/** Sets the Image from another image...
 * \par Description:
 * Sets the Image for the ImageButton from another Image.
 * \param pcImage - The image.
 * \author  Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void CImageButton::SetImage( Image * pcImage )
{
	if( pcImage )
		m->SetImage( pcImage );
}



/** Sets the Image from another image...
 * \par Description:
 * Sets the Image for the ImageButton from another Image.
 * \param pcImage - The image.
 * \author  Rick Caudill (cau0730@cup.edu)
 *****************************************************************************/
void CImageButton::SetSelectedImage( Image * pcImage )
{
	if( pcImage )
		m->SetSelectedImage( pcImage );
}
