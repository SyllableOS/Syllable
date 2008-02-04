
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

#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <gui/guidefines.h>
#include <util/locker.h>
#include <appserver/protocol.h>

#include "server.h"
#include "sfont.h"

#include <macros.h>

using namespace os;

#define FLOOR(x)  ((x) & -64)
#define CEIL(x)   (((x)+63) & -64)
#define TRUNC(x)  ((x) >> 6)

FontServer *FontServer::s_pcInstance = NULL;

Locker g_cFontLock( "font_lock" );

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFontInstance::SFontInstance( SFont * pcFont, const FontProperty & cFP )
{
	m_bFixedWidth = pcFont->IsFixedWidth();
	g_cFontLock.Lock();

	m_pcFont = pcFont;
	m_nGlyphCount = pcFont->GetGlyphCount();

	if( m_nGlyphCount > 0 )
	{
		m_ppcGlyphTable = new Glyph *[m_nGlyphCount];
		memset( m_ppcGlyphTable, 0, m_nGlyphCount * sizeof( Glyph * ) );
	}
	else
	{
		m_ppcGlyphTable = NULL;
	}
	m_cInstanceProperties = cFP;

	FT_Face psFace = m_pcFont->GetTTFace();

	if( psFace->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psFace, m_cInstanceProperties.m_nPointSize, m_cInstanceProperties.m_nPointSize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psFace, 0, ( m_cInstanceProperties.m_nPointSize * 96 / 72 ) / 64 );
	}
	FT_Size psSize = psFace->size;


	m_nNomWidth = psSize->metrics.x_ppem;
	m_nNomHeight = psSize->metrics.y_ppem;

	if( psSize->metrics.descender > 0 )
	{
		m_nDescender = -( psSize->metrics.descender + 63 ) / 64;
//      m_nLineGap   = (psSize->metrics.height - (psSize->metrics.ascender + psSize->metrics.descender) + 63) / 64;
	}
	else
	{
		m_nDescender = ( psSize->metrics.descender + 63 ) / 64;
//      m_nLineGap   = (psSize->metrics.height - (psSize->metrics.ascender - psSize->metrics.descender) + 63) / 64;
	}
	m_nAscender = ( psSize->metrics.ascender + 63 ) / 64;
	m_nLineGap = ( psSize->metrics.height + 63 ) / 64 - ( m_nAscender - m_nDescender );
	m_nAdvance = ( psSize->metrics.max_advance + 63 ) / 64;

//    printf( "Size1(%d): %ld, %ld, %ld (%ld, %ld, %ld)\n", nPointSize, psSize->metrics.ascender, psSize->metrics.descender, psSize->metrics.height,
//          psSize->metrics.ascender / 64, psSize->metrics.descender / 64, psSize->metrics.height / 64 );

	// Register our self with the font

	m_pcFont->AddInstance( this );

	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t SFontInstance::SetProperties( const FontProperty & cFP )
{
	g_cFontLock.Lock();

	m_pcFont->RemoveInstance( this );

	m_cInstanceProperties = cFP;

	FT_Face psFace = m_pcFont->GetTTFace();

	if( psFace->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psFace, m_cInstanceProperties.m_nPointSize, m_cInstanceProperties.m_nPointSize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psFace, 0, ( m_cInstanceProperties.m_nPointSize * 96 / 72 ) / 64 );
	}

	FT_Size psSize = psFace->size;

	m_nNomWidth = psSize->metrics.x_ppem;
	m_nNomHeight = psSize->metrics.y_ppem;

	if( psSize->metrics.descender > 0 )
	{
		m_nDescender = -( psSize->metrics.descender + 63 ) / 64;
//      m_nLineGap   = (psSize->metrics.height - (psSize->metrics.ascender + psSize->metrics.descender) + 63) / 64;
	}
	else
	{
		m_nDescender = ( psSize->metrics.descender + 63 ) / 64;
//      m_nLineGap   = (psSize->metrics.height - (psSize->metrics.ascender - psSize->metrics.descender) + 63) / 64;
	}
	m_nAscender = ( psSize->metrics.ascender + 63 ) / 64;
	m_nLineGap = ( psSize->metrics.height + 63 ) / 64 - ( m_nAscender - m_nDescender );
	m_nAdvance = ( psSize->metrics.max_advance + 63 ) / 64;

