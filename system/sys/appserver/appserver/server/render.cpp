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

#include <stdio.h>

#include "ddriver.h"
#include "server.h"
#include "layer.h"
#include "sfont.h"
#include "bitmap.h"
#include "sprite.h"
#include "swindow.h"

#include <macros.h>

#include <gui/rect.h>

#include <appserver/protocol.h>


static Color32_s g_asPens[128];

static Color32_s GetStdCol( int i )
{
	static uint8 DefaultPalette[] = {
		0x00, 0x60, 0x6b,
		0x00, 0x00, 0x00,
		0xff, 0xff, 0xff,
		0x66, 0x88, 0xbb,

		0x8f, 0x8f, 0x8f,
		0xc6, 0xc6, 0xc6,
		0xbb, 0xaa, 0x99,
		0xff, 0xbb, 0xaa,

		0xff, 0x00, 0x00,
		0x78, 0x78, 0x78,
		0xb4, 0xb4, 0xb4,
		0xdc, 0xdc, 0xdc,

		0x55, 0x55, 0xff,
		0x99, 0x22, 0xff,
		0x00, 0xff, 0x88,
		0xcc, 0xcc, 0xcc,

		0x00, 0x00, 0x00,
		0xe0, 0x40, 0x40,
		0x00, 0x00, 0x00,
		0xe0, 0xe0, 0xc0,
		0x44, 0x44, 0x44,
		0x55, 0x55, 0x55,
		0x66, 0x66, 0x66,
		0x77, 0x77, 0x77,
		0x88, 0x88, 0x88,
		0x99, 0x99, 0x99,
		0xaa, 0xaa, 0xaa,
		0xbb, 0xbb, 0xbb,
		0xcc, 0xcc, 0xcc,
		0xdd, 0xdd, 0xdd,
		0xee, 0xee, 0xee,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0x88, 0x88,
		0xff, 0xff, 0xff,
		0xcc, 0xcc, 0xcc,
		0x44, 0x44, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0xff, 0xff,
		0x44, 0x88, 0x88,
		0xcc, 0xff, 0xff,
		0x66, 0xcc, 0xcc,
		0x22, 0x44, 0x44,
		0xaa, 0xff, 0xff,
		0xee, 0xff, 0xff,
		0xcc, 0xff, 0xff,
		0x66, 0x88, 0x88,
		0xff, 0xff, 0xff,
		0x99, 0xcc, 0xcc,
		0x33, 0x44, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x44, 0xff, 0xff,
		0x22, 0x88, 0x88,
		0x66, 0xff, 0xff,
		0x33, 0xcc, 0xcc,
		0x11, 0x44, 0x44,
		0x55, 0xff, 0xff,
		0x77, 0xff, 0xff,
		0xff, 0x88, 0xff,
		0x88, 0x44, 0x88,
		0xff, 0xcc, 0xff,
		0xcc, 0x66, 0xcc,
		0x44, 0x22, 0x44,
		0xff, 0xaa, 0xff,
		0xff, 0xee, 0xff,
		0x88, 0x88, 0xff,
		0x44, 0x44, 0x88,
		0xcc, 0xcc, 0xff,
		0x66, 0x66, 0xcc,
		0x22, 0x22, 0x44,
		0xaa, 0xaa, 0xff,
		0xee, 0xee, 0xff,
		0xcc, 0x88, 0xff,
		0x66, 0x44, 0x88,
		0xff, 0xcc, 0xff,
		0x99, 0x66, 0xcc,
		0x33, 0x22, 0x44,
		0xff, 0xaa, 0xff,
		0xff, 0xee, 0xff,
		0x44, 0x88, 0xff,
		0x22, 0x44, 0x88,
		0x66, 0xcc, 0xff,
		0x33, 0x66, 0xcc,
		0x11, 0x22, 0x44,
		0x55, 0xaa, 0xff,
		0x77, 0xee, 0xff,
		0xff, 0xcc, 0xff,
		0x88, 0x66, 0x88,
		0xff, 0xff, 0xff,
		0xcc, 0x99, 0xcc,
		0x44, 0x33, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0xcc, 0xff,
		0x44, 0x66, 0x88,
		0xcc, 0xff, 0xff,
		0x66, 0x99, 0xcc,
		0x22, 0x33, 0x44,
		0xaa, 0xff, 0xff,
		0xee, 0xff, 0xff,
		0xcc, 0xcc, 0xff,
		0x66, 0x66, 0x88,
		0xff, 0xff, 0xff,
		0x99, 0x99, 0xcc,
		0x33, 0x33, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x44, 0xcc, 0xff,
		0x22, 0x66, 0x88,
		0x66, 0xff, 0xff,
		0x33, 0x99, 0xcc,
		0x11, 0x33, 0x44,
		0x55, 0xff, 0xff,
		0x77, 0xff, 0xff,
		0xff, 0x44, 0xff,
		0x88, 0x22, 0x88,
		0xff, 0x66, 0xff,
		0xcc, 0x33, 0xcc,
		0x44, 0x11, 0x44,
		0xff, 0x55, 0xff,
		0xff, 0x77, 0xff,
		0x88, 0x44, 0xff,
		0x44, 0x22, 0x88,
		0xcc, 0x66, 0xff,
		0x66, 0x33, 0xcc,
		0x22, 0x11, 0x44,
		0xaa, 0x55, 0xff,
		0xee, 0x77, 0xff,
		0xcc, 0x44, 0xff,
		0x66, 0x22, 0x88,
		0xff, 0x66, 0xff,
		0x99, 0x33, 0xcc,
		0x33, 0x11, 0x44,
		0xff, 0x55, 0xff,
		0xff, 0x77, 0xff,
		0x44, 0x44, 0xff,
		0x22, 0x22, 0x88,
		0x66, 0x66, 0xff,
		0x33, 0x33, 0xcc,
		0x11, 0x11, 0x44,
		0x55, 0x55, 0xff,
		0x77, 0x77, 0xff,
		0xff, 0xff, 0x88,
		0x88, 0x88, 0x44,
		0xff, 0xff, 0xcc,
		0xcc, 0xcc, 0x66,
		0x44, 0x44, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x88, 0xff, 0x88,
		0x44, 0x88, 0x44,
		0xcc, 0xff, 0xcc,
		0x66, 0xcc, 0x66,
		0x22, 0x44, 0x22,
		0xaa, 0xff, 0xaa,
		0xee, 0xff, 0xee,
		0xcc, 0xff, 0x88,
		0x66, 0x88, 0x44,
		0xff, 0xff, 0xcc,
		0x99, 0xcc, 0x66,
		0x33, 0x44, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x44, 0xff, 0x88,
		0x22, 0x88, 0x44,
		0x66, 0xff, 0xcc,
		0x33, 0xcc, 0x66,
		0x11, 0x44, 0x22,
		0x55, 0xff, 0xaa,
		0x77, 0xff, 0xee,
		0xff, 0x88, 0x88,
		0x88, 0x44, 0x44,
		0xff, 0xcc, 0xcc,
		0xcc, 0x66, 0x66,
		0x44, 0x22, 0x22,
		0xff, 0xaa, 0xaa,
		0xff, 0xee, 0xee,
		0x88, 0x88, 0x88,
		0x44, 0x44, 0x44,
		0xcc, 0xcc, 0xcc,
		0x66, 0x66, 0x66,
		0x22, 0x22, 0x22,
		0xaa, 0xaa, 0xaa,
		0xee, 0xee, 0xee,
		0xcc, 0x88, 0x88,
		0x66, 0x44, 0x44,
		0xff, 0xcc, 0xcc,
		0x99, 0x66, 0x66,
		0x33, 0x22, 0x22,
		0xff, 0xaa, 0xaa,
		0xff, 0xee, 0xee,
		0x44, 0x88, 0x88,
		0x22, 0x44, 0x44,
		0x66, 0xcc, 0xcc,
		0x33, 0x66, 0x66,
		0x11, 0x22, 0x22,
		0x55, 0xaa, 0xaa,
		0x77, 0xee, 0xee,
		0xff, 0xcc, 0x88,
		0x88, 0x66, 0x44,
		0xff, 0xff, 0xcc,
		0xcc, 0x99, 0x66,
		0x44, 0x33, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x88, 0xcc, 0x88,
		0x44, 0x66, 0x44,
		0xcc, 0xff, 0xcc,
		0x66, 0x99, 0x66,
		0x22, 0x33, 0x22,
		0xaa, 0xff, 0xaa,
		0xee, 0xff, 0xee,
		0xcc, 0xcc, 0x88,
		0x66, 0x66, 0x44,
		0xff, 0xff, 0xcc,
		0x99, 0x99, 0x66,
		0x33, 0x33, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x44, 0xcc, 0x88,
		0x22, 0x66, 0x44,
		0x66, 0xff, 0xcc,
		0x33, 0x99, 0x66,
		0x11, 0x33, 0x22,
		0x55, 0xff, 0xaa,
		0x77, 0xff, 0xee,
		0xff, 0x44, 0x88,
		0x88, 0x22, 0x44,
		0xff, 0x66, 0xcc,
		0xcc, 0x33, 0x66,
		0x44, 0x11, 0x22,
		0xff, 0x55, 0xaa,
		0xff, 0x77, 0xee,
		0x88, 0x44, 0x88,
		0x44, 0x22, 0x44,
		0xcc, 0x66, 0xcc,
		0x66, 0x33, 0x66,
		0x22, 0x11, 0x22,
		0xaa, 0x55, 0xaa,
		0xee, 0x77, 0xee,
		0xcc, 0x44, 0x88,
		0x66, 0x22, 0x44,
		0xff, 0x66, 0xcc,
		0x99, 0x33, 0x66,
		0x33, 0x11, 0x22,
		0xff, 0x55, 0xaa,
		0xff, 0x77, 0xee,
		0x44, 0x44, 0x88,
		0xbb, 0xbb, 0xbb,
		0x99, 0x99, 0x99,
		0x8f, 0x8f, 0x8f,
		0xc6, 0xc6, 0xc6,
		0xbb, 0xaa, 0x99,
		0x4d, 0x75, 0x44
	};

	Color32_s sColor;

	sColor.red = DefaultPalette[i * 3 + 0];
	sColor.green = DefaultPalette[i * 3 + 1];
	sColor.blue = DefaultPalette[i * 3 + 2];
	sColor.alpha = 0;

	return ( sColor );
}



