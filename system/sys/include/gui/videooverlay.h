/*  Video Overlay class
 *  Copyright (C) 2003 Arno Klenke
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

#ifndef	__F_GUI_VIDEOOVERLAY_H__
#define	__F_GUI_VIDEOOVERLAY_H__

#include <atheos/areas.h>
#include <atheos/threads.h>
#include <gui/guidefines.h>
#include <gui/point.h>
#include <gui/rect.h>
#include <gui/gfxtypes.h>
#include <gui/view.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/* Private structure for communication with the appserver */
struct video_overlay
{
	int			m_nSrcWidth;
	int			m_nSrcHeight;
	int			m_nDstX;
	int			m_nDstY;
	int			m_nDstWidth;
	int			m_nDstHeight;
	color_space m_eFormat;
	Color32_s	m_sColorKey;
	area_id		m_hPhysArea;
	area_id		m_hArea;
	uint8*		m_pAddress;	
};


/** Video Overlay.
 * \ingroup gui
 * \par Description:
 * The Video Overlay View class can create an Overlay on the screen which is 
 * useful especially for video data. It will only work if the 
 * display driver supports video overlays. Note that to
 * update the content on the screen you have to call the Update() method.
 * After every call of Update() the memory address might also
 * have changed.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class VideoOverlayView : public View
{
public:
	VideoOverlayView( const Rect& cFrame, const String& cTitle, uint32 nResizeMask, const IPoint& cSrcSize, color_space eFormat,
					Color32_s sColorKey );
	~VideoOverlayView();
    
	void Update();
    
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Paint( const Rect& cUpdateRect );
    
	uint8* GetRaster() const;
	area_id GetPhysicalArea() const;

private:
    virtual void	__VOV_reserved1__();
    virtual void	__VOV_reserved2__();
    virtual void	__VOV_reserved3__();
    virtual void	__VOV_reserved4__();
    virtual void	__VOV_reserved5__();
private:
    VideoOverlayView& operator=( const VideoOverlayView& );
    VideoOverlayView( const VideoOverlayView& );

	void Recreate( const IPoint& cSrcSize, const IRect& cDstRect );

	class Private;
	Private *m;
};


}

#endif // __F_GUI_VIDEOOVERLAY_H__