//    printf( "Size2(%d): %ld, %ld, %ld (%ld, %ld, %ld)\n", nPointSize, psSize->metrics.ascender, psSize->metrics.descender, psSize->metrics.height,
//          psSize->metrics.ascender / 64, psSize->metrics.descender / 64, psSize->metrics.height / 64 );

	for( int i = 0; i < m_nGlyphCount; ++i )
	{
		if( m_ppcGlyphTable[i] != NULL )
		{
			delete[]reinterpret_cast < char *>( m_ppcGlyphTable[i] );

			m_ppcGlyphTable[i] = NULL;
		}
	}
	m_pcFont->AddInstance( this );

	g_cFontLock.Unlock();

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFontInstance::~SFontInstance()
{
	g_cFontLock.Lock();
	m_pcFont->RemoveInstance( this );

	for( int i = 0; i < m_nGlyphCount; ++i )
	{
		delete[]reinterpret_cast < char *>( m_ppcGlyphTable[i] );
	}
	delete[]m_ppcGlyphTable;

	g_cFontLock.Unlock();
}

#if 0
void SFontInstance::CalculateHeight()
{
	m_nAscender = 0;
	m_nDescender = 0;
	m_nLineGap = 0;
	FT_Size psSize = m_pcFont->GetTTFace()->size;

/*    
    if ( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE ) {
	FT_Set_Char_Size( psSize->face, m_nPointSize, m_nPointSize, 96, 96 );
    } else {
	FT_Set_Pixel_Sizes( psSize->face, 0, (m_nPointSize*96/72) / 64 );
    }
    */

	for( int i = 0; i < m_nGlyphCount; ++i )
	{
		FT_Error nError;

		nError = FT_Load_Glyph( psSize->face, i, FT_LOAD_DEFAULT );

		if( nError != 0 )
		{
			dbprintf( "Unable to load glyph %d (%d) -> %02x\n", i, ( m_cInstanceProperties.m_nPointSize * 96 / 72 ) / 64, nError );
			continue;
		}
		FT_GlyphSlot glyph = psSize->face->glyph;

		int nLeft;
		int nTop;

		if( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE )
		{
			nLeft = FLOOR( glyph->metrics.horiBearingX ) / 64;
			nTop = -CEIL( glyph->metrics.horiBearingY ) / 64;
			nBottom = nTop + glyph->bitmap.rows - 1;
		}
		else
		{
			nLeft = glyph->bitmap_left;
			nTop = -glyph->bitmap_top;
		}
	}

}
#endif
//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Glyph *SFontInstance::GetGlyph( int nIndex )
{
	if( nIndex < 0 || nIndex >= m_nGlyphCount )
	{
		return ( NULL );
	}
	if( m_ppcGlyphTable[nIndex] != NULL )
	{
		return ( m_ppcGlyphTable[nIndex] );
	}
	FT_Error nError;

	FT_Size psSize = m_pcFont->GetTTFace()->size;

	if( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psSize->face, m_cInstanceProperties.m_nPointSize, m_cInstanceProperties.m_nPointSize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psSize->face, 0, ( m_cInstanceProperties.m_nPointSize * 96 / 72 ) / 64 );
	}

	FT_Set_Transform( psSize->face, NULL, NULL );

	if( m_cInstanceProperties.m_nShear || m_cInstanceProperties.m_nRotation )
	{
		FT_Matrix transform;

		transform.xx = ( FT_Fixed )( 0x10000L * cos( ( float )m_cInstanceProperties.m_nRotation / 0x10000f ) );
		transform.yx = ( FT_Fixed )( 0x10000L * sin( ( float )m_cInstanceProperties.m_nRotation / 0x10000f ) );
		transform.xy = ( FT_Fixed )( 0x10000L * -sin( ( float )m_cInstanceProperties.m_nRotation / 0x10000f ) + 0x10000L * tan( ( float )m_cInstanceProperties.m_nShear / 0x10000f ) );
		transform.yy = ( FT_Fixed )( 0x10000L * cos( ( float )m_cInstanceProperties.m_nRotation / 0x10000f ) );

		FT_Set_Transform( psSize->face, &transform, NULL );
	}

	if( m_cInstanceProperties.m_nFlags & FPF_ITALIC )
	{
//		FT_GlyphSlot_Oblique( glyph );

		FT_Matrix transform;

		transform.xx = ( FT_Fixed )( 0x10000L );
		transform.yx = ( FT_Fixed )( 0x00000L );
		transform.xy = ( FT_Fixed )( 0x06000L );
		transform.yy = ( FT_Fixed )( 0x10000L );

		FT_Set_Transform( psSize->face, &transform, NULL );
	//	dbprintf( "Oblique\n" );
	}

	nError = FT_Load_Glyph( psSize->face, nIndex, FT_LOAD_DEFAULT );

	if( nError != 0 )
	{
		dbprintf( "Unable to load glyph %d (%d) -> %02x\n", nIndex, ( m_cInstanceProperties.m_nPointSize * 96 / 72 ) / 64, nError );
		if( nIndex != 0 )
		{
			return ( GetGlyph( 0 ) );
		}
		else
		{
			return ( NULL );
		}
	}
	FT_GlyphSlot glyph = psSize->face->glyph;

	if( m_cInstanceProperties.m_nFlags & FPF_BOLD )
	{
		FT_GlyphSlot_Embolden( glyph );
		dbprintf( "Embolden\n" );
	}


	int nLeft;
	int nTop;

	if( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		nLeft = FLOOR( glyph->metrics.horiBearingX ) / 64;
		nTop = -CEIL( glyph->metrics.horiBearingY ) / 64;
	}
	else
	{
		nLeft = glyph->bitmap_left;
		nTop = -glyph->bitmap_top;
	}

	if( m_pcFont->IsScalable() )
	{
		if( m_cInstanceProperties.m_nFlags & FPF_SMOOTHED )
		{
			nError = FT_Render_Glyph( glyph, FT_RENDER_MODE_NORMAL );
		}
		else
		{
			nError = FT_Render_Glyph( glyph, FT_RENDER_MODE_MONO );
		}

		if( nError != 0 )
		{
			dbprintf( "Failed to render glyph, err = %x\n", nError );
			if( nIndex != 0 )
			{
				return ( GetGlyph( 0 ) );
			}
			else
			{
				return ( NULL );
			}
		}
	}
	if( glyph->bitmap.width < 0 || glyph->bitmap.rows < 0 || glyph->bitmap.pitch < 0 )
	{
		dbprintf( "Error: Glyph got invalid size %dx%d (%d)\n", glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.pitch );
		if( nIndex != 0 )
		{
			return ( GetGlyph( 0 ) );
		}
		else
		{
			return ( NULL );
		}
	}
	IRect cBounds( nLeft, nTop, nLeft + glyph->bitmap.width - 1, nTop + glyph->bitmap.rows - 1 );

	int nRasterSize;

	if( glyph->bitmap.pixel_mode == ft_pixel_mode_grays )
	{
		nRasterSize = glyph->bitmap.pitch * glyph->bitmap.rows;
	}
	else if( glyph->bitmap.pixel_mode == ft_pixel_mode_mono )
	{
		nRasterSize = glyph->bitmap.width * glyph->bitmap.rows;
	}
	else
	{
		dbprintf( "SFontInstance::GetGlyph() unknown pixel mode : %d\n", glyph->bitmap.pixel_mode );
		if( nIndex != 0 )
		{
			return ( GetGlyph( 0 ) );
		}
		else
		{
			return ( NULL );
		}
	}
	try
	{
		m_ppcGlyphTable[nIndex] = ( Glyph * ) new char[sizeof( Glyph ) + nRasterSize];
	}
	catch( ... )
	{
		return ( NULL );
	}
	Glyph *pcGlyph = m_ppcGlyphTable[nIndex];

	pcGlyph->m_pRaster = ( uint8 * )( pcGlyph + 1 );
	pcGlyph->m_cBounds = cBounds;

	if( m_pcFont->IsScalable() )
	{
		if( IsFixedWidth() )
		{
			pcGlyph->m_nAdvance.x = m_nAdvance;
		}
		else
		{
			pcGlyph->m_nAdvance.x = ( glyph->metrics.horiAdvance + 32 ) / 64;
			pcGlyph->m_nAdvance.y = ( glyph->metrics.vertAdvance + 32 ) / 64;
		}
	}
	else
	{
		pcGlyph->m_nAdvance.x = glyph->bitmap.width;
	}

	if( glyph->bitmap.pixel_mode == ft_pixel_mode_grays )
	{
		pcGlyph->m_nBytesPerLine = glyph->bitmap.pitch;
		memcpy( pcGlyph->m_pRaster, glyph->bitmap.buffer, nRasterSize );
	}
	else
	{
		pcGlyph->m_nBytesPerLine = glyph->bitmap.width;

		for( int y = 0; y < pcGlyph->m_cBounds.Height() + 1; ++y )
		{
			for( int x = 0; x < pcGlyph->m_cBounds.Width() + 1; ++x )
			{
				if( glyph->bitmap.buffer[x / 8 + y * glyph->bitmap.pitch] & ( 1 << ( 7 - ( x % 8 ) ) ) )
				{
					pcGlyph->m_pRaster[x + y * pcGlyph->m_nBytesPerLine] = 255;
				}
				else
				{
					pcGlyph->m_pRaster[x + y * pcGlyph->m_nBytesPerLine] = 0;
				}
			}
		}
	}
	return ( m_ppcGlyphTable[nIndex] );
}

