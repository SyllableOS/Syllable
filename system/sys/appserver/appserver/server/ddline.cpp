
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include "ddriver.h"
#include "bitmap.h"

#include <gui/bitmap.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DisplayDriver::ClipLine( const IRect & cRect, int *x1, int *y1, int *x2, int *y2 )
{
	bool point_1 = false;
	bool point_2 = false;	// tracks if each end point is visible or invisible

	bool clip_always = false;	// used for clipping override

	int xi = 0, yi = 0;	// point of intersection

	bool right_edge = false;	// which edges are the endpoints beyond
	bool left_edge = false;
	bool top_edge = false;
	bool bottom_edge = false;


	bool success = false;	// was there a successfull clipping

	float dx, dy;		// used to holds slope deltas

// test if line is completely visible

	if( ( *x1 >= cRect.left ) && ( *x1 <= cRect.right ) && ( *y1 >= cRect.top ) && ( *y1 <= cRect.bottom ) )
	{
		point_1 = true;
	}

	if( ( *x2 >= cRect.left ) && ( *x2 <= cRect.right ) && ( *y2 >= cRect.top ) && ( *y2 <= cRect.bottom ) )
	{
		point_2 = true;
	}


	// test endpoints
	if( point_1 && point_2 )
	{
		return ( true );
	}

	// test if line is completely invisible
	if( point_1 == false && point_2 == false )
	{
		// must test to see if each endpoint is on the same side of one of
		// the bounding planes created by each clipping region boundary

		if( ( ( *x1 < cRect.left ) && ( *x2 < cRect.left ) ) ||	// to the left
			( ( *x1 > cRect.right ) && ( *x2 > cRect.right ) ) ||	// to the right
			( ( *y1 < cRect.top ) && ( *y2 < cRect.top ) ) ||	// above
			( ( *y1 > cRect.bottom ) && ( *y2 > cRect.bottom ) ) )
		{		// below
			return ( 0 );	// the entire line is otside the rectangle
		}

		// if we got here we have the special case where the line cuts into and
		// out of the clipping region
		clip_always = true;
	}

	// take care of case where either endpoint is in clipping region
	if( point_1 || clip_always )
	{
		dx = *x2 - *x1;	// compute deltas
		dy = *y2 - *y1;

		// compute what boundary line need to be clipped against
		if( *x2 > cRect.right )
		{
			// flag right edge
			right_edge = true;

			// compute intersection with right edge
			if( dx != 0 )
				yi = ( int )( .5 + ( dy / dx ) * ( cRect.right - *x1 ) + *y1 );
			else
				yi = -1;	// invalidate intersection
		}
		else if( *x2 < cRect.left )
		{
			// flag left edge
			left_edge = true;

			// compute intersection with left edge
			if( dx != 0 )
			{
				yi = ( int )( .5 + ( dy / dx ) * ( cRect.left - *x1 ) + *y1 );
			}
			else
			{
				yi = -1;	// invalidate intersection
			}
		}

		// horizontal intersections
		if( *y2 > cRect.bottom )
		{
			bottom_edge = true;	// flag bottom edge

			// compute intersection with right edge
			if( dy != 0 )
			{
				xi = ( int )( .5 + ( dx / dy ) * ( cRect.bottom - *y1 ) + *x1 );
			}
			else
			{
				xi = -1;	// invalidate inntersection
			}
		}
		else if( *y2 < cRect.top )
		{
			// flag top edge
			top_edge = true;

			// compute intersection with top edge
			if( dy != 0 )
			{
				xi = ( int )( .5 + ( dx / dy ) * ( cRect.top - *y1 ) + *x1 );
			}
			else
			{
				xi = -1;	// invalidate intersection
			}
		}

		// now we know where the line passed thru
		// compute which edge is the proper intersection

		if( right_edge == true && ( yi >= cRect.top && yi <= cRect.bottom ) )
		{
			*x2 = cRect.right;
			*y2 = yi;
			success = true;
		}
		else if( left_edge && ( yi >= cRect.top && yi <= cRect.bottom ) )
		{
			*x2 = cRect.left;
			*y2 = yi;

			success = true;
		}

		if( bottom_edge == true && ( xi >= cRect.left && xi <= cRect.right ) )
		{
			*x2 = xi;
			*y2 = cRect.bottom;
			success = true;
		}
		else if( top_edge && ( xi >= cRect.left && xi <= cRect.right ) )
		{
			*x2 = xi;
			*y2 = cRect.top;
			success = true;
		}
	}			// end if point_1 is visible

	// reset edge flags
	right_edge = left_edge = top_edge = bottom_edge = false;

	// test second endpoint
	if( point_2 || clip_always )
	{
		dx = *x1 - *x2;	// compute deltas
		dy = *y1 - *y2;

		// compute what boundary line need to be clipped against
		if( *x1 > cRect.right )
		{

			// flag right edge
			right_edge = true;

			// compute intersection with right edge
			if( dx != 0 )
			{
				yi = ( int )( .5 + ( dy / dx ) * ( cRect.right - *x2 ) + *y2 );
			}
			else
			{
				yi = -1;	// invalidate inntersection
			}
		}
		else if( *x1 < cRect.left )
		{
			left_edge = true;	// flag left edge

			// compute intersection with left edge
			if( dx != 0 )
			{
				yi = ( int )( .5 + ( dy / dx ) * ( cRect.left - *x2 ) + *y2 );
			}
			else
			{
				yi = -1;	// invalidate intersection
			}
		}

		// horizontal intersections
		if( *y1 > cRect.bottom )
		{
			bottom_edge = true;	// flag bottom edge

			// compute intersection with right edge
			if( dy != 0 )
			{
				xi = ( int )( .5 + ( dx / dy ) * ( cRect.bottom - *y2 ) + *x2 );
			}
			else
			{
				xi = -1;	// invalidate inntersection
			}
		}
		else if( *y1 < cRect.top )
		{
			top_edge = true;	// flag top edge

			// compute intersection with top edge
			if( dy != 0 )
			{
				xi = ( int )( .5 + ( dx / dy ) * ( cRect.top - *y2 ) + *x2 );
			}
			else
			{
				xi = -1;	// invalidate inntersection
			}
		}

		// now we know where the line passed thru
		// compute which edge is the proper intersection
		if( right_edge && ( yi >= cRect.top && yi <= cRect.bottom ) )
		{
			*x1 = cRect.right;
			*y1 = yi;
			success = true;
		}
		else if( left_edge && ( yi >= cRect.top && yi <= cRect.bottom ) )
		{
			*x1 = cRect.left;
			*y1 = yi;
			success = true;
		}

		if( bottom_edge && ( xi >= cRect.left && xi <= cRect.right ) )
		{
			*x1 = xi;
			*y1 = cRect.bottom;
			success = true;
		}
		else if( top_edge == 1 && ( xi >= cRect.left && xi <= cRect.right ) )
		{
			*x1 = xi;
			*y1 = cRect.top;

			success = true;
		}
	}			// end if point_2 is visible

	return ( success );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::DrawLine16( SrvBitmap * pcBitmap, const IRect & cClip, int x1, int y1, int x2, int y2, uint16 nColor, uint32 nColor32, int nMode )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint16 *pRaster;
	int nModulo = pcBitmap->m_nBytesPerLine;

	if( DisplayDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false )
	{
		return;
	}

/*	if( ( ( x1 == x2 ) || ( y1 == y2 ) ) && nMode != DM_INVERT )
	{
		FillRect( pcBitmap, IRect( x1, y1, x2, y2 ), os::Color32_s( nColor32 ), nMode );
		return;
	}*/
		
	LockBitmap( pcBitmap, NULL, IRect(), cClip );
	
	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = ( nODeltaY - nODeltaX ) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs( x1 - ox1 );
			int nClipDeltaY = abs( y1 - oy1 );

			d += ( ( nClipDeltaY * dinc2 ) + ( ( nClipDeltaX - nClipDeltaY ) * dinc1 ) );
		}

		if( y1 > y2 )
		{
			nYStep = -nModulo;
		}
		else
		{
			nYStep = nModulo;
		}
		if( x1 > x2 )
		{
			nYStep = -nYStep;
			pRaster = ( uint16 * )( pcBitmap->m_pRaster + x2 * 2 + nModulo * y2 );
		}
		else
		{
			pRaster = ( uint16 * )( pcBitmap->m_pRaster + x1 * 2 + nModulo * y1 );
		}

		for( int i = 0; i <= nDeltaX; ++i )
		{
			if( nMode == DM_COPY )
			{
				*pRaster = nColor;
			}
			else if( nMode == DM_INVERT )
			{
				Color32_s sColor = RGB16_TO_COL( *pRaster );

				sColor.red = 255 - sColor.red;
				sColor.green = 255 - sColor.green;
				sColor.blue = 255 - sColor.blue;
				*pRaster = COL_TO_RGB16( sColor );
			}
			else if( nMode == DM_BLEND )
			{
				uint32 nSrcColor = nColor32;
				unsigned nAlpha = nSrcColor >> 27;
					
				if( nAlpha == ( 0xff >> 3 ) ) {
					*pRaster = ( nSrcColor >> 8 & 0xf800 ) + ( nSrcColor >> 5 & 0x7e0 ) 
						+ ( nSrcColor >> 3  & 0x1f );
				} else if( nAlpha != 0x00 ) {
					uint32 nDstColor = *pRaster;
					nSrcColor = ( ( nSrcColor & 0xfc00 ) << 11 ) + ( nSrcColor >> 8 & 0xf800 )
						      + ( nSrcColor >> 3 & 0x1f );
					nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
					nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
					nDstColor &= 0x07e0f81f;
					*pRaster = nDstColor | nDstColor >> 16;
				}		
			}
			if( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = ( uint16 * )( ( ( uint8 * )pRaster ) + nYStep );
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = ( nODeltaX - nODeltaY ) << 1;
		int nXStep;

		if( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs( x1 - ox1 );
			int nClipDeltaY = abs( y1 - oy1 );

			d += ( ( nClipDeltaX * dinc2 ) + ( ( nClipDeltaY - nClipDeltaX ) * dinc1 ) );
		}


		if( x1 > x2 )
		{
			nXStep = -sizeof( uint16 );
		}
		else
		{
			nXStep = sizeof( uint16 );
		}
		if( y1 > y2 )
		{
			nXStep = -nXStep;
			pRaster = ( uint16 * )( pcBitmap->m_pRaster + x2 * 2 + nModulo * y2 );
		}
		else
		{
			pRaster = ( uint16 * )( pcBitmap->m_pRaster + x1 * 2 + nModulo * y1 );
		}

		for( int i = 0; i <= nDeltaY; ++i )
		{
			if( nMode == DM_COPY )
			{
				*pRaster = nColor;
			}
			else if( nMode == DM_INVERT )
			{
				Color32_s sColor = RGB16_TO_COL( *pRaster );

				sColor.red = 255 - sColor.red;
				sColor.green = 255 - sColor.green;
				sColor.blue = 255 - sColor.blue;
				*pRaster = COL_TO_RGB16( sColor );
			}
			else if( nMode == DM_BLEND )
			{
				uint32 nSrcColor = nColor32;
				unsigned nAlpha = nSrcColor >> 27;
					
				if( nAlpha == ( 0xff >> 3 ) ) {
					*pRaster = ( nSrcColor >> 8 & 0xf800 ) + ( nSrcColor >> 5 & 0x7e0 ) 
						+ ( nSrcColor >> 3  & 0x1f );
				} else if( nAlpha != 0x00 ) {
					uint32 nDstColor = *pRaster;
					nSrcColor = ( ( nSrcColor & 0xfc00 ) << 11 ) + ( nSrcColor >> 8 & 0xf800 )
						      + ( nSrcColor >> 3 & 0x1f );
					nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
					nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
					nDstColor &= 0x07e0f81f;
					*pRaster = nDstColor | nDstColor >> 16;
				}		
			}
			if( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = ( uint16 * )( ( ( uint8 * )pRaster ) + nXStep );
			}
			pRaster = ( uint16 * )( ( ( uint8 * )pRaster ) + nModulo );
		}
	}
	UnlockBitmap( pcBitmap, NULL, IRect(), cClip );
}

