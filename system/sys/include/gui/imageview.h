/*  ImageView - View class for images
 *  Copyright (C) 2002 Henrik Isaksson
 *  Copyright (C) 2003 - 3004 Syllable Team
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

#ifndef __F_GUI_IMAGEVIEW_H__
#define __F_GUI_IMAGEVIEW_H__

#include <gui/button.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/font.h>

#include <gui/image.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** View class for Image objects.
 * \ingroup gui
 * \par Description:
 *	The ImageView class allows you to display Image objects of any kind
 *	on screen. Simply attach an Image object to the ImageView, and attach
 *	the ImageView to a Window.
 * \note ImageView does not delete the delete the image, so you must delete it
 * yourself. This also means that it is possible to display the same Image in two
 * or more ImageViews, but care should be taken not to share the same image
 * between multiple threads.
 *
 * \sa os::Image, os::BitmapImage, os::VectorImage, os::View
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class ImageView : public View
{
	public:
	enum ImageMode {
		FIXED_SIZE = 0x10000000,
		NORMAL     = 0x00000000 | FIXED_SIZE,
		TILE       = 0x00000001,
		STRETCH    = 0x00000002,
		ALPHA      = 0x00001000,
		DEFAULT    = NORMAL | ALPHA
		};

	public:
	ImageView( Rect cFrame, const String& cName, Image *pzImage, ImageMode eMode = DEFAULT,
	             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
	~ImageView();

	void Paint( const Rect &cUpdateRect );
	Point GetPreferredSize( bool bLargest ) const;

	void SetImage( Image *pcImage );
	Image *GetImage( void ) const;

	void Refresh();

	private:
    virtual void	__IV_reserved1__();
    virtual void	__IV_reserved2__();
    virtual void	__IV_reserved3__();
    virtual void	__IV_reserved4__();
    virtual void	__IV_reserved5__();

	private:
	ImageView& operator=( const ImageView& );
	ImageView( const ImageView& );

	class Private;
	Private*	m;
};

}

#endif /* __F_GUI_IMAGEVIEW_H__ */
