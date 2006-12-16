#include <gui/imageview.h>

using namespace os;

/** \internal */
class ImageView::Private
{
      public:

	Private()
	{
		m_pcImage = NULL;
	}

	~Private()
	{
		/* The user is responsible for deleting */
	}

	Image *m_pcImage;
	uint32 m_nMode;
};

/** Constructor
 * \par 	Description:
 * \param	cFrame The frame rectangle in the parents coordinate system.
 * \param	pzName The logical name of the view. This parameter is newer
 *	   	rendered anywhere, but is passed to the Handler::Handler()
 *		constructor to identify the view.
 * \param	pcImage The Image object to display.
 * \param	eMode Specifies how to render the Image (TILE, STRETCH, NORMAL).
 * \param	nResizeMask Flags defining how the views frame rectangle is
 *		affected if the parent view is resized.
 * \param	nFlags Various flags to control the views behavour.
 * \sa os::Image, os::View
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
ImageView::ImageView( Rect cFrame, const String& cName, Image * pcImage, ImageMode eMode, uint32 nResizeMask, uint32 nFlags )
:View( cFrame, cName, nResizeMask, nFlags )
{
	m = new Private;
	m->m_pcImage = pcImage;
	m->m_nMode = eMode;
}

ImageView::~ImageView()
{
	delete m;
}

/** View callback, Renders the Image
 * \par 	Description:
 *		This method is called by the OS when the ImageView needs to
 *		refresh it's imagery.
 * \param	cUpdateRect The part of the ImageView that needs refreshing.
 * \todo	STRETCH is not implemented.
 * \sa os::Image, os::View
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void ImageView::Paint( const Rect & cUpdateRect )
{
	if( !m->m_pcImage )
		return;

	Rect cBounds( GetBounds() );
	Point cImgSize( m->m_pcImage->GetSize() );

	SetDrawingMode( DM_COPY );

	SetEraseColor( get_default_color( COL_NORMAL ) );
	EraseRect( cUpdateRect );

	if( m->m_nMode & ALPHA )
	{
		SetDrawingMode( DM_BLEND );
	}
	else
	{
		SetDrawingMode( DM_OVER );
	}

	if( m->m_nMode & TILE )
	{
		Point n( cBounds.Width(), cBounds.Height(  ) );
		Point p;

		n.x /= cImgSize.x;
		n.y /= cImgSize.y;
		for( p.y = 0; p.y < n.y; p.y++ )
		{
			for( p.x = 0; p.x < n.x; p.x++ )
			{
				m->m_pcImage->Draw( Point( cImgSize.x * p.x, cImgSize.y * p.y ), this );
			}
		}
	}
	else if( m->m_nMode & STRETCH )
	{
		/* TODO: First rect should be changed to the section of the Image that will result    */
		/* in cUpdateRect when scaled according to the ration between CBounds and Image size. */
		m->m_pcImage->Draw( m->m_pcImage->GetBounds(), GetBounds(), this );
	}
	else
	{
		os::Rect cRect = cUpdateRect & m->m_pcImage->GetBounds();
		if( cRect.IsValid() )
			m->m_pcImage->Draw( cRect, cRect, this );
	}
}

/** Returns size for automatic layout
 * \par 	Description:
 *		This method is used by LayoutNode classes to calculate the
 *		size of the object. You can override this method to get a
 *		fixed-size ImageView.
 * \param	bLargest Tells whether the smallest (normal) size is requested,
 *		or the largest possible size.
 * \retval	Returns the the Image size if bLargest is false, otherwise
 *		it returns Point( COORD_MAX, COORD_MAX ).
 * \sa os::View, os::LayoutNode, os::LayoutView
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Point ImageView::GetPreferredSize( bool bLargest ) const
{
	if( !m->m_pcImage )
	{
		return Point( 1.0, 1.0 );
	}
	if( m->m_nMode & FIXED_SIZE )
	{
		return m->m_pcImage->GetSize();
	}
	else
	{
		if( bLargest )
			return Point( COORD_MAX, COORD_MAX );
		else
			return m->m_pcImage->GetSize();
	}
}

void ImageView::SetImage( Image * pcImage )
{
	m->m_pcImage = pcImage;
	Invalidate();
}

Image *ImageView::GetImage( void ) const
{
	return m->m_pcImage;
}

/** Refresh image
 * \par 	Description:
 *		This method is intended for use with double buffered animation. Call
 *		Refresh() after changing the attached image. The difference between
 *		this method and Invalidate(), is that this method will not clear the
 *		background.
 * \note Refresh() does not work with transparent bitmaps.
 * \sa os::Image, os::View
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void ImageView::Refresh()
{
	if( !m->m_pcImage )
		return;

	Rect cBounds( GetBounds() );
	Point cImgSize( m->m_pcImage->GetSize() );

	SetDrawingMode( DM_COPY );

	if( m->m_nMode & TILE )
	{
		Point n( cBounds.Width(), cBounds.Height(  ) );
		Point p;

		n.x /= cImgSize.x;
		n.y /= cImgSize.y;
		for( p.y = 0; p.y < n.y; p.y++ )
		{
			for( p.x = 0; p.x < n.x; p.x++ )
			{
				m->m_pcImage->Draw( Point( cImgSize.x * p.x, cImgSize.y * p.y ), this );
			}
		}
	}
	else if( m->m_nMode & STRETCH )
	{
		/* TODO: First rect should be changed to the section of the Image that will result    */
		/* in cUpdateRect when scaled according to the ration between CBounds and Image size. */
		/* NOTE: View->DrawBitmap() scaling must be implemented first! */
		m->m_pcImage->Draw( cBounds, cBounds, this );
	}
	else
	{
		m->m_pcImage->Draw( cBounds, cBounds, this );
	}
}

void ImageView::__IV_reserved1__() {}
void ImageView::__IV_reserved2__() {}
void ImageView::__IV_reserved3__() {}
void ImageView::__IV_reserved4__() {}
void ImageView::__IV_reserved5__() {}


