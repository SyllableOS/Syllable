
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

using namespace os;


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

void Layer::DrawFrame( const Rect &a_cRect, uint32 nStyle )
{
	Rect cRect = a_cRect;
	Color32_s fg_save = m_sFgColor;
	Color32_s bg_save = m_sBgColor;
	
	cRect.Floor();
	bool bSunken = false;

	if( ( ( nStyle & FRAME_RAISED ) == 0 ) && ( nStyle & ( FRAME_RECESSED ) ) )
	{
		bSunken = true;
	}

	Color32_s sFgCol = get_default_color( COL_SHINE );
	Color32_s sBgCol = get_default_color( COL_SHADOW );

	if( nStyle & FRAME_DISABLED )
	{
		sFgCol = Tint( sFgCol, 0.6f );
		sBgCol = Tint( sBgCol, 0.4f );
	}
	Color32_s sFgShadowCol = Tint( sFgCol, 0.6f );
	Color32_s sBgShadowCol = Tint( sBgCol, 0.5f );

	if( nStyle & FRAME_FLAT )
	{
		SetFgColor( ( bSunken ) ? sBgCol : sFgCol );
		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( Point( cRect.left, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.bottom ) );
		DrawLine( Point( cRect.left, cRect.bottom ) );
	}
	else
	{
		if( nStyle & FRAME_THIN )
		{
			SetFgColor( ( bSunken ) ? sBgCol : sFgCol );
		}
		else
		{
			SetFgColor( ( bSunken ) ? sBgCol : sFgShadowCol );
		}

		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( Point( cRect.left, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.top ) );

		if( nStyle & FRAME_THIN )
		{
			SetFgColor( ( bSunken ) ? sFgCol : sBgCol );
		}
		else
		{
			SetFgColor( ( bSunken ) ? sFgCol : sBgShadowCol );
		}
		DrawLine( Point( cRect.right, cRect.bottom ) );
		DrawLine( Point( cRect.left, cRect.bottom ) );


		if( ( nStyle & FRAME_THIN ) == 0 )
		{
			if( nStyle & FRAME_ETCHED )
			{
				SetFgColor( ( bSunken ) ? sBgCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				DrawLine( Point( cRect.left + 1.0f, cRect.top + 1.0f ) );
				DrawLine( Point( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetFgColor( ( bSunken ) ? sFgCol : sBgCol );

				DrawLine( Point( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				DrawLine( Point( cRect.left + 1.0f, cRect.bottom - 1.0f ) );
			}
			else
			{
				SetFgColor( ( bSunken ) ? sBgShadowCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				DrawLine( Point( cRect.left + 1.0f, cRect.top + 1.0f ) );
				DrawLine( Point( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetFgColor( ( bSunken ) ? sFgShadowCol : sBgCol );

				DrawLine( Point( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				DrawLine( Point( cRect.left + 1.0f, cRect.bottom - 1.0f ) );
			}
			if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
			{

				FillRect( Rect( cRect.left + 2.0f, cRect.top + 2.0f, cRect.right - 2.0f, cRect.bottom - 2.0f ), m_sEraseColor );
			}
		}
		else
		{
			if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
			{
				FillRect( Rect( cRect.left + 1.0f, cRect.top + 1.0f, cRect.right - 1.0f, cRect.bottom - 1.0f ), m_sEraseColor );
			}
		}
	}

	SetFgColor( fg_save );
	SetBgColor( bg_save );
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

	Point cTopLeft = ConvertToBitmap( Point( 0, 0 ) );

	IPoint cMin( m_cPenPos + cTopLeft + m_cScrollOffset );
	IPoint cMax( cToPos + cTopLeft + m_cScrollOffset );

	m_cPenPos = cToPos;

	ClipRect *pcClip;
	
	os::IRect cHideRect;
	cHideRect |= cMin;
	cHideRect |= cMax;	

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Hide( cHideRect );
	}
	
	IPoint cITopLeft( cTopLeft );

	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		m_pcBitmap->m_pcDriver->DrawLine( m_pcBitmap, pcClip->m_cBounds + cITopLeft, cMin, cMax, m_sFgColor, m_nDrawingMode );
	}
	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
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
	if( m_pcFont->GetInstance() == NULL )
	{
		return;
	}
	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}
	
	SrvBitmap* pcBitmap = m_pcBitmap;

	if(m_nDrawingMode == DM_COPY && m_bFontPalletteValid == false )
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
		switch ( pcBitmap->m_eColorSpc )
		{
		case CS_RGB16:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB16( m_asFontPallette[i] );
			}
			break;
		case CS_RGB32:
		case CS_RGBA32:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB32( m_asFontPallette[i] );
			}
			break;
		default:
			dbprintf( "Layer::DrawString() unknown colorspace %d\n", pcBitmap->m_eColorSpc );
			break;
		}
		m_bFontPalletteValid = true;
	}

	if( nLength == -1 )
	{
		nLength = strlen( pzString );
	}

	Point cTopLeft = ConvertToBitmap( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	IPoint cPos( m_cPenPos + cTopLeft + m_cScrollOffset );

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
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
					pcBitmap->m_pcDriver->RenderGlyph( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_anFontPalletteConverted );
				}
				cPos.x += pcGlyph->m_nAdvance.x;
				m_cPenPos.x += pcGlyph->m_nAdvance.x;
			}
			FontServer::Unlock();
		}
	}
	else if( m_nDrawingMode == DM_BLEND && ( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGBA32 ) )
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
					pcBitmap->m_pcDriver->RenderGlyphBlend( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
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
					pcBitmap->m_pcDriver->RenderGlyph( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
				cPos.x += pcGlyph->m_nAdvance.x;
				m_cPenPos.x += pcGlyph->m_nAdvance.x;
			}
			FontServer::Unlock();
		}
	}
	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}