Color32_s GetDefaultColor( int nIndex )
{
	static bool bFirst = true;

	if( bFirst )
	{
		g_asPens[PEN_BACKGROUND] = GetStdCol( 4 );
		g_asPens[PEN_DETAIL] = GetStdCol( 1 );
		g_asPens[PEN_SHINE] = GetStdCol( 2 );
		g_asPens[PEN_SHADOW] = GetStdCol( 1 );
		g_asPens[PEN_BRIGHT] = GetStdCol( 11 );
		g_asPens[PEN_DARK] = GetStdCol( 9 );
		g_asPens[PEN_WINTITLE] = GetStdCol( 9 );
		g_asPens[PEN_WINBORDER] = GetStdCol( 10 );
		g_asPens[PEN_SELWINTITLE] = GetStdCol( 3 );
		g_asPens[PEN_SELWINBORDER] = GetStdCol( 10 );
		g_asPens[PEN_WINDOWTEXT] = GetStdCol( 1 );
		g_asPens[PEN_SELWNDTEXT] = GetStdCol( 2 );
		g_asPens[PEN_WINCLIENT] = GetStdCol( 5 );
		g_asPens[PEN_GADGETFILL] = GetStdCol( 4 );
		g_asPens[PEN_SELGADGETFILL] = GetStdCol( 9 );
		g_asPens[PEN_GADGETTEXT] = GetStdCol( 2 );
		g_asPens[PEN_SELGADGETTEXT] = GetStdCol( 1 );
		bFirst = false;
	}
	return ( g_asPens[nIndex] );
}

