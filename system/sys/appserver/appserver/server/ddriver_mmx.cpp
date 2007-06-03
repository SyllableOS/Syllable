/*
 *  The Syllable application server
 *  MMX accelerated colorspace conversion
 *  Copyright (C) MPlayer ( www.mplayerhq.hu )
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

#include <atheos/types.h>
#include <string.h>

#include "mmx.h"


static uint64 red_32to16mask __attribute__ ( ( aligned( 8 ) ) ) = 0x0000f8000000f800ULL;
static uint64 green_32to16mask __attribute__ ( ( aligned( 8 ) ) ) = 0x000007e0000007e0ULL;
static uint64 blue_32to16mask __attribute__ ( ( aligned( 8 ) ) ) = 0x0000001f0000001fULL;
static uint64 red_16to32mask __attribute__ ( ( aligned( 8 ) ) ) = 0xF800F800F800F800ULL;
static uint64 green_16to32mask __attribute__ ( ( aligned( 8 ) ) ) = 0x07E007E007E007E0ULL;
static uint64 blue_16to32mask __attribute__ ( ( aligned( 8 ) ) ) = 0x001F001F001F001FULL;


static inline void mmx_memcpy( uint8 *pTo, uint8 *pFrom, int nLen )
{
	int i;

	for ( i = 0; i < nLen / 8; i++ )
	{
		movq_m2r( *pFrom, mm0 );
		movq_r2m( mm0, *pTo );
		pFrom += 8;
		pTo += 8;
	}

	if ( nLen & 7 )
		memcpy( pTo, pFrom, nLen & 7 );
}

static inline void mmx_revcpy( uint8 *pTo, uint8 *pFrom, int nLen )
{
	int i;

	pFrom += nLen - 8;
	pTo += nLen - 8;
	for ( i = 0; i < nLen / 8; i++ )
	{
		movq_m2r( *pFrom, mm0 );
		movq_r2m( mm0, *pTo );

		pFrom -= 8;
		pTo -= 8;
	}
	
	if( nLen & 7 )
	{
		pFrom += 7;
		pTo += 7;
		for( i = 0; i < ( nLen & 7 ); i++ )
		{
			*pTo-- = *pFrom--;
		}
	}
}


/** Convert from RGB32 to RGB16 with MMX acceleration.
 * \par Description:
 * Converts one line of pixels.
 * \par
 * \param pSrc - Source pointer.
 * \param pDst - Dest. pointer.
 * \param nPixels - Number of pixels
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de ) / MPlayer (www.mplayerhq.hu )
 *****************************************************************************/

static inline void mmx_rgb32_to_rgb16( uint8 *pSrc, uint8 *pDst, unsigned nPixels )
{
	const uint8 *pS = pSrc;
	const uint8 *pEnd;
	const uint8 *pMMXend;

	uint16 *pD = ( uint16 * )pDst;

	pEnd = pS + nPixels * 4;
	
	/* Load masks */
	movq_m2r( red_32to16mask, mm7 );
	movq_m2r( green_32to16mask, mm6 );

	pMMXend = pEnd - 15;
	while ( pS < pMMXend )
	{
		/* Convert 4 pixels */
		movd_m2r( *pS, mm0 ); /* mm0: 0000 ARGB */
		movd_m2r( *( pS + 4 ), mm3 ); /* mm3: 0000 ARGB */
		punpckldq_m2r( *( pS + 8 ), mm0 ); /* mm0: ARGB ARGB */
		punpckldq_m2r( *( pS + 12 ), mm3 ); /* mm3: ARGB ARGB */
		movq_r2r( mm0, mm1 ); /* mm1: mm0 */
		movq_r2r( mm0, mm2 ); /* mm2: mm0 */
		movq_r2r( mm3, mm4 ); /* mm4: mm3 */
		movq_r2r( mm3, mm5 ); /* mm5: mm3 */
		psrlq_i2r( 3, mm0 );
		psrlq_i2r( 3, mm3 );
		pand_m2r( blue_32to16mask, mm0 ); /* mm0: 000B+ 000B+ */
		pand_m2r( blue_32to16mask, mm3 ); /* mm3: 000B+ 000B+ */
		psrlq_i2r( 5, mm1 );
		psrlq_i2r( 5, mm4 );
		pand_r2r( mm6, mm1 ); /* mm1: 00GG+ 00GG+ */
		pand_r2r( mm6, mm4 ); /* mm4: 00GG+ 00GG+ */
		psrlq_i2r( 8, mm2 );
		psrlq_i2r( 8, mm5 );
		pand_r2r( mm7, mm2 ); /* mm2: 00R+0 00R+0 */
		pand_r2r( mm7, mm5 ); /* mm5: 00R+0 00R+0 */
		por_r2r( mm1, mm0 ); /* mm0: 00GB+ 00GB+ */
		por_r2r( mm4, mm3 ); /* mm3: 00GB+ 00GB+ */
		por_r2r( mm2, mm0 ); /* mm0: 00RGB+ 00RGB+ */
		por_r2r( mm5, mm3 ); /* mm3: 00RGB+ 00RGB+ */
		psllq_i2r( 16, mm3 ); /* mm3: RGB+00 RGB+00 */
		por_r2r( mm3, mm0 ); /* mm0: RGB+RGB+ RGB+RGB+ */
		movq_r2m( mm0, *pD );
		pD += 4;
		pS += 16;
	}

	while ( pS < pEnd )
	{
		const int nB = *pS++;
		const int nG = *pS++;
		const int nR = *pS++;

		*pD++ = ( nB >> 3 ) | ( ( nG & 0xfc ) << 3 ) | ( ( nR & 0xf8 ) << 8 );
		pS++;
	}
}


