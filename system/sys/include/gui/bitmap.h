/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef	__F_GUI_BITMAP_H__
#define	__F_GUI_BITMAP_H__

#include <atheos/types.h>
#include <gui/guidefines.h>
#include <gui/region.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Window;
class View;

/** Container for bitmap-image data.
 * \ingroup gui
 * \par Description:
 *	The Bitmap class make it possible to render bitmap graphics
 *	into view's and to make view's render into an offscreen buffer
 *	to implement things like double-buffering.
 *
 *	The bitmap class have two different ways to communicate with
 *	the application server. If the \b SHARE_FRAMEBUFFER flag is
 *	set the bitmaps raster memory is created in a memory-area
 *	shared between the application and the appserver. This makes
 *	it possible for the appserver to blit graphics written
 *	directly to the bitmaps raster buffer by the application into
 *	views on the screen (or inside other bitmaps). If the
 *	\b ACCEPT_VIEWS flag is set the bitmap will accept views to be
 *	added much like a os::Window object. All rendering performed
 *	by the views will then go into the bitmap's offscreen buffer
 *	rather than the screen. The rendered image can then be read
 *	out by the application (requiers the \b SHARE_FRAMEBUFFER flag
 *	to be set aswell) or it can be blited into other views. The
 *	\b NO_ALPHA_CHANNEL flag disables the alpha channel for 32 bit
 *	bitmaps. This can improve performance if you render to this bitmap.
 *
 * \note In most cases, the SHARE_FRAMEBUFFER flag should be used.
 *
 * \sa View, Window
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Bitmap
{
public:
    enum { ACCEPT_VIEWS = 0x0001, SHARE_FRAMEBUFFER = 0x0002, NO_ALPHA_CHANNEL = 0x0004 };
    Bitmap( int nWidth, int nHeight, color_space eColorSpc, uint32 nFlags = SHARE_FRAMEBUFFER );
    Bitmap( int hHandle );
    virtual ~Bitmap();

    bool		IsValid( void ) const;
	int			GetHandle( void ) const;
    color_space	GetColorSpace() const;
    Rect		GetBounds( void ) const;
    int		GetBytesPerRow() const;
    virtual void	AddChild( View* pcView );
    virtual bool	RemoveChild( View* pcView );
    View*		FindView( const char* pzName ) const;

    void		Sync( void );
    void		Flush( void );

/**
 * Get a pointer to the raw raster data.
 * \par Description:
 *    This method gets a void* pointer to the raw raster data.
 *    The size of the raster data in bytes can be calculated by GetBytesPerLine()*(GetBounds().Height()+1).
 *    Since the raster data is allocated by the appserver, this method is only valid if the bitmap
 *    was created with the flag SHARE_FRAMEBUFFER.
 * \note Remember to call UnlockRaster() when finished accessing the raster.
 * \retval Returns a pointer to the raster data, or NULL if the bitmap was not created with the SHARE_FRAMEBUFFER flag.
 * \sa UnlockRaster()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
    uint8*	LockRaster( void ) { return( m_pRaster ); }
    void		UnlockRaster() {}

private:
    friend class View;
    friend class Window;
    friend class Sprite;
    friend class Desktop;

    Bitmap& operator=( const Bitmap& );
    Bitmap( const Bitmap& );
  
    Window* 	m_pcWindow;
    int	  	m_hHandle;
    area_id	m_hArea;
    color_space	m_eColorSpace;
    Rect	  	m_cBounds;
    uint8*  	m_pRaster;
};



inline int BitsPerPixel( color_space eColorSpc )
{
    switch( eColorSpc )
    {
	case CS_RGB32:
	case CS_RGBA32:
	    return( 32 );
	case CS_RGB24:
	    return( 24 );
	case CS_RGB16:
	case CS_RGB15:
	case CS_RGBA15:
	    return( 16 );
	case CS_CMAP8:
	case CS_GRAY8:
	    return( 8 );
	case CS_GRAY1:
	    return( 1 );
/*      
	case CS_YUV422:
	case CS_YUV411:
	case CS_YUV420:
	return( 8 );
	case CS_YUV444:
	return( 16 );
	case CS_YUV9:
	case CS_YUV12:
	*/
	default:
	    dbprintf( "BitsPerPixel() invalid color space %d\n", eColorSpc );
	    return( 8 );
    }
}


extern const uint8 __5_to_8_bit_table[];
extern const uint8 __6_to_8_bit_table[];

inline uint32 COL_TO_RGB32( const Color32_s& col ) {
    return( ( ((col).red << 16) | ((col).green << 8) | (col).blue | ((col).alpha << 24) ) );
}

inline uint16 COL_TO_RGB16( const Color32_s& col ) {
    return( ( (((col).red >> 3) << 11) | (((col).green >> 2) << 5) | ((col).blue >> 3) ) );
}

inline uint16 COL_TO_RGB15( const Color32_s& col ) {
    return( ( (((col).red >> 3) << 10) | (((col).green >> 3) << 5) | ((col).blue >> 3) ) );
}

inline Color32_s RGB32_TO_COL( uint32 pix ) {
    return( Color32_s( ((pix) >> 16) & 0xff, ((pix) >> 8) & 0xff, (pix) & 0xff, ((pix) >> 24) & 0xff ) );
}

inline Color32_s RGB16_TO_COL( uint16 pix ) {
    return( Color32_s( __5_to_8_bit_table[ ((pix) >> 11) & 0x1f ],
		       __6_to_8_bit_table[ ((pix) >> 5) & 0x3f ],
		       __5_to_8_bit_table[ (pix) & 0x1f ], 255 ) );
}

inline Color32_s RGB15_TO_COL( uint16 pix ) {
    return( Color32_s( __5_to_8_bit_table[ ((pix) >> 10) & 0x1f ],
		       __5_to_8_bit_table[ ((pix) >> 5) & 0x1f ],
		       __5_to_8_bit_table[ (pix) & 0x1f ], 255 ) );
}

inline Color32_s RGBA15_TO_COL( uint16 pix ) {
    return( Color32_s( __5_to_8_bit_table[ ((pix) >> 10) & 0x1f ],
		       __5_to_8_bit_table[ ((pix) >> 5) & 0x1f ],
		       __5_to_8_bit_table[ (pix) & 0x1f ],
		       ((pix) & 0x8000) ? 0x00 : 0xff ) );
}


}

#endif // __F_GUI_BITMAP_H__