void Layer::DrawFrame( const Rect & cRect, uint32 nStyle )
{
	bool Sunken = false;

	if( ( ( nStyle & FRAME_RAISED ) == 0 ) && ( nStyle & ( FRAME_RECESSED ) ) )
	{
		Sunken = true;
	}

	if( nStyle & FRAME_FLAT )
	{
	}
	else
	{
		Color32_s sShinePen = GetDefaultColor( PEN_SHINE );
		Color32_s sShadowPen = GetDefaultColor( PEN_SHADOW );

		if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
		{
			FillRect( Rect( cRect.left + 2, cRect.top + 2, cRect.right - 2, cRect.bottom - 2 ), m_sEraseColor );
		}

		SetFgColor( ( Sunken ) ? sShadowPen : sShinePen );

		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( Point( cRect.left, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.top ) );

		SetFgColor( ( Sunken ) ? sShinePen : sShadowPen );

		DrawLine( Point( cRect.right, cRect.bottom ) );
		DrawLine( Point( cRect.left, cRect.bottom ) );


		if( ( nStyle & FRAME_THIN ) == 0 )
		{
			if( nStyle & FRAME_ETCHED )
			{
				SetFgColor( ( Sunken ) ? sShadowPen : sShinePen );

				MovePenTo( cRect.left + 1, cRect.bottom - 1 );

				DrawLine( Point( cRect.left + 1, cRect.top + 1 ) );
				DrawLine( Point( cRect.right - 1, cRect.top + 1 ) );

				SetFgColor( ( Sunken ) ? sShinePen : sShadowPen );

				DrawLine( Point( cRect.right - 1, cRect.bottom - 1 ) );
				DrawLine( Point( cRect.left + 1, cRect.bottom - 1 ) );
			}
			else
			{
				Color32_s sBrightPen = GetDefaultColor( PEN_BRIGHT );
				Color32_s sDarkPen = GetDefaultColor( PEN_DARK );

				SetFgColor( ( Sunken ) ? sDarkPen : sBrightPen );

				MovePenTo( cRect.left + 1, cRect.bottom - 1 );

				DrawLine( Point( cRect.left + 1, cRect.top + 1 ) );
				DrawLine( Point( cRect.right - 1, cRect.top + 1 ) );

				SetFgColor( ( Sunken ) ? sBrightPen : sDarkPen );

				DrawLine( Point( cRect.right - 1, cRect.bottom - 1 ) );
				DrawLine( Point( cRect.left + 1, cRect.bottom - 1 ) );
			}
		}
		else
		{
			if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
			{
				FillRect( Rect( cRect.left + 1, cRect.top + 1, cRect.right - 1, cRect.bottom - 1 ), m_sEraseColor );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawLine( const Point & cToPos )
{
	if( NULL == m_pcBitmap )
	{
		return;
	}
	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}

	Point cTopLeft = ConvertToRoot( Point( 0, 0 ) );

	IPoint cMin( m_cPenPos + cTopLeft + m_cScrollOffset );
	IPoint cMax( cToPos + cTopLeft + m_cScrollOffset );

	m_cPenPos = cToPos;

	ClipRect *pcClip;

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + cTopLeft ) );
	}

	IPoint cITopLeft( cTopLeft );

	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		m_pcBitmap->m_pcDriver->DrawLine( m_pcBitmap, pcClip->m_cBounds + cITopLeft, cMin, cMax, m_sFgColor, m_nDrawingMode );
	}
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawString( const char *pzString, int nLength )
{
	if( m_pcBitmap == NULL )
	{
		return;
	}
	if( m_pcFont == NULL )
	{
		return;
	}
	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}

	if( m_nDrawingMode == DM_COPY && m_bFontPalletteValid == false )
	{
		m_asFontPallette[0] = m_sBgColor;
		m_asFontPallette[NUM_FONT_GRAYS - 1] = m_sFgColor;


		for( int i = 1; i < NUM_FONT_GRAYS - 1; ++i )
		{
			m_asFontPallette[i].red = m_sBgColor.red + int ( m_sFgColor.red - m_sBgColor.red ) * i / ( NUM_FONT_GRAYS - 1 );
			m_asFontPallette[i].green = m_sBgColor.green + int ( m_sFgColor.green - m_sBgColor.green ) * i / ( NUM_FONT_GRAYS - 1 );
			m_asFontPallette[i].blue = m_sBgColor.blue + int ( m_sFgColor.blue - m_sBgColor.blue ) * i / ( NUM_FONT_GRAYS - 1 );
		}

		/* Convert font pallette to colorspace */
		switch ( m_pcBitmap->m_eColorSpc )
		{
		case CS_RGB15:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB15( m_asFontPallette[i] );
			}
			break;
		case CS_RGB16:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB16( m_asFontPallette[i] );
			}
			break;
		case CS_RGB32:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB32( m_asFontPallette[i] );
			}
			break;
		default:
			dbprintf( "Layer::DrawString() unknown colorspace %d\n", m_pcBitmap->m_eColorSpc );
			break;
		}
		m_bFontPalletteValid = true;
	}

	if( nLength == -1 )
	{
		nLength = strlen( pzString );
	}

	Point cTopLeft = ConvertToRoot( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	IPoint cPos( m_cPenPos + cTopLeft + m_cScrollOffset );

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + cTopLeft ) );
	}
	if( m_nDrawingMode == DM_COPY )
	{

		if( FontServer::Lock() )
		{
			while( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );

				if( nCharLen > nLength )
				{
					break;
				}
				Glyph *pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );

				pzString += nCharLen;
				nLength -= nCharLen;
				if( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString:2() failed to load glyph\n" );
					continue;
				}

				ClipRect *pcClip;

				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyph( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_anFontPalletteConverted );
				}
				cPos.x += pcGlyph->m_nAdvance.x;
				m_cPenPos.x += pcGlyph->m_nAdvance.x;
			}
			FontServer::Unlock();
		}
	}
	else if( m_nDrawingMode == DM_BLEND && m_pcBitmap->m_eColorSpc == CS_RGB32 )
	{
		if( FontServer::Lock() )
		{
			while( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );

				if( nCharLen > nLength )
				{
					break;
				}
				Glyph *pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );

				pzString += nCharLen;
				nLength -= nCharLen;
				if( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString:2() failed to load glyph\n" );
					continue;
				}

				ClipRect *pcClip;

				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyphBlend( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
				cPos.x += pcGlyph->m_nAdvance.x;
				m_cPenPos.x += pcGlyph->m_nAdvance.x;
			}
			FontServer::Unlock();
		}
	}
	else
	{
		if( FontServer::Lock() )
		{
			while( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );

				if( nCharLen > nLength )
				{
					break;
				}
				Glyph *pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );

				pzString += nCharLen;
				nLength -= nCharLen;
				if( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString:2() failed to load glyph\n" );
					continue;
				}

				ClipRect *pcClip;

				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyph( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
				cPos.x += pcGlyph->m_nAdvance.x;
				m_cPenPos.x += pcGlyph->m_nAdvance.x;
			}
			FontServer::Unlock();
		}
	}
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}