/** Convert from RGB16 to RGB32 with MMX acceleration.
 * \par Description:
 * Converts one line of pixels.
 * \par
 * \param pSrc - Source pointer.
 * \param pDst - Dest. pointer.
 * \param nPixels - Number of pixels
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de ) / MPlayer (www.mplayerhq.hu )
 *****************************************************************************/

static inline void mmx_rgb16_to_rgb32( uint8 *pSrc, uint8 *pDst, unsigned nPixels )
{
	const uint16 *pS = ( uint16 * )pSrc;
	const uint16 *pEnd;
	const uint16 *pMMXend;
	uint8 nMask[8];

	uint8 *pD = ( uint8 * )pDst;

	pEnd = pS + nPixels;
	pxor_r2r( mm7, mm7 );
	*(uint64 *)nMask = 0xFF000000FF000000ULL;
	movq_m2r( *nMask, mm6 );
	
	pMMXend = pEnd - 3;
	while ( pS < pMMXend )
	{
		/* Convert 4 pixels */
		movq_m2r( *pS, mm0 ); /* mm0: RGB+RGB+ RGB+RGB+ */
		movq_r2r( mm0, mm1 ); /* mm1: RGB+RGB+ RGB+RGB+ */
		movq_r2r( mm0, mm2 ); /* mm2: RGB+RGB+ RGB+RGB+ */
		pand_m2r( blue_16to32mask, mm0 ); /* mm0: 0B+0B+ 0B+0B+ */
		pand_m2r( green_16to32mask, mm1 ); /* mm1: GG+GG+ GG+GG+ */
		pand_m2r( red_16to32mask, mm2 ); /* mm2: R0+R0+ R0+R0+ */
		psllq_i2r( 3, mm0 ); /* mm0: 0B0B 0B0B */
		psrlq_i2r( 3, mm1 ); /* mm1: 0G0G 0G0G */
		psrlq_i2r( 8, mm2 ); /* mm2: 0R0R 0R0R */
		movq_r2r( mm0, mm3 ); /* mm3: mm0 */
		movq_r2r( mm1, mm4 ); /* mm4: mm1 */
		movq_r2r( mm2, mm5 ); /* mm5: mm2 */
		punpcklwd_r2r( mm7, mm0 ); /* mm0: 000B 000B */
		punpcklwd_r2r( mm7, mm1 ); /* mm1: 000G 000G */
		punpcklwd_r2r( mm7, mm2 ); /* mm2: 000R 000R */
		punpckhwd_r2r( mm7, mm3 ); /* mm0: 000B 000B */
		punpckhwd_r2r( mm7, mm4 ); /* mm1: 000G 000G */
		punpckhwd_r2r( mm7, mm5 ); /* mm2: 000R 000R */
		psllq_i2r( 8, mm1 ); /* mm1: 00G0 00G0 */
		psllq_i2r( 16, mm2 ); /* mm2: 0R00 0R00 */
		por_r2r( mm1, mm0 ); /* mm0: 00GB 00GB */
		por_r2r( mm2, mm0 ); /* mm0: 0RGB 0RGB */
		psllq_i2r( 8, mm4 ); /* mm4: 00G0 00G0 */
		psllq_i2r( 16, mm5 ); /* mm5: 0R00 0R00 */
		por_r2r( mm4, mm3 ); /* mm3: 00GB 00GB */
		por_r2r( mm5, mm3 ); /* mm3: 0RGB 0RGB */
		por_r2r( mm6, mm0 ); /* mm0: FRGB FRGB */
		por_r2r( mm6, mm3 ); /* mm3: FRGB FRGB */
		movq_r2m( mm0, *pD );
		movq_r2m( mm3, *(pD + 8) );
		pD += 16;
		pS += 4;
	}

	while ( pS < pEnd )
	{
		register uint16 nBGR = *pS++;

		*pD++ = ( nBGR & 0x1F ) << 3;
		*pD++ = ( nBGR & 0x7E0 ) >> 3;
		*pD++ = ( nBGR & 0xF800 ) >> 8;
		*pD++ = 0xff;
	}
}