int _FindEndOfLine( const char *pzStr, int nLen )
{
	int len, i = 0;

	while( nLen > 0 && *pzStr != '\n' )
	{
		len = utf8_char_length( *pzStr );
		nLen -= len;
		pzStr += len;
		i += len;
	}

	return i;
}

void Layer::DrawText( const Rect & cRect, const char *pzString, int nLength, uint32 nFlags )
{
	if( m_pcBitmap == NULL || m_pcFont == NULL || m_pcFont->GetInstance() == NULL )
		return;

	Region *pcReg = GetRegion();
	SrvBitmap* pcBitmap = m_pcBitmap;

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
		switch ( pcBitmap->m_eColorSpc )
		{
		case CS_RGB16:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB16( m_asFontPallette[i] );
			}
			break;
		case CS_RGB32:
		case CS_RGBA32:
			for( int i = 1; i < NUM_FONT_GRAYS; ++i )
			{
				m_anFontPalletteConverted[i] = COL_TO_RGB32( m_asFontPallette[i] );
			}
			break;
		default:
			dbprintf( "Layer::DrawString() unknown colorspace %d\n", pcBitmap->m_eColorSpc );
			break;
		}
		m_bFontPalletteValid = true;
	}

	if( nLength == -1 )
	{
		nLength = strlen( pzString );
	}

	int nRows;
	int i;
	const char *p;

	// Count rows.
	for( nRows = 1, p = pzString, i = nLength; i > 0; )
	{
		if( *p == '\n' )
			nRows++;

		int len = utf8_char_length( *p );

		i -= len;
		p +=len;
	}

	Point cTopLeft = ConvertToBitmap( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	IPoint cPos( cTopLeft + m_cScrollOffset );

	cPos.x += ( int )cRect.left;
	cPos.y += ( int )cRect.top;
	IPoint cStartPos = cPos;

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false )  && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + cTopLeft ) );
	}

	if( FontServer::Lock() )
	{
		int nLineHeight = 0;
		bool bNewLine = true;

		nLineHeight = m_pcFont->GetInstance()->GetAscender(  ) - m_pcFont->GetInstance(  )->GetDescender(  ) + m_pcFont->GetInstance(  )->GetLineGap(  );

		if( nFlags & DTF_ALIGN_TOP )
		{
		}
		else if( nFlags & DTF_ALIGN_BOTTOM )
		{
			cPos.y += ( int )( m_pcFont->GetInstance()->GetAscender(  ) + cRect.Height(  ) - ( nRows * nLineHeight - m_pcFont->GetInstance(  )->GetLineGap(  ) ) );
		}
		else
		{
			cPos.y += ( int )( m_pcFont->GetInstance()->GetAscender(  ) + cRect.Height(  ) * 0.5 - ( nRows * nLineHeight + 0.5 - m_pcFont->GetInstance(  )->GetLineGap(  ) ) * 0.5 );
		}

		while( nLength > 0 )
		{
			bool bUnderlineChar = false;

			if( !( nFlags & DTF_IGNORE_FMT ) )
			{
				bool bDone;

				do
				{
					bDone = false;
					switch ( *pzString )
					{
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
						switch ( *pzString )
						{
						case 'r':
							nFlags &= ~( DTF_ALIGN_RIGHT | DTF_ALIGN_CENTER );
							nFlags |= DTF_ALIGN_RIGHT;
							pzString++;
							nLength--;
							break;
						case 'l':
							nFlags &= ~( DTF_ALIGN_RIGHT | DTF_ALIGN_CENTER );
							pzString++;
							nLength--;
							break;
						case 'c':
							nFlags &= ~( DTF_ALIGN_RIGHT | DTF_ALIGN_CENTER );
							nFlags |= DTF_ALIGN_CENTER;
							pzString++;
							nLength--;
							break;
						case 'd':
							nFlags |= DTF_IGNORE_FMT;
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
				}
				while( nLength > 0 && !bDone );
			}

			if( bNewLine )
			{
				if( nFlags & DTF_ALIGN_CENTER )
				{
					cPos.x = ( int )( cStartPos.x + cRect.Width() / 2 - ( m_pcFont->GetInstance(  )->GetTextExtent( pzString, _FindEndOfLine( pzString, nLength ), nFlags ) ).x / 2 );
				}
				else if( nFlags & DTF_ALIGN_RIGHT )
				{
					cPos.x = ( int )( cStartPos.x + cRect.Width() - ( m_pcFont->GetInstance(  )->GetTextExtent( pzString, _FindEndOfLine( pzString, nLength ), nFlags ) ).x );
				}
				else
				{
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
					pcBitmap->m_pcDriver->RenderGlyph( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_anFontPalletteConverted );
				}
			}
			else if( m_nDrawingMode == DM_BLEND && ( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGBA32 ) )
			{
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					pcBitmap->m_pcDriver->RenderGlyphBlend( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
			}
			else
			{
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					pcBitmap->m_pcDriver->RenderGlyph( pcBitmap, pcGlyph, cPos, pcClip->m_cBounds + cITopLeft, m_sFgColor );
				}
			}

			if( bUnderlineChar )
			{
				ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
				{
					IPoint cMax( cPos );

					cMax.x += pcGlyph->m_nAdvance.x;
					pcBitmap->m_pcDriver->DrawLine( pcBitmap, pcClip->m_cBounds + cITopLeft, cPos, cMax, m_sFgColor, m_nDrawingMode );
				}
			}

			cPos.x += pcGlyph->m_nAdvance.x;
		}
		FontServer::Unlock();
	}

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
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

void Layer::CopyRect( GRndCopyRect_s * psCmd )
{
	SrvBitmap* pcBitmap = m_pcBitmap;
	if( pcBitmap == NULL )
		return;
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

	SrvBitmap* pcBitmap = m_pcBitmap;
	Region *pcReg = GetRegion();

	if( NULL == pcReg )
	{
		return;
	}

	Point cTopLeft = ConvertToBitmap( Point( 0, 0 ) );
	IPoint cITopLeft( cTopLeft );
	ClipRect *pcClip;
	IRect cDstRect( cRect + m_cScrollOffset );

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Hide( cDstRect + cITopLeft );
	}
	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		IRect cRect = cDstRect & pcClip->m_cBounds;

		if( cRect.IsValid() )
		{
			pcBitmap->m_pcDriver->FillRect( pcBitmap, cRect + cITopLeft, sColor, m_nDrawingMode );
		}
	}
	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
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
	if( m_pcBitmapReg == NULL || m_pcBitmap == NULL )
	{
		return;
	}
	SrvBitmap* pcDstBitmap = m_pcBitmap;

	IRect cISrcRect( cSrcRect );
	IPoint cDelta = IPoint( cDstPos ) - cISrcRect.LeftTop();
	IRect cIDstRect = cISrcRect + cDelta;

	ClipRect *pcSrcClip;
	ClipRect *pcDstClip;
	ClipRectList cBltList;
	Region cDamage( *m_pcBitmapReg, cIDstRect, false );

	ENUMCLIPLIST( &m_pcBitmapReg->m_cRects, pcSrcClip )
	{

	  /***	Clip to source rectangle	***/
		IRect cSRect( cISrcRect & pcSrcClip->m_cBounds );

		if( cSRect.IsValid() == false )
		{
			continue;
		}

	  /***	Transform into destination space	***/
		cSRect += cDelta;

		ENUMCLIPLIST( &m_pcBitmapReg->m_cRects, pcDstClip )
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
		UpdateIfNeeded();
		return;
	}

	IPoint cTopLeft( ConvertToBitmap( Point( 0, 0 ) ) );

	ClipRect **apsClips = new ClipRect *[nCount];

	for( int i = 0; i < nCount; ++i )
	{
		apsClips[i] = cBltList.RemoveHead();
		__assertw( apsClips[i] != NULL );
	}
	qsort( apsClips, nCount, sizeof( ClipRect * ), SortCmp );

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Hide( static_cast < IRect > ( GetBounds() + ConvertToBitmap( Point( 0, 0 ) ) ) );
	}

	for( int i = 0; i < nCount; ++i )
	{
		ClipRect *pcClip = apsClips[i];

		pcClip->m_cBounds += cTopLeft;	// Convert into screen space
		pcDstBitmap->m_pcDriver->BltBitmap( pcDstBitmap, pcBitmap, pcClip->m_cBounds - pcClip->m_cMove, pcClip->m_cBounds, DM_COPY, 0xff );
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
	UpdateIfNeeded();
	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
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

void Layer::DrawBitMap( SrvBitmap * pcBitMap, Rect cSrcRect, Rect cDstRect )
{
	if( m_pcBitmap == NULL || pcBitMap == NULL )
		return;

	SrvBitmap* pcDstBitmap = m_pcBitmap;
	Region *pcReg = GetRegion();
	
	
	if( NULL == pcReg )
	{
		return;
	}
	IPoint cTopLeft( ConvertToBitmap( Point( 0, 0 ) ) );
	ClipRect *pcClip;
	

	IRect cISrcRect( cSrcRect );
	IRect cIDstRect( cDstRect + m_cScrollOffset );

	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Hide( cIDstRect + cTopLeft );
	}
	

	ENUMCLIPLIST( &pcReg->m_cRects, pcClip )
	{
		IRect cRect = cIDstRect & pcClip->m_cBounds;

		if( cRect.IsValid() )
		{
			IRect cDst = cRect + cTopLeft;
			IRect cSrc;
			if( cISrcRect.Size() != cIDstRect.Size() )
			{
				cSrc = cISrcRect;
				cSrc.left += ( cRect.left - cIDstRect.left ) * ( cISrcRect.Width() + 1 ) / ( cIDstRect.Width() + 1 );
				cSrc.right -= ( cIDstRect.right - cRect.right ) * ( cISrcRect.Width() + 1 ) / ( cIDstRect.Width() + 1 );
				cSrc.top += ( cRect.top - cIDstRect.top ) * ( cISrcRect.Height() + 1 ) / ( cIDstRect.Height() + 1 );
				cSrc.bottom -= ( cIDstRect.bottom - cRect.bottom ) * ( cISrcRect.Height() + 1 ) / ( cIDstRect.Height() + 1 );
			}
			else
				cSrc = cRect - cIDstRect.LeftTop() + cISrcRect.LeftTop();
			pcDstBitmap->m_pcDriver->BltBitmap( pcDstBitmap, pcBitMap, cSrc, cDst, m_nDrawingMode, 0xff );
		}
	}
	if( ( m_pcWindow == NULL || m_pcWindow->IsOffScreen() == false ) && m_pcBitmap == g_pcScreenBitmap )
	{
		SrvSprite::Unhide();
	}
	PutRegion( pcReg );
}