int _FindEndOfLine( const char* pzStr, int nLen )
{
	int len, i = 0;
	
	while( nLen > 0 && *pzStr != '\n' ) {
		len = utf8_char_length( *pzStr );
		nLen -= len;
		pzStr += len;
		i += len;
	}
	
	return i;
}

void Layer::DrawText( const Rect& cRect, const char *pzString, int nLength, uint32 nFlags )
{
	if( m_pcBitmap == NULL || m_pcFont == NULL )
		return;

	Region *pcReg = GetRegion();

	if( NULL == pcReg )
		return;

	if( m_nDrawingMode == DM_COPY && m_bFontPalletteValid == false )
	{
		m_asFontPallette[0] = m_sBgColor;
		m_asFontPallette[NUM_FONT_GRAYS - 1] = m_sFgColor;


		for( int i = 1; i < NUM_FONT_GRAYS - 1; ++i )
		{
			m_asFontPallette[i].red = m_sBgColor.red + int ( m_sFgColor.red - m_sBgColor.red ) * i / ( NUM_FONT_GRAYS - 1 );
			m_asFontPallette[i].green = m_sBgColor.green + int ( m_sFgColor.green - m_sBgColor.green ) * i / ( NUM_FONT_GRAYS - 1 );
			m_asFontPallette[i].blue = m_sBgColor.blue + int ( m_sFgColor.blue - m_sBgColor.blue ) * i / ( NUM_FONT_GRAYS - 1 );
		}

		/* Convert font pallette to colorspace */
		switch ( m_pcBitmap->m_eColorSpc )
		{
			case CS_RGB15:
				for( int i = 1; i < NUM_FONT_GRAYS; ++i )
				{
					m_anFontPalletteConverted[i] = COL_TO_RGB15( m_asFontPallette[i] );
				}
				break;
			case CS_RGB16:
				for( int i = 1; i < NUM_FONT_GRAYS; ++i )
				{
					m_anFontPalletteConverted[i] = COL_TO_RGB16( m_asFontPallette[i] );
				}
				break;
			case CS_RGB32:
				for( int i = 1; i < NUM_FONT_GRAYS; ++i )
				{
					m_anFontPalletteConverted[i] = COL_TO_RGB32( m_asFontPallette[i] );
				}
				break;
			default:
				dbprintf( "Layer::DrawString() unknown colorspace %d\n", m_pcBitmap->m_eColorSpc );
				break;
		}
		m_bFontPalletteValid = true;
	}

	if( nLength == -1 )
	{
		nLength = strlen( pzString );
	}

	int			nRows;
	int			i;
	const char	*p;

	// Count rows.
	for( nRows = 1, p = pzString, i = nLength; i > 0; ) {
		if( *p == '\n' ) nRows++;

		int len = utf8_char_length( *p );
		i -= len;
		p += len;
	}

	Point cTopLeft = ConvertToRoot( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	IPoint cPos( cTopLeft + m_cScrollOffset );
	cPos.x += (int)cRect.left;
	cPos.y += (int)cRect.top;
	IPoint cStartPos = cPos;
	
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + cTopLeft ) );
	}

	if( FontServer::Lock() )
	{
		int nLineHeight = 0;
		bool bNewLine = true;

		nLineHeight = m_pcFont->GetInstance()->GetAscender() - m_pcFont->GetInstance()->GetDescender() + m_pcFont->GetInstance()->GetLineGap();

		if( nFlags & DTF_ALIGN_TOP ) {
		} else if( nFlags & DTF_ALIGN_BOTTOM ) {
			cPos.y += (int)(m_pcFont->GetInstance()->GetAscender() + cRect.Height() - ( nRows * nLineHeight - m_pcFont->GetInstance()->GetLineGap() ) );
		} else {
			cPos.y += (int)(m_pcFont->GetInstance()->GetAscender() + cRect.Height() * 0.5 - ( nRows * nLineHeight + 0.5 - m_pcFont->GetInstance()->GetLineGap() ) * 0.5 );
		}

		while( nLength > 0 )
		{
			bool bUnderlineChar = false;

			if( ! ( nFlags & DTF_IGNORE_FMT ) ) {
				bool bDone;
				do {
					bDone = false;
					switch( *pzString ) {
						case '_':
							bUnderlineChar = true;
							pzString++;
							nLength--;
							break;
						case '\n':
							pzString++;
							nLength--;
							cPos.y += nLineHeight;
							bNewLine = true;
							break;
						case 27:
							pzString++;
							nLength--;
							switch( *pzString ) {
								case 'r':
									nFlags &= ~(DTF_ALIGN_RIGHT|DTF_ALIGN_CENTER);
									nFlags |= DTF_ALIGN_RIGHT;
									pzString++;
									nLength--;
									break;
								case 'l':
									nFlags &= ~(DTF_ALIGN_RIGHT|DTF_ALIGN_CENTER);
									pzString++;
									nLength--;
									break;
								case 'c':
									nFlags &= ~(DTF_ALIGN_RIGHT|DTF_ALIGN_CENTER);
									nFlags |= DTF_ALIGN_CENTER;
									pzString++;
									nLength--;
									break;
								case '[':
									break;
							}
							break;
						default:
							bDone = true;
					}
				} while( nLength > 0 && !bDone );
			}

			if( bNewLine ) {
				if( nFlags & DTF_ALIGN_CENTER ) {
					cPos.x = (int)(cStartPos.x + cRect.Width()/2 - (m_pcFont->GetInstance()->GetTextExtent( pzString, _FindEndOfLine( pzString, nLength ), nFlags ) ).x/2);
				} else if( nFlags & DTF_ALIGN_RIGHT ) {
					cPos.x = (int)(cStartPos.x + cRect.Width() - (m_pcFont->GetInstance()->GetTextExtent( pzString, _FindEndOfLine( pzString, nLength ), nFlags ) ).x);
				} else {
					cPos.x = cStartPos.x;
				}
				bNewLine = false;
			}

			int nCharLen = utf8_char_length( *pzString );

			if( nCharLen > nLength )
				break;

			Glyph *pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );

			pzString += nCharLen;
			nLength -= nCharLen;
			if( pcGlyph == NULL )
			{
				//printf( "Error: Layer::DrawString:2() failed to load glyph\n" );
				continue;
			}

			ClipRect *pcClip;

			if( m_nDrawingMode == DM_COPY )
			{
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyph( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_anFontPalletteConverted );
				}
			} else if( m_nDrawingMode == DM_BLEND && m_pcBitmap->m_eColorSpc == CS_RGB32 ) {
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyphBlend( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
			} else {
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					m_pcBitmap->m_pcDriver->RenderGlyph( m_pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
			}

			if( bUnderlineChar ) {
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					IPoint cMax( cPos );
					cMax.x += pcGlyph->m_nAdvance.x;
					m_pcBitmap->m_pcDriver->DrawLine( m_pcBitmap, pcClip->m_cBounds + cITopLeft, cPos, cMax, m_sFgColor, m_nDrawingMode );
				}
			}

			cPos.x += pcGlyph->m_nAdvance.x;
		}
		FontServer::Unlock();
	}

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}

	PutRegion( pcReg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::CopyRect( SrvBitmap * pcBitmap, GRndCopyRect_s * psCmd )
{
	ScrollRect( pcBitmap, psCmd->cSrcRect, psCmd->cDstRect.LeftTop() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::FillRect( Rect cRect )
{
	if( NULL == m_pcBitmap )
	{
		return;
	}
	FillRect( cRect, m_sFgColor );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::FillRect( Rect cRect, Color32_s sColor )
{
	if( NULL == m_pcBitmap )
	{
		return;
	}

	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}

	Point cTopLeft = ConvertToRoot( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	ClipRect *pcClip;
	IRect cDstRect( cRect + m_cScrollOffset );

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + cTopLeft ) );
	}
	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		IRect cRect = cDstRect & pcClip->m_cBounds;

		if( cRect.IsValid() )
		{
			m_pcBitmap->m_pcDriver->FillRect( m_pcBitmap, cRect + cITopLeft, sColor );
		}
	}
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
//      To be removed when ScrollRect() are repaired.
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::BitBlit( SrvBitmap * pcBitmap, Rect cSrcRect, Point cDstPos )
{
	ConvertToRoot( &cSrcRect );
	ConvertToRoot( &cDstPos );

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + ConvertToRoot( Point( 0, 0 ) ) ) );
	}
	pcBitmap->m_pcDriver->BltBitmap( pcBitmap, pcBitmap, static_cast < IRect > ( cSrcRect + m_cScrollOffset ), static_cast < IPoint > ( cDstPos + m_cScrollOffset ), m_nDrawingMode );
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static int SortCmp( const void *pNode1, const void *pNode2 )
{
	ClipRect *pcClip1 = *( ( ClipRect ** ) pNode1 );
	ClipRect *pcClip2 = *( ( ClipRect ** ) pNode2 );


	if( pcClip1->m_cBounds.left > pcClip2->m_cBounds.right && pcClip1->m_cBounds.right < pcClip2->m_cBounds.left )
	{
		if( pcClip1->m_cMove.x < 0 )
		{
			return ( pcClip1->m_cBounds.left - pcClip2->m_cBounds.left );
		}
		else
		{
			return ( pcClip2->m_cBounds.left - pcClip1->m_cBounds.left );
		}
	}
	else
	{
		if( pcClip1->m_cMove.y < 0 )
		{
			return ( pcClip1->m_cBounds.top - pcClip2->m_cBounds.top );
		}
		else
		{
			return ( pcClip2->m_cBounds.top - pcClip1->m_cBounds.top );
		}
	}
}

