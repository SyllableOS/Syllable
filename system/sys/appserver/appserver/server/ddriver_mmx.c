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

#define PREFETCH "/nop"
#define PREFETCHW "/nop"
#define MOVNTQ "movq"
#define SFENCE "/nop"
#define EMMS "emms"

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
		__asm__ __volatile__( "movq (%0), %%mm0\n" "movq %%mm0, (%1)\n"::"r"( pFrom ), "r"( pTo ):"memory" );

		pFrom += 8;
		pTo += 8;
	}

	if ( nLen & 7 )
		memcpy( pTo, pFrom, nLen & 7 );
}

static inline void mmx_revcpy( uint8 *pTo, uint8 *pFrom, int nLen )
{
	int i;

	for ( i = 0; i < nLen / 8; i++ )
	{
		__asm__ __volatile__( "movq (%0), %%mm0\n" "movq %%mm0, (%1)\n"::"r"( pFrom ), "r"( pTo ):"memory" );

		pFrom -= 8;
		pTo -= 8;
	}

	if ( nLen & 7 )
		dbprintf( "Error: mmx_revcpy() called with non-aligned length\n" );
}


static inline void mmx_end()
{
	__asm __volatile( SFENCE:::"memory" );
	__asm __volatile( EMMS:::"memory" );
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
	__asm __volatile( PREFETCH "%0	"::"m"( *pS ):"memory" );
	__asm __volatile( "movq	%0, %%mm7\n\t" "movq	%1, %%mm6\n\t"::"m"( red_32to16mask ), "m"( green_32to16mask ) );

	pMMXend = pEnd - 15;
	while ( pS < pMMXend )
	{
		__asm __volatile( PREFETCH " 32%1\n\t" "movd	%1, %%mm0\n\t" "movd	4%1, %%mm3\n\t" "punpckldq 8%1, %%mm0\n\t" "punpckldq 12%1, %%mm3\n\t" "movq	%%mm0, %%mm1\n\t" "movq	%%mm0, %%mm2\n\t" "movq	%%mm3, %%mm4\n\t" "movq	%%mm3, %%mm5\n\t" "psrlq	$3, %%mm0\n\t" "psrlq	$3, %%mm3\n\t" "pand	%2, %%mm0\n\t" "pand	%2, %%mm3\n\t" "psrlq	$5, %%mm1\n\t" "psrlq	$5, %%mm4\n\t" "pand	%%mm6, %%mm1\n\t" "pand	%%mm6, %%mm4\n\t" "psrlq	$8, %%mm2\n\t" "psrlq	$8, %%mm5\n\t" "pand	%%mm7, %%mm2\n\t" "pand	%%mm7, %%mm5\n\t" "por	%%mm1, %%mm0\n\t" "por	%%mm4, %%mm3\n\t" "por	%%mm2, %%mm0\n\t" "por	%%mm5, %%mm3\n\t" "psllq	$16, %%mm3\n\t" "por	%%mm3, %%mm0\n\t" MOVNTQ "	%%mm0, %0\n\t":"=m"( *pD ):"m"( *pS ), "m"( blue_32to16mask ):"memory" );

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

	uint8 *pD = ( uint8 * )pDst;

	pEnd = pS + nPixels;
	__asm __volatile( PREFETCH "%0	"::"m"( *pS ):"memory" );
	__asm __volatile( "pxor	%%mm7,%%mm7\n\t":::"memory" );

	pMMXend = pEnd - 3;
	while ( pS < pMMXend )
	{
		__asm __volatile( PREFETCH " 32%1\n\t" "movq	%1, %%mm0\n\t" "movq	%1, %%mm1\n\t" "movq	%1, %%mm2\n\t" "pand	%2, %%mm0\n\t" "pand	%3, %%mm1\n\t" "pand	%4, %%mm2\n\t" "psllq	$3, %%mm0\n\t" "psrlq	$3, %%mm1\n\t" "psrlq	$8, %%mm2\n\t" "movq	%%mm0, %%mm3\n\t" "movq	%%mm1, %%mm4\n\t" "movq	%%mm2, %%mm5\n\t" "punpcklwd %%mm7, %%mm0\n\t" "punpcklwd %%mm7, %%mm1\n\t" "punpcklwd %%mm7, %%mm2\n\t" "punpckhwd %%mm7, %%mm3\n\t" "punpckhwd %%mm7, %%mm4\n\t" "punpckhwd %%mm7, %%mm5\n\t" "psllq	$8, %%mm1\n\t" "psllq	$16, %%mm2\n\t" "por	%%mm1, %%mm0\n\t" "por	%%mm2, %%mm0\n\t" "psllq	$8, %%mm4\n\t" "psllq	$16, %%mm5\n\t" "por	%%mm4, %%mm3\n\t" "por	%%mm5, %%mm3\n\t" MOVNTQ "	%%mm0, %0\n\t" MOVNTQ "	%%mm3, 8%0\n\t":"=m"( *pD ):"m"( *pS ), "m"( blue_16to32mask ), "m"( green_16to32mask ), "m"( red_16to32mask ):"memory" );

		pD += 16;
		pS += 4;
	}

	while ( pS < pEnd )
	{
		register uint16 nBGR = *pS++;

		*pD++ = ( nBGR & 0x1F ) << 3;
		*pD++ = ( nBGR & 0x7E0 ) >> 3;
		*pD++ = ( nBGR & 0xF800 ) >> 8;
		*pD++ = 0;
	}
}