inline void mmx_fill32( uint32 *pDst, int nMod, int W, int H, uint32 nColor )
{
	int X, Y;
	int nAlignedWidth = W / 2;
	bool bNonAlignedWidth = ( ( W % 2 ) != 0 );
	
	movd_m2r( nColor, mm1 ); /* mm1: 0000 ARGB */
	punpckldq_r2r( mm1, mm1 ); /* mm1: ARGB ARGB */
	
	for( Y = 0; Y < H; ++Y )
	{
		if( bNonAlignedWidth ) {
			for( X = 0; X < nAlignedWidth; ++X ) {
				movq_r2m( mm1, *pDst );
				pDst += 2;
			}
			movd_r2m( mm1, *pDst );			
			pDst++;			
		}
		else {
			for( X = 0; X < nAlignedWidth; ++X ) {
				movq_r2m( mm1, *pDst );
				pDst += 2;
			}
		}
		pDst += nMod;
	}
}

inline void mmx_fill_alpha32( uint32 *pDst, int nMod, int W, int H, uint32 nColor, uint32 nFlags )
{
	bool bNoAlphaChannel = nFlags & os::Bitmap::NO_ALPHA_CHANNEL;
	/* Load constants */
	uint32 nAlpha = nColor >> 24;	
	uint8 nTemp[8] = { nAlpha, 0, nAlpha, 0,
						nAlpha, 0, 0, 0 };
	movq_m2r( *nTemp, mm4 ); /* mm4: 000A 0A0A */
	*(uint64 *)nTemp = 0x0;
	movq_m2r( *nTemp, mm0 ); /* mm0: 0000 0000 */
	movd_m2r( nColor, mm5 ); /* mm5: 0000 ARGB (source) */
	punpcklbw_r2r( mm0, mm5 ); /* mm5: 0A0R 0G0B (source) */
	
	/* Fill */
	for( int y = 0; y < H; ++y )
	{
		for( int x = 0; x < W; ++x )
		{
			if( nAlpha == 0xff )
			{
				if( bNoAlphaChannel )
					*pDst = ( 0xff000000 ) | ( nColor & 0x00ffffff );
				else
					*pDst = ( *pDst & 0xff000000 ) | ( nColor & 0x00ffffff );
			}
			else
			{
				movq_r2r( mm5, mm1 ); /* mm1: 0A0R 0G0B (source) */
				movq_m2r( *pDst, mm2 ); /* mm2: 0000 ARGB (dest) */
				punpcklbw_r2r( mm0, mm2 ); /* mm2: 0A0R 0G0B (dest) */
				psubw_r2r( mm2, mm1 ); /* mm1: 0A0R 0G0B (source-dest) */
				pmullw_r2r( mm4, mm1 ); /* mm1: AARR GGBB (source-dest)*alpha */
				psrlw_i2r( 8, mm1 ); /* mm1: 0A0R 0G0B ((source-dest)*alpha>>8) */
				paddb_r2r( mm1, mm2 ); /* mm2: 0A0R 0G0B (final) */
				packuswb_r2r( mm2, mm2 );  /* mm2: 000 ARGB (final) */
				movd_r2m( mm2, *pDst );
			}
			pDst++;
		}
		pDst = pDst + nMod;
	}
}


