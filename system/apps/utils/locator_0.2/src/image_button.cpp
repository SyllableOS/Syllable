/*
    Locator - A fast file finder for Syllable
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)
                                                                                                                                                                                                        
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                                                                                                                                        
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                                                                                                                                        
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "image_button.h"

ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;
	m_bDrawBorder = bDrawBorder;
}



ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, Bitmap *pcBitmap, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;
	m_bDrawBorder = bDrawBorder;	
	SetImage( pcBitmap );	
}



ImageButton::ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nWidth, uint32 nHeight, uint8 *pnBuffer, uint32 nTextPosition = IB_TEXT_RIGHT,bool bDrawBorder = true, 
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE) : 
				Button( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
	m_nTextPosition = nTextPosition;
	m_pcBitmap = 0;	
	m_bDrawBorder = bDrawBorder;
	SetImage( nWidth, nHeight, pnBuffer );
}


ImageButton::~ImageButton( )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
}


void ImageButton::SetImage( uint32 nWidth, uint32 nHeight, uint8 *pnBuffer )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
		
	uint32 nBufferSize = nWidth * nHeight * 4;
		
	m_pcBitmap = new Bitmap( nWidth, nHeight, CS_RGBA32 );
	
	Color32_s cBGCol = get_default_color( COL_NORMAL );
	
	uint8 *pnRaster = m_pcBitmap->LockRaster( );
	
	for( uint32 nByte = 0; nByte < nBufferSize; nByte+=4 ) {
		if( (pnBuffer[nByte] != 0) || (pnBuffer[nByte + 1] != 0) || (pnBuffer[nByte + 2] != 0) ) {
			(*pnRaster++) = pnBuffer[nByte+2];
			(*pnRaster++) = pnBuffer[nByte+1];
			(*pnRaster++) = pnBuffer[nByte];
			(*pnRaster++) = pnBuffer[nByte+3];
		} else {
			(*pnRaster++) = cBGCol.blue;
			(*pnRaster++) = cBGCol.green;
			(*pnRaster++) = cBGCol.red;
			(*pnRaster++) = cBGCol.alpha;
		}
	
	}
	
	m_pcBitmap->UnlockRaster( );

	Invalidate();
	Flush();	
}


void ImageButton::SetImage( Bitmap *pcBitmap )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;

	m_pcBitmap = pcBitmap;
	
	Invalidate();
	Flush();	
}




bool ImageButton::SetImageFromFile( string zFile )
{
	
	File *pcFile = new File( );
	
	if( pcFile->SetTo( zFile ) != 0 )
		return false;
		
	Translator *pcTrans = NULL;
	TranslatorFactory *pcFactory = new TranslatorFactory();
	pcFactory->LoadAll();
	
	Bitmap *pcBitmap = NULL;
	BitmapFrameHeader sFrameHeader;
	
	uint8 anBuf[8192];
	
	int nBytesRead = 0;
	
	while( true ) {
		
		nBytesRead = pcFile->Read( anBuf, sizeof(anBuf) );
		
		if( nBytesRead == 0 )
			break;
			
		if( pcTrans == NULL ) {
			int nErr = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuf, nBytesRead, &pcTrans );
			if( nErr < 0 )
				return false;
		}
		
		pcTrans->AddData( anBuf, nBytesRead, nBytesRead != sizeof(anBuf) );
	}

	BitmapHeader sBmHeader;
	if( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) )
		return false;
				
	pcBitmap = new Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height(), CS_RGBA32 );
	
	Color32_s cBGCol = get_default_color( COL_NORMAL );
	uint8 anBGPix[4];
	anBGPix[0] = cBGCol.blue;
	anBGPix[1] = cBGCol.green;
	anBGPix[2] = cBGCol.red;	
	anBGPix[3] = cBGCol.alpha;
		
	uint8 *pDstRaster = pcBitmap->LockRaster();
	
	// Why are the first 32 bytes garbage?
	pcTrans->Read(anBuf, 32 );
	
	uint32 nSize = (sBmHeader.bh_bounds.Width() + 1) * sBmHeader.bh_bounds.Height();
	for( uint32 nPix = 0; nPix < nSize; nPix++ ) {
		if( pcTrans->Read( anBuf, 4 ) < 0 )
			break;
				
		memcpy( pDstRaster, (anBuf[3] == 0) ? anBGPix : anBuf, 4 );
	
		pDstRaster += 4;
	}
	
	if( pcBitmap == NULL )
		return false;

	SetImage( pcBitmap );
	return true;
}

			
Point ImageButton::GetPreferredSize( bool bLargest ) const
{
	
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vCharHeight = sHeight.ascender + sHeight.descender + 8;    
	float vStrWidth = GetStringWidth( GetLabel( ) ) + 16;
	
	float x = vStrWidth;
	float y = vCharHeight;

	if( m_pcBitmap != 0 ) {
		Rect cBitmapRect = m_pcBitmap->GetBounds( );
	
		if( vStrWidth > 16 ) {
			if( (m_nTextPosition == IB_TEXT_RIGHT ) || ( m_nTextPosition == IB_TEXT_LEFT ) ) {
				x += cBitmapRect.Width() + 2;
				y = ( (cBitmapRect.Height() + 8) > y ) ? cBitmapRect.Height() + 8 : y;
			} else {
				x = ( (cBitmapRect.Width() + 8) > x ) ? cBitmapRect.Width() + 8 : x;			
				y += cBitmapRect.Height() + 2;
			}
		} else {
			x = cBitmapRect.Width() + 8;
			y = cBitmapRect.Height() + 8;
		}
	}
	
	return Point( x,y );
}

	
void ImageButton::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds( );
	Rect cTextBounds = GetBounds( );
	Rect cBitmapRect;
	
	float vStrWidth = GetStringWidth( GetLabel( ) );
	
    if( m_pcBitmap != 0 ) {
    		
	    cBitmapRect = m_pcBitmap->GetBounds( );
    
		float vBmWidth = cBitmapRect.Width();
		float vBmHeight = cBitmapRect.Height();
	
		switch( m_nTextPosition )
		{
			case IB_TEXT_RIGHT:
				cBitmapRect.left += 4;
				cBitmapRect.right += 4;
				cBitmapRect.top = (cBounds.Height() / 2) - (vBmHeight / 2);
				cBitmapRect.bottom = cBitmapRect.top + vBmHeight;
				cTextBounds.left -= vBmWidth;	
				break;
			case IB_TEXT_LEFT:
				cBitmapRect.top += 4;
				cBitmapRect.bottom += 4;				
				cBitmapRect.left = cBounds.right - 4 - vBmWidth;
				cBitmapRect.right = cBounds.right - 4;
				cTextBounds.right -= vBmWidth;
				break;
			case IB_TEXT_BOTTOM:
				cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
				cBitmapRect.right = cBitmapRect.left + vBmWidth;				
				cBitmapRect.top +=4 ;
				cBitmapRect.bottom += 4;
				cTextBounds.top -= vBmHeight;
				break;				
			case IB_TEXT_TOP:
				cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
				cBitmapRect.right = cBitmapRect.left + vBmWidth;				
				cBitmapRect.top = cBounds.bottom - 4 - vBmHeight;
				cBitmapRect.bottom = cBounds.bottom -4;
				cTextBounds.bottom -= vBmHeight;
				break;
			default:
				break;
		}
		
		if( vStrWidth == 0 ) {
			cBitmapRect.left = (cBounds.Width() / 2) - (vBmWidth / 2);
			cBitmapRect.top = (cBounds.Height() / 2) - (vBmHeight / 2);
			cBitmapRect.right = cBitmapRect.left + vBmWidth;
			cBitmapRect.bottom = cBitmapRect.top + vBmHeight;
		}			
	}
	
    SetEraseColor( get_default_color( COL_NORMAL ) );
    uint32 nFrameFlags = (GetValue().AsBool() && IsEnabled()) ? FRAME_RECESSED : FRAME_RAISED;
	SetBgColor( get_default_color( COL_NORMAL ) );
	if( ( m_bDrawBorder == true ) || (nFrameFlags == FRAME_RECESSED ) ) {
		DrawFrame( cBounds, nFrameFlags );
	} else {
		FillRect( cBounds, get_default_color( COL_NORMAL ) );
		SetFgColor( 0, 255,0 );
		Rect cBounds2 = ConvertToScreen( GetBounds() );
		Rect cWinBounds = GetWindow()->GetFrame();	
	}
	
	font_height sHeight;
	GetFontHeight( &sHeight );
	
    float vCharHeight = sHeight.ascender + sHeight.descender;
    float y = floor( 2.0f + (cTextBounds.Height()+1.0f)*0.5f - vCharHeight*0.5f + sHeight.ascender );
    float x = floor((cTextBounds.Width()+1.0f) / 2.0f - vStrWidth / 2.0f);
		
    if ( IsEnabled() ) {
    	if( GetWindow()->GetDefaultButton() == this ) {
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
    	SetFgColor( 0,0,0 );
		
		if( HasFocus( ) ) 
			DrawLine( Point( (cTextBounds.Width()+1.0f)*0.5f - vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ),
			  Point( (cTextBounds.Width()+1.0f)*0.5f + vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );	

		MovePenTo( x,y );	
		DrawString( GetLabel( ) );
					  
    } else {

		MovePenTo( x,y );	
		DrawString( GetLabel( ) );    	
		MovePenTo( x - 1.0f, y - 1.0f );
    	SetFgColor( 100,100,100 );
    	SetDrawingMode( DM_OVER );
    	DrawString( GetLabel( ) );
    	SetDrawingMode( DM_COPY );
    	
    }
	 
    
    if( m_pcBitmap != 0 )
    	DrawBitmap( m_pcBitmap, m_pcBitmap->GetBounds(), cBitmapRect );  
}
	
	
void ImageButton::SetTextPosition( uint32 nTextPosition )
{
	m_nTextPosition = nTextPosition;
	Invalidate();
	Flush();
}

uint32 ImageButton::GetTextPosition( void )
{
	return m_nTextPosition;
}

Bitmap *ImageButton::GetImage( void )
{
	return m_pcBitmap;
}

void ImageButton::ClearImage( void )
{
	if( m_pcBitmap != 0 )
		delete m_pcBitmap;
		
	m_pcBitmap = 0;

	Invalidate();
	Flush();
}


