/*  zoomview.h - A framebuffer zoom component for AtheOS
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

#include <gui/desktop.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/bitmap.h>


using namespace os;

/**
 * Zoom component.
 */
class ZoomView : public LayoutView
{
	public:
	ZoomView( Rect cFrame );
	~ZoomView();
	void Paint( const Rect& cUpdateRect );
	virtual void TimerTick( int nID );
	virtual Point GetPreferredSize( bool bLargest );
	virtual void FrameSized( const Point& cDelta );
	void setMagnification( int );

	private:
	void drawPixels();
	void resizeZoom();
	void fetchPixels();

	void MagnifyBitmap(
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
		uint32 bytesperpixel);	// Bytes per pixel, both bitmaps


	int				m_H;					// Current zoomed pixel size.
	bool			m_bIsFirst;			// Initialisation flag.
	Bitmap			*m_Bitmap;				// Buffer for scaled bitmap
	int				m_MouseX, m_MouseY;	// Top-left of the rectangle to zoom
	int				m_Update;				// Update counter. When the mouse is
											// not in transit, the update freq is
											// divided by 15 to decrease CPU load
	Desktop		*m_Screen;				// Screen and desktop attributes,
	color_space	m_ColSpace;			// if attributes change while the
	screen_mode	m_ScrMode;				// program is still running, things
											// may go very wrong.
};