void Layer::ScrollRect( SrvBitmap * pcBitmap, Rect cSrcRect, Point cDstPos )
{
	if( m_pcVisibleReg == NULL )
	{
		return;
	}

	IRect cISrcRect( cSrcRect );
	IPoint cDelta = IPoint( cDstPos ) - cISrcRect.LeftTop();
	IRect cIDstRect = cISrcRect + cDelta;

	ClipRect *pcSrcClip;
	ClipRect *pcDstClip;
	ClipRectList cBltList;
	Region cDamage( *m_pcVisibleReg, cIDstRect, false );

	ENUMCLIPLIST( &m_pcVisibleReg->m_cRects, pcSrcClip )
	{

	  /***	Clip to source rectangle	***/
		IRect cSRect( cISrcRect & pcSrcClip->m_cBounds );

		if( cSRect.IsValid() == false )
		{
			continue;
		}

	  /***	Transform into destination space	***/
		cSRect += cDelta;

		ENUMCLIPLIST( &m_pcVisibleReg->m_cRects, pcDstClip )
		{
			IRect cDRect = cSRect & pcDstClip->m_cBounds;

			if( cDRect.IsValid() == false )
			{
				continue;
			}
			cDamage.Exclude( cDRect );
			ClipRect *pcClips = Region::AllocClipRect();

			pcClips->m_cBounds = cDRect;
			pcClips->m_cMove = cDelta;

			cBltList.AddRect( pcClips );
		}
	}

	int nCount = cBltList.GetCount();

	if( nCount == 0 )
	{
		Invalidate( cIDstRect );
		UpdateIfNeeded( true );
		return;
	}

	IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

	ClipRect **apsClips = new ClipRect *[nCount];

	for( int i = 0; i < nCount; ++i )
	{
		apsClips[i] = cBltList.RemoveHead();
		__assertw( apsClips[i] != NULL );
	}
	qsort( apsClips, nCount, sizeof( ClipRect * ), SortCmp );

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + ConvertToRoot( Point( 0, 0 ) ) ) );
	}

	for( int i = 0; i < nCount; ++i )
	{
		ClipRect *pcClip = apsClips[i];

		pcClip->m_cBounds += cTopLeft;	// Convert into screen space
		m_pcBitmap->m_pcDriver->BltBitmap( m_pcBitmap, m_pcBitmap, pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ), DM_COPY );
		Region::FreeClipRect( pcClip );
	}
	delete[]apsClips;
	if( m_pcDamageReg != NULL )
	{
		ClipRect *pcDmgClip;
		Region cReg( *m_pcDamageReg, cISrcRect, false );

		ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
		{
			m_pcDamageReg->Include( ( pcDmgClip->m_cBounds + cDelta ) & cIDstRect );
			if( m_pcActiveDamageReg != NULL )
			{
				m_pcActiveDamageReg->Exclude( ( pcDmgClip->m_cBounds + cDelta ) & cIDstRect );
			}
		}
	}
	if( m_pcActiveDamageReg != NULL )
	{
		ClipRect *pcDmgClip;
		Region cReg( *m_pcActiveDamageReg, cISrcRect, false );

		if( cReg.m_cRects.GetCount() > 0 )
		{
			if( m_pcDamageReg == NULL )
			{
				m_pcDamageReg = new Region();
			}
			ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
			{
				m_pcActiveDamageReg->Exclude( ( pcDmgClip->m_cBounds + cDelta ) & cIDstRect );
				m_pcDamageReg->Include( ( pcDmgClip->m_cBounds + cDelta ) & cIDstRect );
			}
		}
	}
	ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip )
	{
		Invalidate( pcDstClip->m_cBounds );
	}
	if( m_pcDamageReg != NULL )
	{
		m_pcDamageReg->Optimize();
	}
	if( m_pcActiveDamageReg != NULL )
	{
		m_pcActiveDamageReg->Optimize();
	}
	UpdateIfNeeded( true );
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawBitMap( SrvBitmap * pcDstBitmap, SrvBitmap * pcBitMap, Rect cSrcRect, Point cDstPos )
{
	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}
	IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );
	ClipRect *pcClip;

	IRect cISrcRect( cSrcRect );
	IPoint cIDstPos( cDstPos + m_cScrollOffset );
	IRect cDstRect = cISrcRect.Bounds() + cIDstPos;
	IPoint cSrcPos( cISrcRect.LeftTop() );

	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() ) + cTopLeft );
	}

	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		IRect cRect = cDstRect & pcClip->m_cBounds;

		if( cRect.IsValid() )
		{
			IPoint cDst = cRect.LeftTop() + cTopLeft;
			IRect cSrc = cRect - cIDstPos + cSrcPos;

			pcDstBitmap->m_pcDriver->BltBitmap( pcDstBitmap, pcBitMap, cSrc, cDst, m_nDrawingMode );
		}
	}
	if( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}