bool SFontInstance::GetFirstSupportedChar( uint32* pnChar )
{
	FT_ULong	nCharCode;
	FT_UInt		nGIndex;
	FT_Face psFace = m_pcFont->GetTTFace();

	nCharCode = FT_Get_First_Char( psFace, &nGIndex );
	if( nGIndex ) {
		dbprintf("s: %u\n", ( unsigned int )nCharCode);
		*pnChar = nCharCode;
		return true;
	}
	return false;
}

bool SFontInstance::GetNextSupportedChar( uint32* pnLastChar )
{
	FT_ULong	nCharCode;
	FT_UInt		nGIndex;
	FT_Face psFace = m_pcFont->GetTTFace();

	nCharCode = FT_Get_Next_Char( psFace, *pnLastChar, &nGIndex );
	if( nGIndex ) {
		*pnLastChar = nCharCode;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int SFontInstance::GetStringWidth( const char *pzString, int nLength )
{
	int nWidth = 0;

	g_cFontLock.Lock();
	while( nLength > 0 )
	{
		int nCharLen = utf8_char_length( *pzString );

		if( nCharLen > nLength )
		{
			break;
		}
		Glyph *pcGlyph = GetGlyph( FT_Get_Char_Index( m_pcFont->GetTTFace(), utf8_to_unicode( pzString ) ) );

		pzString += nCharLen;
		nLength -= nCharLen;
		if( pcGlyph == NULL )
		{
			dbprintf( "Error: GetStringWidth() failed to load glyph\n" );
			continue;
		}
		nWidth += pcGlyph->m_nAdvance.x;
	}
	g_cFontLock.Unlock();
	return ( nWidth );
}

IPoint SFontInstance::GetTextExtent( const char *pzString, int nLength, uint32 nFlags, int nTargetWidth )
{
	IPoint cExt( 0, m_nAscender - m_nDescender );

	if( ( nFlags & DTF_WRAP_SOFT ) && ( nTargetWidth <= 0 ) )
		return cExt;

	g_cFontLock.Lock();

	// Count rows.
	int i, nCharLength = 0;
	const char *p = pzString;
	std::list<int> vRowLens;

	if( nFlags & DTF_WRAP_SOFT )
	{
		int nWordWidth = 0, nLineWidth = 0;	// Width of word/line in pixels
		int nWordLength = 0, nLineLength = 0;	// Length of word/line in bytes

		for( i = nLength; i > 0; p += nCharLength )
		{
			// Get char length
			nCharLength = utf8_char_length( *p );
			nWordLength += nCharLength;
			i -= nCharLength;

			// Get glyph width
			Glyph *pcGlyph = GetGlyph( FT_Get_Char_Index( m_pcFont->GetTTFace(), utf8_to_unicode( p ) ) );
			if( NULL == pcGlyph )
				continue;
			nWordWidth += pcGlyph->m_nAdvance.x;

			if( *p == ' ' || *p == '\t' || *p == '\n' )
			{
				if( nLineWidth + nWordWidth > nTargetWidth )
				{
					// Start a new line before this word
					vRowLens.push_back( nLineLength );
					nLineWidth = nWordWidth;
					nLineLength = nWordLength;
				}
				else
				{
					// Continue the current line
					nLineWidth += nWordWidth;
					nLineLength += nWordLength;

					if( *p == '\n' )
					{
						// Start a newline after this word
						vRowLens.push_back( nLineLength );
						nLineWidth = nLineLength = 0;
					}
				}
				nWordWidth = nWordLength = 0;
			}
		}

		// Push back the last line.
		vRowLens.push_back( nLineLength );
	}
	else
		vRowLens.push_back( nLength );

	int nOffset = 0, nLineExtent;
	std::list<int>::iterator l;
	for( l = vRowLens.begin(); l != vRowLens.end(); l++ )
	{
		int nLineLength = (*l);
		const char *pzLine = pzString + nOffset;
		nLineExtent = 0;

		while( nLineLength > 0 )
		{
			if( !( nFlags & DTF_IGNORE_FMT ) )
			{
				bool bDone;

				do
				{
					bDone = false;
					switch ( *pzLine )
					{
						case '_':
						{
							if( !( nFlags & DTF_UNDERLINES ) )
								bDone = true;
							else
							{
								pzLine++;
								nLineLength--;
							}
							break;
						}

						case '\n':
						{
							if( !( nFlags & DTF_WRAP_SOFT ) )
							{
								cExt.y += m_nAscender - m_nDescender + m_nLineGap;
								if( nLineExtent > cExt.x )
									cExt.x = nLineExtent;
								nLineExtent = 0;
							}
							pzLine++;
							nLineLength--;
							break;
						}

						case '\t':
						{
							int nSkip = TAB_STOP - int( cExt.x ) % TAB_STOP;
							if( nSkip < 2 )
								nSkip = TAB_STOP;
							cExt.x += nSkip;

							pzLine++;
							nLineLength--;
							break;
						}

						case 27:
						{
							pzLine++;
							nLineLength--;
							if( nLineLength > 0 && *pzLine != '[' )
							{
								pzLine++;
								nLineLength--;
							}
							break;
						}

						default:
							bDone = true;
					}	// switch()
				}
				while( nLineLength > 0 && !bDone );
			}	// if()

			int nCharLen = utf8_char_length( *pzLine );
			if( nCharLen > nLineLength )
				break;

			Glyph *pcGlyph = GetGlyph( FT_Get_Char_Index( m_pcFont->GetTTFace(), utf8_to_unicode( pzLine ) ) );

			pzLine += nCharLen;
			nLineLength -= nCharLen;

			if( pcGlyph == NULL )
			{
				dbprintf( "Error: GetTextExtent() failed to load glyph\n" );
				continue;
			}
			nLineExtent += pcGlyph->m_nAdvance.x;
		}	// while()

		if( nLineExtent > cExt.x )
			cExt.x = nLineExtent;

		if( nFlags & DTF_WRAP_SOFT )
			cExt.y += m_nAscender - m_nDescender + m_nLineGap;

		nOffset += (*l);
	}	// for()

	g_cFontLock.Unlock();
	return ( cExt );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int SFontInstance::GetStringLength( const char *pzString, int nLength, int nWidth, bool bIncludeLast )
{
	int nStrLen = 0;

	g_cFontLock.Lock();
	while( nLength > 0 )
	{
		int nCharLen = utf8_char_length( *pzString );

		if( nCharLen > nLength )
		{
			break;
		}
		Glyph *pcGlyph = GetGlyph( FT_Get_Char_Index( m_pcFont->GetTTFace(), utf8_to_unicode( pzString ) ) );

		if( pcGlyph == NULL )
		{
			dbprintf( "Error: GetStringLength() failed to load glyph\n" );
			break;
		}
		if( nWidth < pcGlyph->m_nAdvance.x )
		{
			if( bIncludeLast )
			{
				nStrLen += nCharLen;
			}
			break;
		}
		pzString += nCharLen;
		nLength -= nCharLen;
		nStrLen += nCharLen;
		nWidth -= pcGlyph->m_nAdvance.x;
	}
	g_cFontLock.Unlock();
	return ( nStrLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFont::SFont( FontFamily * pcFamily, FT_Face psFace ):m_cStyle( psFace->style_name )
{
	m_pcFamily = pcFamily;
	m_psFace = psFace;

	m_bFixedWidth = ( psFace->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0;
	m_bScalable = ( psFace->face_flags & FT_FACE_FLAG_SCALABLE ) != 0;
	m_bDeleted = false;

	m_nGlyphCount = m_psFace->num_glyphs;

	for( int i = 0; i < psFace->num_fixed_sizes; ++i )
	{
		m_cBitmapSizes.push_back( float ( psFace->available_sizes[i].height ) * 72.0f / 96.0f );
	}
	pcFamily->AddFont( this );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFont::~SFont()
{
	m_pcFamily->RemoveFont( this );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

#define	CMP_FLOATS( a, b, t )	(((a) > (b) - (t)) && ((a) < (b) + (t)))

SFontInstance *SFont::FindInstance( const FontProperty & cFP ) const
{
	std::map <FontProperty, SFontInstance * >::const_iterator i;

	__assertw( g_cFontLock.IsLocked() );
	i = m_cInstances.find( cFP );

	if( i == m_cInstances.end() )
	{
		return ( NULL );
	}
	else
	{
		return ( ( *i ).second );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SFont::AddInstance( SFontInstance * pcInstance )
{
	g_cFontLock.Lock();
	__assertw( g_cFontLock.IsLocked() );
	m_cInstances[pcInstance->m_cInstanceProperties] = pcInstance;
	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SFont::RemoveInstance( SFontInstance * pcInstance )
{
	g_cFontLock.Lock();

	std::map <FontProperty, SFontInstance * >::iterator i;

	__assertw( g_cFontLock.IsLocked() );
	i = m_cInstances.find( pcInstance->m_cInstanceProperties );

	if( i != m_cInstances.end() )
	{
		m_cInstances.erase( i );
	}
	else
	{
		dbprintf( "Error: SFont::RemoveInstance could not find instance\n" );
	}
	if( m_bDeleted && m_cInstances.empty() )
	{
		dbprintf( "Last instance of deleted font %s, %s removed. Deleting font\n", m_pcFamily->GetName().c_str(  ), m_cStyle.c_str(  ) );
		delete this;
	}
	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFontInstance *SFont::OpenInstance( const FontProperty & cFP )
{
	SFontInstance *pcInstance = FindInstance( cFP );

	if( NULL == pcInstance )
	{
		pcInstance = new SFontInstance( this, cFP );

/*	dbprintf("Creating new instance for: Size: %d Shear: %d Rot: %d Flg: %x\n",
		(int)cFP.m_nPointSize, (int)cFP.m_nShear, (int)cFP.m_nRotation, (int)cFP.m_nFlags);*/

		std::map <FontProperty, SFontInstance * >::iterator i;

		for( i = m_cInstances.begin(); i != m_cInstances.end(  ); i++ )
		{

/*		dbprintf("Size: %d Shear: %d Rot: %d Flg: %x\n",
			(int)(*i).first.m_nPointSize,
			(int)(*i).first.m_nShear,
			(int)(*i).first.m_nRotation,
			(int)(*i).first.m_nFlags);*/
		}

	}
	else
	{

/*	dbprintf("OpenInstance: %d %d %d %x == %d %d %d %x\n",
		(int)cFP.m_nPointSize, (int)cFP.m_nShear, (int)cFP.m_nRotation, (int)cFP.m_nFlags,
		(int)pcInstance->m_cInstanceProperties.m_nPointSize,
		(int)pcInstance->m_cInstanceProperties.m_nShear,
		(int)pcInstance->m_cInstanceProperties.m_nRotation,
		(int)pcInstance->m_cInstanceProperties.m_nFlags );*/

		pcInstance->AddRef();
	}

	return ( pcInstance );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int SFont::CharToUnicode( uint nChar )
{
	int nIndex = FT_Get_Char_Index( m_psFace, nChar );

	return ( nIndex );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void pathcat( char *pzPath, const char *pzName )
{
	int nPathLen = strlen( pzPath );

	if( nPathLen > 0 )
	{
		if( pzPath[nPathLen - 1] != '/' )
		{
			pzPath[nPathLen] = '/';
			nPathLen++;
		}
	}
	strcpy( pzPath + nPathLen, pzName );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontFamily::FontFamily( const char *pzName ):m_cName( pzName )
{
}

FontFamily::~FontFamily()
{
	FontServer::GetInstance()->RemoveFamily( this );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontFamily::AddFont( SFont * pcFont )
{
	m_cFonts[pcFont->GetStyle()] = pcFont;
}

void FontFamily::RemoveFont( SFont * pcFont )
{
	std::map <std::string, SFont * >::iterator i;

	i = m_cFonts.find( pcFont->GetStyle() );
	if( i == m_cFonts.end() )
	{
		dbprintf( "Error: FontFamily::RemoveFont() could not find style '%s' in family '%s'\n", pcFont->GetStyle().c_str(  ), m_cName.c_str(  ) );
		return;
	}
	m_cFonts.erase( i );
	if( m_cFonts.empty() )
	{
		dbprintf( "Font family '%s' is empty. Removing.\n", m_cName.c_str() );
		delete this;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFont *FontFamily::FindStyle( const std::string & pzStyle )
{
	std::map <std::string, SFont * >::iterator i;

	i = m_cFonts.find( pzStyle );

	if( i == m_cFonts.end() )
	{
		return ( NULL );
	}
	else
	{
		return ( ( *i ).second );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontServer::FontServer()
{
	assert( s_pcInstance == NULL );

	s_pcInstance = this;

	FT_Error nError;

	nError = FT_Init_FreeType( &m_hFTLib );
	if( nError != 0 )
	{
		dbprintf( "ERROR: While initializing font renderer, code = %d\n", nError );
	}

	ScanDirectory( "/system/fonts/" );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontServer::~FontServer()
{
	FT_Done_FreeType( m_hFTLib );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool FontServer::Lock()
{
	return ( g_cFontLock.Lock() == 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontServer::Unlock()
{
	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int FontServer::GetFamilyCount() const
{
	int nCount;

	g_cFontLock.Lock();
	nCount = m_cFamilies.size();
	g_cFontLock.Unlock();
	return ( nCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int FontServer::GetFamily( int nIndex, char *pzFamily, uint32 *pnFlags )
{
	int nError = EINVAL;

	g_cFontLock.Lock();
	if( nIndex < int ( m_cFamilies.size() ) )
	{
		std::map <std::string, FontFamily * >::const_iterator i = m_cFamilies.begin();

		while( nIndex-- > 0 )
			++i;

		strcpy( pzFamily, ( *i ).first.c_str() );
		nError = 0;
	}
	g_cFontLock.Unlock();
	return ( nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int FontServer::GetStyleCount( const std::string & cFamily ) const
{
	int nCount = -1;

	g_cFontLock.Lock();

	FontFamily *pcFamily = FindFamily( cFamily );

	if( pcFamily != NULL )
	{
		nCount = pcFamily->m_cFonts.size();
	}

	g_cFontLock.Unlock();
	return ( nCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int FontServer::GetStyle( const std::string & cFamily, int nIndex, char *pzStyle, uint32 *pnFlags ) const
{
	int nError = 0;

	g_cFontLock.Lock();

	FontFamily *pcFamily = FindFamily( cFamily );

	if( pcFamily != NULL )
	{
		if( nIndex < int ( pcFamily->m_cFonts.size() ) )
		{
			std::map <std::string, SFont * >::const_iterator i = pcFamily->m_cFonts.begin();

			while( nIndex-- > 0 )
				++i;

			strcpy( pzStyle, ( *i ).first.c_str() );
			*pnFlags = 0;
			if( ( *i ).second->IsFixedWidth() )
			{
				*pnFlags |= FONT_IS_FIXED;
			}
			if( ( *i ).second->IsScalable() )
			{
				*pnFlags |= FONT_IS_SCALABLE;
				if( ( *i ).second->GetBitmapSizes().size(  ) > 0 )
				{
					*pnFlags |= FONT_HAS_TUNED_SIZES;
				}
			}
			else
			{
				*pnFlags |= FONT_HAS_TUNED_SIZES;
			}
			nError = 0;
		}
		else
		{
			nError = EINVAL;
		}
	}
	else
	{
		nError = ENOENT;
	}
	g_cFontLock.Unlock();

	return ( nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontFamily *FontServer::FindFamily( const std::string & cName ) const
{
	std::map <std::string, FontFamily * >::const_iterator i;

	i = m_cFamilies.find( cName );

	if( i == m_cFamilies.end() )
	{
		return ( NULL );
	}
	else
	{
		return ( ( *i ).second );
	}
}

void FontServer::RemoveFamily( FontFamily * pcFamily )
{
	std::map <std::string, FontFamily * >::iterator i;

	i = m_cFamilies.find( pcFamily->GetName() );

	if( i == m_cFamilies.end() )
	{
		dbprintf( "Error: FontServer::RemoveFamily() could not find family '%s'\n", pcFamily->GetName().c_str(  ) );
		return;
	}
	m_cFamilies.erase( i );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontServer::ScanDirectory( const char *pzPath )
{
	DIR *hDir;
	dirent *psEntry;

	g_cFontLock.Lock();

	std::map <std::string, FontFamily * >::iterator cFamIter;

	for( cFamIter = m_cFamilies.begin(); cFamIter != m_cFamilies.end(  ); ++cFamIter )
	{
		std::map <std::string, SFont * >::iterator cStyleIter;

		for( cStyleIter = ( *cFamIter ).second->m_cFonts.begin(); cStyleIter != ( *cFamIter ).second->m_cFonts.end(  ); ++cStyleIter )
		{
			( *cStyleIter ).second->SetDeletedFlag( true );
		}
	}

	if( ( hDir = opendir( pzPath ) ) )
	{
		int nCount = 0;

		while( ( psEntry = readdir( hDir ) ) )
		{
			FT_Face psFace;
			FT_Error nError;
			char zFullPath[PATH_MAX];

			if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			strcpy( zFullPath, pzPath );
			pathcat( zFullPath, psEntry->d_name );

			nError = FT_New_Face( m_hFTLib, zFullPath, 0, &psFace );

			if( nError != 0 )
			{
				continue;
			}

			FT_CharMap psCharMap;
			int i;

			for( i = 0; i < psFace->num_charmaps; i++ )
			{
				psCharMap = psFace->charmaps[i];
				if( psCharMap->platform_id == 3 && psCharMap->encoding_id == 1 )
				{	// Windows unicode
					goto found;
				}
			}
			for( i = 0; i < psFace->num_charmaps; i++ )
			{
				psCharMap = psFace->charmaps[i];
				if( psCharMap->platform_id == 1 && psCharMap->encoding_id == 0 )
				{	// Apple unicode
					goto found;
				}
			}

			for( i = 0; i < psFace->num_charmaps; i++ )
			{
				psCharMap = psFace->charmaps[i];
				if( psCharMap->platform_id == 3 && psCharMap->encoding_id == 0 )
				{	// Windows symbol
					goto found;
				}
			}

			for( i = 0; i < psFace->num_charmaps; i++ )
			{
				psCharMap = psFace->charmaps[i];
				if( psCharMap->platform_id == 0 && psCharMap->encoding_id == 0 )
				{	// Apple roman
					goto found;
				}
			}

			for( i = 0; i < psFace->num_charmaps; i++ )
			{
				psCharMap = psFace->charmaps[i];
				dbprintf( "platform=%d, encoding=%d\n", psCharMap->platform_id, psCharMap->encoding_id );
			}

			FT_Done_Face( psFace );
			dbprintf( "Error: failed to find character map\n" );
			continue;
		      found:
			psFace->charmap = psCharMap;

			FontFamily *pcFamily = FindFamily( psFace->family_name );

			if( NULL == pcFamily )
			{
				try
				{
					pcFamily = new FontFamily( psFace->family_name );
				}
				catch( ... )
				{
					continue;
				}
				m_cFamilies[psFace->family_name] = pcFamily;
			}

			SFont *pcFont = pcFamily->FindStyle( psFace->style_name );

			if( pcFont != NULL )
			{
				pcFont->SetDeletedFlag( false );
				FT_Done_Face( psFace );
				continue;
			}
			else
			{
				try
				{
					pcFont = new SFont( pcFamily, psFace );
				}
				catch( ... )
				{
					continue;
				}
			}
			__assertw( NULL != pcFont );

#if 0
			dbprintf( "Font : '%s'-'%s' (%d) added\n", psFace->family_name, psFace->style_name, psFace->num_fixed_sizes );
			if( psFace->num_fixed_sizes > 0 )
			{
				for( int j = 0; j < psFace->num_fixed_sizes; ++j )
				{
					dbprintf( "  Size %d : %dx%d\n", j, psFace->available_sizes[j].width, psFace->available_sizes[j].height );
				}
			}
#endif
			nCount++;
		}
		dbprintf( "Directory '%s' scanned %d fonts found\n", pzPath, nCount );
		closedir( hDir );
	}
      restart:
	for( cFamIter = m_cFamilies.begin(); cFamIter != m_cFamilies.end(  ); ++cFamIter )
	{
		std::map <std::string, SFont * >::iterator cStyleIter;

		for( cStyleIter = ( *cFamIter ).second->m_cFonts.begin(); cStyleIter != ( *cFamIter ).second->m_cFonts.end(  ); ++cStyleIter )
		{
			SFont *pcStyle = ( *cStyleIter ).second;

			if( pcStyle->IsDeleted() && pcStyle->GetInstanceCount(  ) == 0 )
			{
				dbprintf( "Deleting font %s:%s\n", pcStyle->GetFamily()->GetName(  ).c_str(  ), pcStyle->GetStyle(  ).c_str(  ) );
				delete pcStyle;

				goto restart;
			}
		}
	}

	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFontInstance *FontServer::OpenInstance( const std::string & cFamily, const std::string & cStyle, const FontProperty & cFP )
{
	FontFamily *pcFamily;

	g_cFontLock.Lock();

	if( ( pcFamily = FindFamily( cFamily ) ) )
	{
		SFont *pcFont = pcFamily->FindStyle( cStyle );

		if( pcFont != NULL )
		{
			SFontInstance *pcInstance = pcFont->OpenInstance( cFP );

			if( pcInstance != NULL )
			{
				g_cFontLock.Unlock();
				return ( pcInstance );
			}
		}
	}
	g_cFontLock.Unlock();
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SFont *FontServer::OpenFont( const std::string & cFamily, const std::string & cStyle )
{
	FontFamily *pcFamily;
	SFont *pcFont = NULL;

	if( Lock() )
	{
		if( ( pcFamily = FindFamily( cFamily ) ) )
		{
			pcFont = pcFamily->FindStyle( cStyle );
		}
		Unlock();
	}
	return ( pcFont );
}



