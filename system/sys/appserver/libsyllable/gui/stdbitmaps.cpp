
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001 Kurt Skauen
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

#include <assert.h>

#include <gui/bitmap.h>
#include "stdbitmaps.h"

using namespace os;


//#define ARROW_WIDTH  7
//#define ARROW_HEIGHT 4

#define MAX_BM_SIZE (8*8)

struct BitmapDesc
{
	int w;
	int h;
	int y;
	uint8 anRaster[MAX_BM_SIZE];
};

BitmapDesc g_asBitmapDescs[] = {
	{4, 7, 0, {		// BMID_ARROW_LEFT
					0xff, 0xff, 0xff, 0x00,
					0xff, 0xff, 0x00, 0x00,
					0xff, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00,
					0xff, 0x00, 0x00, 0x00,
					0xff, 0xff, 0x00, 0x00,
			0xff, 0xff, 0xff, 0x00}},
	{4, 7, 0, {		// BMID_ARROW_RIGHT
					0x00, 0xff, 0xff, 0xff,
					0x00, 0x00, 0xff, 0xff,
					0x00, 0x00, 0x00, 0xff,
					0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0xff,
					0x00, 0x00, 0xff, 0xff,
			0x00, 0xff, 0xff, 0xff}},
	{7, 4, 0, {		// BMID_ARROW_UP
					0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
					0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff,
					0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
	{7, 4, 0, {		// BMID_ARROW_DOWN
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
					0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff,
			0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff}},
	{0, 0, 0, {0x00}},	// End marker
};



static Bitmap *g_pcBitmap = NULL;


Bitmap *os::get_std_bitmap( int nBitmap, int nColor, Rect * pcRect )
{
	assert( nBitmap >= 0 && nBitmap < BMID_COUNT );
	assert( nColor >= 0 && nColor < 3 );

	if( g_pcBitmap == NULL )
	{
		int nWidth = 0;
		int nHeight = 0;

		for( int i = 0; g_asBitmapDescs[i].w > 0; ++i )
		{
			if( g_asBitmapDescs[i].w > nWidth )
			{
				nWidth = g_asBitmapDescs[i].w;
			}
			g_asBitmapDescs[i].y = nHeight;
			nHeight += g_asBitmapDescs[i].h * 3;
		}

		g_pcBitmap = new Bitmap( nWidth, nHeight, CS_CMAP8, Bitmap::SHARE_FRAMEBUFFER );

		uint8 *pRaster = g_pcBitmap->LockRaster();
		int nPitch = g_pcBitmap->GetBytesPerRow();

		for( int i = 0; g_asBitmapDescs[i].w > 0; ++i )
		{
			int w = g_asBitmapDescs[i].w;
			int h = g_asBitmapDescs[i].h;

			assert( w * h < MAX_BM_SIZE );

			for( int y = 0; y < h; ++y )
			{
				for( int x = 0; x < w; ++x )
				{
					*pRaster++ = g_asBitmapDescs[i].anRaster[y * w + x];	// Black
				}
				pRaster += nPitch - w;
			}
			for( int y = 0; y < h; ++y )
			{
				for( int x = 0; x < w; ++x )
				{
					*pRaster++ = ( g_asBitmapDescs[i].anRaster[y * w + x] == 0xff ) ? 0xff : 14;	// Gray
				}
				pRaster += nPitch - w;
			}
			for( int y = 0; y < h; ++y )
			{
				for( int x = 0; x < w; ++x )
				{
					*pRaster++ = ( g_asBitmapDescs[i].anRaster[y * w + x] == 0xff ) ? 0xff : 63;	// White
				}
				pRaster += nPitch - w;
			}
		}
	}
//    int nIndex = nBitmap * 3 + nColor;
	int y = g_asBitmapDescs[nBitmap].y + g_asBitmapDescs[nBitmap].h * nColor;

	*pcRect = Rect( 0, y, g_asBitmapDescs[nBitmap].w - 1, y + g_asBitmapDescs[nBitmap].h - 1 );
	return ( g_pcBitmap );
}