void DisplayDriver::DrawLine32( SrvBitmap * pcBitmap, const IRect & cClip, int x1, int y1, int x2, int y2, uint32 nColor, int nMode )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint32 *pRaster;
	int nModulo = pcBitmap->m_nBytesPerLine;

	if( DisplayDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false )
	{
		return;
	}
	#if 0
	if( ( ( x1 == x2 ) || ( y1 == y2 ) ) && nMode != DM_INVERT )
	{
		FillRect( pcBitmap, IRect( x1, y1, x2, y2 ), os::Color32_s( nColor ), nMode );
		return;
	}
	#endif
	
	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );
	
	LockBitmap( pcBitmap, NULL, IRect(), cClip );

	if( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = ( nODeltaY - nODeltaX ) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs( x1 - ox1 );
			int nClipDeltaY = abs( y1 - oy1 );

			d += ( ( nClipDeltaY * dinc2 ) + ( ( nClipDeltaX - nClipDeltaY ) * dinc1 ) );
		}

		if( y1 > y2 )
		{
			nYStep = -nModulo;
		}
		else
		{
			nYStep = nModulo;
		}
		if( x1 > x2 )
		{
			nYStep = -nYStep;
			pRaster = ( uint32 * )( pcBitmap->m_pRaster + x2 * 4 + nModulo * y2 );
		}
		else
		{
			pRaster = ( uint32 * )( pcBitmap->m_pRaster + x1 * 4 + nModulo * y1 );
		}

		for( int i = 0; i <= nDeltaX; ++i )
		{
			if( nMode == DM_COPY )
			{
				*pRaster = nColor;
			}
			else if( nMode == DM_INVERT )
			{
				Color32_s sColor = RGB32_TO_COL( *pRaster );

				sColor.red = 255 - sColor.red;
				sColor.green = 255 - sColor.green;
				sColor.blue = 255 - sColor.blue;
				*pRaster = COL_TO_RGB32( sColor );
			} 
			else if( nMode == DM_BLEND )
			{
				uint32 nSrcColor = nColor;
				uint32 nAlpha = nSrcColor >> 24;
			
				if( nAlpha == 0xff )
				{
					*pRaster = nSrcColor;
				} 
				else if( nAlpha != 0x00 )
				{
					uint32 nDstColor = *pRaster;
					uint32 nSrc1;
					uint32 nDst1;
					uint32 nDstAlpha = nDstColor & 0xff000000;

					nSrc1 = nSrcColor & 0xff00ff;
					nDst1 = nDstColor & 0xff00ff;
					nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
					nSrcColor &= 0xff00;
					nDstColor &= 0xff00;
					nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8)) & 0xff00;
					*pRaster = nDst1 | nDstColor | nDstAlpha;
				}
			}
			if( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = ( uint32 * )( ( ( uint8 * )pRaster ) + nYStep );
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = ( nODeltaX - nODeltaY ) << 1;
		int nXStep;

		if( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs( x1 - ox1 );
			int nClipDeltaY = abs( y1 - oy1 );

			d += ( ( nClipDeltaX * dinc2 ) + ( ( nClipDeltaY - nClipDeltaX ) * dinc1 ) );
		}


		if( x1 > x2 )
		{
			nXStep = -sizeof( uint32 );
		}
		else
		{
			nXStep = sizeof( uint32 );
		}
		if( y1 > y2 )
		{
			nXStep = -nXStep;
			pRaster = ( uint32 * )( pcBitmap->m_pRaster + x2 * 4 + nModulo * y2 );
		}
		else
		{
			pRaster = ( uint32 * )( pcBitmap->m_pRaster + x1 * 4 + nModulo * y1 );
		}

		for( int i = 0; i <= nDeltaY; ++i )
		{
			if( nMode == DM_COPY )
			{
				*pRaster = nColor;
			}
			else if( nMode == DM_INVERT )
			{
				Color32_s sColor = RGB32_TO_COL( *pRaster );

				sColor.red = 255 - sColor.red;
				sColor.green = 255 - sColor.green;
				sColor.blue = 255 - sColor.blue;
				*pRaster = COL_TO_RGB32( sColor );
			} 
			else if( nMode == DM_BLEND )
			{
				uint32 nSrcColor = nColor;
				uint32 nAlpha = nSrcColor >> 24;
			
				if( nAlpha == 0xff )
				{
					*pRaster = nSrcColor;
				} 
				else if( nAlpha != 0x00 )
				{
					uint32 nDstColor = *pRaster;
					uint32 nSrc1;
					uint32 nDst1;
					uint32 nDstAlpha = nDstColor & 0xff000000;

					nSrc1 = nSrcColor & 0xff00ff;
					nDst1 = nDstColor & 0xff00ff;
					nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
					nSrcColor &= 0xff00;
					nDstColor &= 0xff00;
					nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8)) & 0xff00;
					*pRaster = nDst1 | nDstColor | nDstAlpha;
				}
			}
			if( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = ( uint32 * )( ( ( uint8 * )pRaster ) + nXStep );
			}
			pRaster = ( uint32 * )( ( ( uint8 * )pRaster ) + nModulo );
		}
	}
	UnlockBitmap( pcBitmap, NULL, IRect(), cClip );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DisplayDriver::DrawLine( SrvBitmap * psBitmap, const IRect & cClipRect, const IPoint & cPnt1, const IPoint & cPnt2, const Color32_s & sColor, int nMode )
{
	switch ( nMode )
	{
	case DM_COPY:
	case DM_OVER:
	default:
		
		switch ( psBitmap->m_eColorSpc )
		{
		case CS_RGB16:
			DrawLine16( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB16( sColor ), sColor, DM_COPY );
			break;
		case CS_RGB32:
		case CS_RGBA32:
			DrawLine32( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB32( sColor ), DM_COPY );
			break;
		default:
			dbprintf( "DisplayDriver::DrawLine() unknown colour space %d\n", psBitmap->m_eColorSpc );
		}
		break;
	case DM_INVERT:
		switch ( psBitmap->m_eColorSpc )
		{
		case CS_RGB16:
			DrawLine16( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB16( sColor ), sColor, DM_INVERT );
			break;
		case CS_RGB32:
		case CS_RGBA32:
			DrawLine32( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB32( sColor ), DM_INVERT );
			break;
		default:
			dbprintf( "DisplayDriver::DrawLine() unknown colour space %d: can't invert\n", psBitmap->m_eColorSpc );
		}
		break;
	case DM_BLEND:
		switch ( psBitmap->m_eColorSpc )
		{
		case CS_RGB16:
			DrawLine16( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB16( sColor ), sColor, DM_BLEND );
			break;
		case CS_RGB32:
		case CS_RGBA32:
			DrawLine32( psBitmap, cClipRect, cPnt1.x, cPnt1.y, cPnt2.x, cPnt2.y, COL_TO_RGB32( sColor ), DM_BLEND );
			break;
		default:
			dbprintf( "DisplayDriver::DrawLine() unknown colour space %d: can't blend\n", psBitmap->m_eColorSpc );
		}
		break;
	}
	
	return ( true );
}