static inline void mmx_blit_alpha32( SrvBitmap *pcDst, SrvBitmap *pcSrc, const IRect& cSrcRect, const IPoint& cDstPos )
{
	int nWidth = cSrcRect.Width() + 1;
	int nHeight = cSrcRect.Height() + 1;	
	uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	int nSrcModulo = ( pcSrc->m_nBytesPerLine - ( nWidth ) * 4 ) / 4;
	bool bNoAlphaChannel = pcDst->m_nFlags & Bitmap::NO_ALPHA_CHANNEL;
	uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	int nDstModulo = ( pcDst->m_nBytesPerLine - ( nWidth ) * 4 ) / 4;

	/* Load constants */
	mmx_t nTemp;
	nTemp.uq = 0x0;
	movq_m2r( nTemp, mm0 ); /* mm0: 0000 0000 */
	nTemp.uq = 0x0000FFFFFFFFFFFFULL;
	movq_m2r( nTemp, mm3 ); /* mm3: 00FF FFFF (dest alpha mask ) */
	uint32 nSrcColor;

	/* Blit */	
	for( int y = 0; y < nHeight; ++y )
	{
		for( int x = 0; x < nWidth; ++x )
		{
			nSrcColor = *pSrc++;
			uint32 nAlpha = nSrcColor >> 24;
								
			if( nAlpha == 0xff && bNoAlphaChannel )
				*pDst = 0xff000000 | ( nSrcColor & 0x00ffffff );
			else if( nAlpha == 0xff )
				*pDst = ( *pDst & 0xff000000 ) | ( nSrcColor & 0x00ffffff );
			else if( nAlpha != 0x00 )
			{
				movd_m2r( nSrcColor, mm1 ); /* mm1: 0000 ARGB (source) */
				movq_r2r( mm1, mm4 ); /* mm4: 0000 ARGB (source) */
				psrld_i2r( 24, mm4 ); /* mm4: 0000 000A (source alpha) */
				punpcklwd_r2r( mm4, mm4 ); /* mm4: 0000 0A0A (source alpha) */
				punpckldq_r2r( mm4, mm4 ); /* mm4: 0A0A 0A0A (source alpha) */
				pand_r2r( mm3, mm4 ); /* mm4: 000A 0A0A (source alpha) */
				punpcklbw_r2r( mm0, mm1 ); /* mm1: 0A0R 0G0B (source */

				movd_m2r( *pDst, mm2 ); /* mm2: 0000 ARGB (dest) */
				punpcklbw_r2r( mm0, mm2 ); /* mm2: 0A0R 0G0B (dest) */
		  
	        	psubw_r2r( mm2, mm1 ); /* mm1: 0A0R 0G0B (source-dest) */
				pmullw_r2r( mm4, mm1 ); /* mm1: 00RR GGBB (source-dest)*alpha */
				psrlw_i2r( 8, mm1 ); /* mm1: 000R 0G0B ((source-dest)*alpha>>8) */
				paddb_r2r( mm1, mm2 ); /* mm2: 0A0R 0G0B (final) */
				packuswb_r2r( mm2, mm2 ); /* mm2: 000 ARGB (final) */
				movd_r2m( mm2, *pDst );
	   	    }
			pDst++;
		}
		pSrc += nSrcModulo;
		pDst += nDstModulo;
	}
}


static inline void mmx_render_glyph_over32( uint32* pDst, uint8* pSrc, const int nWidth, const int nHeight, 
										const uint32 nFgColor, const int nDstModulo, const int nSrcModulo, const bool bNoAlphaChannel )
{
	/* Load constants */
	mmx_t nTemp;
	nTemp.uq = 0x0;
	movq_m2r( nTemp, mm0 ); /* mm0: 0000 0000 */
	nTemp.uq = 0x0000FFFFFFFFFFFFULL;
	movq_m2r( nTemp, mm3 ); /* mm3: 00FF FFFF (dest alpha mask ) */
	movd_m2r( nFgColor, mm5 ); /* mm5: 0000 0RGB (source) */
	punpcklbw_r2r( mm0, mm5 ); /* mm5: 000R 0G0B (source ) */

	/* Render glyph */	
	for( int y = 0; y < nHeight; ++y )
	{
		for( int x = 0; x < nWidth; ++x )
		{
			uint32 nAlpha = *pSrc++;

			if( nAlpha > 0 )
			{
				if( nAlpha == NUM_FONT_GRAYS - 1 )
				{
					if( bNoAlphaChannel )
						*pDst = 0xff000000 | ( nFgColor & 0x00ffffff );
					else {
						uint32 nDstAlpha = *pDst & 0xff000000;
						*pDst = nDstAlpha | ( nFgColor & 0x00ffffff );
					}
				}
				else
				{
					movq_r2r( mm5, mm1 ); /* mm1: 000R 0G0B (source) */
					movd_m2r( nAlpha, mm4 ); /* mm4: 0000 000A (source alpha) */
					punpcklwd_r2r( mm4, mm4 ); /* mm4: 0000 0A0A (source alpha) */
					punpckldq_r2r( mm4, mm4 ); /* mm4: 0A0A 0A0A (source alpha) */
					pand_r2r( mm3, mm4 ); /* mm4: 000A 0A0A (source alpha) */

					movd_m2r( *pDst, mm2 ); /* mm2: 0000 ARGB (dest) */
					punpcklbw_r2r( mm0, mm2 ); /* mm2: 0A0R 0G0B (dest) */
		  
		        	psubw_r2r( mm2, mm1 ); /* mm1: 0A0R 0G0B (source-dest) */
					pmullw_r2r( mm4, mm1 ); /* mm1: 00RR GGBB (source-dest)*alpha */
					psrlw_i2r( 8, mm1 ); /* mm1: 000R 0G0B ((source-dest)*alpha>>8) */
					paddb_r2r( mm1, mm2 ); /* mm2: 0A0R 0G0B (final) */
					packuswb_r2r( mm2, mm2 ); /* mm2: 000 ARGB (final) */
					movd_r2m( mm2, *pDst );
				}
	   	    }
			pDst++;
		}
		pSrc += nSrcModulo;
		pDst += nDstModulo;
	}
}








