/*  zoomview.cpp - A framebuffer zoom component for AtheOS
 *  Based on AZoom sources, by Chir.
 *  Copyright (C) 2002 digitaliz
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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


#include "zoomview.h"

#define UPDATE_INTERVAL		50000		// 50 ms

using namespace os;

// ---------------------------------------------------------------------------

ZoomView::ZoomView( Rect cFrame ) : LayoutView( cFrame, "my_view" )
{
	m_bIsFirst = true;
	m_H = 2;
	m_Bitmap = 0;
	m_MouseX = 0;
	m_MouseY = 0;
	m_Update = 3;
	m_Screen = new Desktop;
	m_ColSpace = m_Screen->GetColorSpace();
	m_ScrMode = m_Screen->GetScreenMode();
	// Note: If the screen mode changes, the program has to be restarted.
	// Putting this in the fetchPixels() method, as it was before, causes
	// too much overhead.
}

ZoomView::~ZoomView()
{
	if(m_Bitmap)
		delete m_Bitmap;
	if(m_Screen)
		delete m_Screen;
}

void ZoomView::Paint( const Rect& cUpdateRect )
{
	if(m_bIsFirst) {
		// Defered initialisation.
		m_bIsFirst = false;
		GetWindow()->AddTimer( this, 0, UPDATE_INTERVAL, false );
		resizeZoom();
	}
	drawPixels();
}

Point ZoomView::GetPreferredSize( bool bLargest )
{
	return Point(100,100);
}

void ZoomView::FrameSized( const Point& cDelta )
{
	resizeZoom();
   Invalidate(true);       // Refresh.
}

void ZoomView::TimerTick( int nID )
{
	fetchPixels();
}

/*
 * Changes the zoom level.
 */
void ZoomView::setMagnification( int nSize )
{
	m_H = nSize;
	resizeZoom();
	Invalidate(true);       // Refresh.
}

void ZoomView::MagnifyBitmap(
	uint8 *s,					// Source bitmap data
	uint32 xsstart,			// Source bitmap x start pos
	uint32 ysstart,			// Source bitmap y start pos
	uint32 ws,					// Source bitmap width (to magnify)
	uint32 hs,					// Source bitmap height (to magnify)
	uint8 zoom,				// Zoom factor
	uint32 sbytesperrow,		// Source bitmap bytes per row
	uint8 *d,					// Destination bitmap, must be zoom times
								// larger than ws x wh
	uint32 dbytesperrow,		// Destination bitmap bytes per row
	uint32 bytesperpixel)	// Bytes per pixel, both bitmaps
{
	uint32 xs, ys;

	if(bytesperpixel == 2) {
		// A special optimized version for 15 and 16 bit depths.
		// (It can probably be optimized a lot more)

		for(ys = 0; ys < hs; ys++) {
			uint16 *tmp = (uint16 *)&s[(ys+ysstart)*sbytesperrow + xsstart * 2];

			for(xs = 0; xs < ws; xs++) {
				uint16 tmpval = *tmp;	// Reading from the frame buffer is terribly
											// slow, so we have to cache this value
											// temporarily. The hardware caches doesn't
											// seem to work with gfx mem, or maybe it has
											// something to do with tmp being a pointer?

				for(uint8 yz = 0; yz < zoom; yz++) {
					uint16 *dtmp = (uint16 *)&d[zoom*ys*dbytesperrow + xs*zoom*2 + yz*dbytesperrow];
					for(uint8 xz = 0; xz < zoom; xz++) {
						*dtmp++ = tmpval;
					}
				}

				tmp ++;
			}
		}
	} else {
		// Generic version that works with most screen depths.
		for(ys = 0; ys < hs; ys++) {
			uint8 *tmp = &s[(ys+ysstart)*sbytesperrow + xsstart * bytesperpixel];

			for(xs = 0; xs < ws*bytesperpixel; xs+=bytesperpixel) {
				for(uint8 z = 0; z < bytesperpixel; z++) {
					uint8 tmpval = *tmp;
					for(uint8 yz = 0; yz < zoom; yz++) {
						for(uint8 xz = 0; xz < zoom*bytesperpixel; xz+=bytesperpixel) {
							d[yz*dbytesperrow + zoom*ys*dbytesperrow + xs*zoom + xz + z] = tmpval;
						}
					}
					tmp ++;
				}
			}
		}
	}
}

