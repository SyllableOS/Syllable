/*  image.h - baseclass for image objects (bitmaps, vectors etc.)
 *  Copyright (C) 2002 Henrik Isaksson
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#ifndef	__F_GUI_IMAGE_H__
#define	__F_GUI_IMAGE_H__

#include <atheos/types.h>
#include <gui/guidefines.h>
#include <gui/region.h>
#include <gui/bitmap.h>
#include <util/message.h>
#include <storage/streamableio.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Container for image data.
 * \ingroup gui
 * \par Description:
 *	The Image class defines the interface to image classes, ie.
 *	classes that contain image data, be it in the form of bitmaps
 *	or vectors.
 *
 * \sa os::BitmapImage, os::VectorImage, os::Bitmap, os::ImageView
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class Image
{
public:
    /** Filter types
     * \par Description:
     *      These values are used to specify to ApplyFilter() which filter to 
     *      apply.
     * \sa ApplyFilter() */
    enum {
    	/** Does nothing */
	F_NONE,
    	/** Creates a greyed out ("disabled") image */
	F_GRAY,
    	/** Creates a highlighted ("selected") image */
	F_HIGHLIGHT,
    	/** Converts alpha channel data to colour overlay */
	F_ALPHA_TO_OVERLAY,
	/** Adds a "glowing" outline to the image \sa GlowFilter */
	F_GLOW,
	/** Blend all pixels with a certain colour \sa ColorizeFilter */
	F_COLORIZE,
    } FilterType;

public:
    Image( );
    virtual ~Image();

	/** Run-time type checking */
    virtual const String ImageType( void ) const = 0;

	/** Check if object is initialized and ready to be used */
    virtual bool	IsValid( void ) const = 0;

    virtual status_t	Load( StreamableIO* pcSource, const String& cType = "" ) = 0;
    virtual status_t	Save( StreamableIO* pcDest, const String& cType ) = 0;

	/** Draw an image to a View
	 * \par		Description:
	 *		This method renders the image to a View object.
	 * \param	cPos Position in the destination View to draw at.
	 * \param	pcView The View to draw in.
	 * \sa os::View
	 * \author Henrik Isaksson (henrik@boing.nu)
	 *****************************************************************************/
    virtual void	Draw( const Point& cPos, View* pcView ) = 0;

	/** Draw an image to a View
	 * \par		Description:
	 *		This method renders the image to a View object.
	 * \param	cSource Rectangular region in the source Image to draw.
	 * \param	cDest Rectangular region in the destination View to draw the
	 *		image in. If the size of cDest is not equal to cSource, the
	 *		image data from cSource will be scaled to fit.
	 * \param	pcView The View to draw in.
	 * \todo	Implement scaling. Needs support from os::View.
	 * \sa os::View
	 * \author Henrik Isaksson (henrik@boing.nu)
	 *****************************************************************************/
    virtual void	Draw( const Rect& cSource, const Rect& cDest, View* pcView ) = 0;
 
    virtual status_t	SetSize( const Point& cSize );
    virtual Point	GetSize( void ) const;
    Rect		GetBounds( void ) const;

    virtual status_t	ApplyFilter( uint32 nFilterID );
    virtual status_t	ApplyFilter( const Message& cFilterData );

private:

    virtual void	__IMG_reserved1__() {}
    virtual void	__IMG_reserved2__() {}
    virtual void	__IMG_reserved3__() {}
    virtual void	__IMG_reserved4__() {}
    virtual void	__IMG_reserved5__() {}

private:
    Image& operator=( const Image& );
    Image( const Image& );

    class Private;
    Private *m;
};

/** Container for bitmap image data.
 * \ingroup gui
 * \par Description:
 *	The BitmapImage class contains image data in the form of a bitmap and
 *      implements methods for loading and saving images via Translators.
 * \par Example:
 * \code
 *	File cFile( "picture.png" );
 *	Image *pcImage;
 *	pcImage = new BitmapImage( &cFile );	// Load picture.png
 *	pcImage->GrayFilter();
 * \endcode
 *
 * \sa os::Image, os::VectorImage, os::Bitmap
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class BitmapImage : public Image
{
public:
    BitmapImage( uint32 nFlags = Bitmap::SHARE_FRAMEBUFFER );
    BitmapImage( const BitmapImage &cSource, uint32 nFlags = Bitmap::SHARE_FRAMEBUFFER );
    BitmapImage( StreamableIO* pcSource, uint32 nFlags = Bitmap::SHARE_FRAMEBUFFER );
    BitmapImage( const uint8 *pData, const IPoint &cSize, color_space eColorSpace, uint32 nFlags = Bitmap::SHARE_FRAMEBUFFER );
    virtual ~BitmapImage();

	// From Image    
    virtual const String ImageType( void ) const;

    virtual bool	IsValid( void ) const;

    virtual status_t	Load( StreamableIO* pcSource, const String& cType = "" );
    virtual status_t	Save( StreamableIO* pcDest, const String& cType );

    virtual void	Draw( const Point& cPos, View* pcView );
    virtual void	Draw( const Rect& cSource, const Rect& cDest, View* pcView );
   
    virtual status_t	SetSize( const Point& cSize );
    virtual Point	GetSize( void ) const;

	// From BitmapImage
    void			SetBitmapData( const uint8 *pData, const IPoint &cSize, color_space eColorSpace, uint32 nFlags = Bitmap::SHARE_FRAMEBUFFER );

    color_space		GetColorSpace() const;
    virtual status_t	SetColorSpace( color_space eColorSpace );
    
    virtual Bitmap*	LockBitmap( void );
    virtual void	UnlockBitmap( void );

    virtual BitmapImage& operator=( const BitmapImage& cSource );

	virtual View* GetView();
	virtual void ResizeCanvas( const Point& cSize );
	virtual uint32* operator[]( int row );
	void Sync();
	void Flush();

	// Filter functions

    virtual status_t	ApplyFilter( const Message& cFilterData );

    status_t		GrayFilter( void );
    status_t		HighlightFilter( void );
    status_t		AlphaToOverlay( uint32 cTransparentColor );
    status_t		GlowFilter( Color32_s cInnerColor, Color32_s cOuterColor, int nRadius );
    status_t		ColorizeFilter( Color32_s cColor );
private:
    class Private;
    Private *m;
};

class ColorizeFilter : public Message {
	public:
	ColorizeFilter( Color32_s cColor )
		:Message( Image::F_COLORIZE )
	{
		AddColor32( "color", cColor );
	}
};

class GlowFilter : public Message {
	public:
	GlowFilter( Color32_s cInnerColor = 0xFFFFFFFF,	Color32_s cOuterColor = 0x7FAAAA00, uint nRadius = 3 )
		:Message( Image::F_GLOW )
	{
		AddColor32( "innerColor", cInnerColor );
		AddColor32( "outerColor", cOuterColor );
		AddInt32( "radius", nRadius );
	}
};

}

#endif // __F_GUI_IMAGE_H__