/**
 * Fills the pixel buffer, and displays it.
 */
void ZoomView::fetchPixels()
{
	Rect	bounds = GetBounds();
	int		w, h;
	int		zoom = m_H;

	uint32 nButtons;
	Point Pos;
	GetMouse( &Pos, &nButtons );

	if(nButtons == 2) {	// Zoom on RMB
		zoom+=2;
	}

	w = (int)bounds.Width()/zoom;
	h = (int)bounds.Height()/zoom;
  
	int nWidth = m_ScrMode.m_nWidth , nHeight = m_ScrMode.m_nHeight;

	if( m_bIsFirst )  return;                  // Not initialised yet.

	if(Pos.x >= 0 && Pos.x < Width() && Pos.y >= 0 && Pos.y < Height())
		return;

	Pos = ConvertToScreen(Pos);

	int nMouseX = (int)Pos.x - w/2;
	int nMouseY = (int)Pos.y - h/2;

	// Respect screen limits.
	if( nMouseX < 0 )  nMouseX = 0;
	if( nMouseX + w >= nWidth )  nMouseX = nWidth - w;
	if( nMouseY < 0 )  nMouseY = 0;
	if( nMouseY + h >= nHeight )  nMouseY = nHeight - h;

	if( nMouseX == m_MouseX &&
	    nMouseY == m_MouseY &&
	    --m_Update) {
			return;
	}

	m_Update = 15;
	m_MouseX = nMouseX;
	m_MouseY = nMouseY;

	int nDepth;

	switch(m_ColSpace) {
		case CS_CMAP8:
			nDepth = 1;
			break;
		case CS_RGB15:
		case CS_RGB16:
		case CS_RGBA15:
			nDepth = 2;
			break;
		case CS_RGB24:
			nDepth = 3;
			break;
		default:
			nDepth = 4;
			break;
	}

	if(m_Bitmap) {
		unsigned char* pBuffer = (unsigned char*)(m_Screen->GetFrameBuffer());
		uint8 *destbfr = m_Bitmap->LockRaster();

		MagnifyBitmap(pBuffer, nMouseX, nMouseY, w, h, zoom,
			m_ScrMode.m_nBytesPerLine,
			destbfr, m_Bitmap->GetBytesPerRow(), nDepth);
		m_Bitmap->UnlockRaster();
	}

    drawPixels();
}

/**
 * Displays the contents of the pixel buffer.
 */
void ZoomView::drawPixels()
{
	Rect bounds = m_Bitmap->GetBounds();
	DrawBitmap(m_Bitmap, bounds, bounds);

    Flush();        // Ensure everything gets drawn right now.
}

/*
 * Creates a new (empty) pixel buffer, and fills it.
 */
void ZoomView::resizeZoom()
{
	Rect		bounds = GetBounds();

	if(m_Bitmap) {
		// If the window is smaller, there's no need
		// to reallocate the bitmap.
		// Comment out this to save memory. (by freeing unused
		// bitmap space, though it will make resizing slower).
		if(bounds.Width() <= m_Bitmap->GetBounds().Width() &&
		   bounds.Height() <= m_Bitmap->GetBounds().Height()) {
			return;	// No reallocation necessary
		}

		delete m_Bitmap;
	}

	m_Bitmap = new Bitmap((int)(bounds.Width())+1, (int)(bounds.Height())+1, m_ColSpace, Bitmap::SHARE_FRAMEBUFFER);
	// Note: The +1's are necessary, since the Bitmap constructor creates
	// a bitmap that is 1 pixel smaller in both directions, for some reason.
	// (Bitmap->GetBounds().Width() returns 1 less than the width you ask
	// for, same with ...Height()).

	fetchPixels();
}

// ---------------------------------------------------------------------------
