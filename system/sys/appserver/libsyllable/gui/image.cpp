#include <translation/translator.h>
#include <gui/view.h>
#include <util/exceptions.h>
#include <gui/image.h>
#include <assert.h>

#define CLAMP255( x )	( x > 255 ? 255 : x )

using namespace os;

//
// From IconEdit:
//

/** \internal */
enum bitmapscale_filtertype
{
	filter_filter,
	filter_box,
	filter_triangle,
	filter_bell,
	filter_bspline,
	filter_lanczos3,
	filter_mitchell
};

void Scale( Bitmap * srcbitmap, Bitmap * dstbitmap, bitmapscale_filtertype filtertype, float filterwidth = 0.0f );

//
//
//

/** \internal */
class Image::Private
{
      public:
	Private()
	{
		m_cSize.x = m_cSize.y = 0;
	}

	/** Size of the image */
	Point m_cSize;
};

/**
 * Default constructor
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Image::Image()
{
	m = new Private;
}

Image::~Image()
{
	delete m;
}

/** Set the size of the image
 * \par		Description:
 *		Scales the image.
 *		SetSize may provide better quality scaling than Draw(), which
 *		is optimized for speed rather than quality.
 *		You are not guaranteed to get exactly the requested size with
 *		kinds of image objects.
 * \param	cSize The new size.
 * \sa GetSize()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Image::SetSize( const Point & cSize )
{
	m->m_cSize = cSize;
	return 0;
}

/** Get image size
 * \par		Description:
 *		Returns the size of the image.
 *		Note! This returns the actual size in pixels.
 * \sa SetSize(), GetBounds()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Point Image::GetSize( void ) const
{
	return m->m_cSize;
}

/** Get image bounds
 * \par		Description:
 *		Returns the size of the image as a Rect object (for consistency).
 *		Left and Top will typically be set to 0 and right/bottom to the
 *		value returned by GetSize() - 1.
 *		Note! GetBounds().Width() and GetBounds().Height() will return one
 *		less then GetSize().x and GetSize.y, due to coordinate system conventions.
 * \sa SetSize(), GetBounds()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Rect Image::GetBounds( void ) const
{
	Point z = GetSize();

	return Rect( 0, 0, z.x - 1, z.y - 1 );
}

status_t Image::ApplyFilter( uint32 nFilterID )
{
	return ApplyFilter( Message( nFilterID ) );
}

status_t Image::ApplyFilter( const Message & cFilterData )
{
	return -1;
}

//
// BitmapImage
//

/** \internal */
class BitmapImage::Private
{
      public:
	Private()
	{
		m_pcBitmap = NULL;
		m_pcView = NULL;
		m_nBitmapFlags = 0;
	}

	~Private()
	{
		if( !m_pcBitmap && m_pcView ) {
			// The view will be deleted when deleting the bitmap,
			// unless, of course, there is no bitmap to delete.
			delete m_pcView;
		}			
		SetBitmap( NULL );
	}

	/** Set the bitmap pointer, delete old bitmap, if any */
	void SetBitmap( Bitmap * pcBitmap )
	{
		if( m_pcBitmap ) {
			if( m_pcView ) {
				m_pcBitmap->RemoveChild( m_pcView );
			}
			delete m_pcBitmap;
		}
		m_pcBitmap = pcBitmap;
		if( m_pcBitmap && m_pcView ) {
			m_pcView->SetFrame( m_pcBitmap->GetBounds() );
			m_pcBitmap->AddChild( m_pcView );
		}
	}

	/** Make sure that we have a bitmap */
	void AssertBitmap() const
	{
		if( !m_pcBitmap )
			throw errno_exception( "Empty bitmap" );
	}

	/** Make sure that the bitmap was allocated with the right flags, change otherwise */
	void AssertBitmapFlags( uint32 nFlags )
	{
		AssertBitmap();
		if( ( m_nBitmapFlags & nFlags ) != nFlags ) {
			Rect cBounds( m_pcBitmap->GetBounds() );
			Bitmap* pcNew = new Bitmap( int(cBounds.Width() + 1), int(cBounds.Height() + 1), m_pcBitmap->GetColorSpace(), m_nBitmapFlags | nFlags );
			if( pcNew ) {
				/* Copy the raster data to new Bitmap. This is only possible if the old Bitmap was created with SHARE_FRAMEBUFFER flag. */
				if( m_nBitmapFlags & Bitmap::SHARE_FRAMEBUFFER ) {
				memcpy( pcNew->LockRaster(), m_pcBitmap->LockRaster(), m_pcBitmap->GetBytesPerRow() * int( cBounds.Height() + 1 ) );
					pcNew->UnlockRaster();
					m_pcBitmap->UnlockRaster();
				}
				m_nBitmapFlags |= nFlags;
				delete m_pcBitmap;
				m_pcBitmap = pcNew;
			} else {
				throw errno_exception( "" );
			}
		}
	}

	/** Pointer to the Bitmap */
	Bitmap*	m_pcBitmap;

	/** Bitmap Flags (see os::Bitmap) */
	uint32	m_nBitmapFlags;

	/** View for rendering to the bitmap */
	View*	m_pcView;
};

/**
 * Default constructor
 *  \param	nFlags Bitmap flags, see os::Bitmap.
 * \sa os::Bitmap, os::BitmapImage::IsValid()
 * \note This constructor does not create the internal Bitmap object.
 *       Thus BitmapImage::IsValid() will return false after this creator is used.
 *       Note that the methods BitmapImage::Load(), BitmapImage::SetBitmapData(), BitmapImage::SetSize(),
 *       BitmapImage::ResizeCanvas() (re)initialise the internal Bitmap object, so one of
 *       these must be called before operations such as BitmapImage::Draw(), BitmapImage::GetSize() etc can be used.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
BitmapImage::BitmapImage( uint32 nFlags )
{
	m = new BitmapImage::Private;
	m->m_nBitmapFlags = nFlags;
}

/**
 * Copy constructor
 *  \param	cSource Original BitmapImage object to copy.
 *  \param	nFlags Bitmap flags, see os::Bitmap.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
BitmapImage::BitmapImage( const BitmapImage & cSource, uint32 nFlags )
{
	m = new BitmapImage::Private;
	m->m_nBitmapFlags = nFlags;
	*this = cSource;
}

/**
 * Constructor
 *  \param	pcSource A pointer to a StreamableIO object. Could be a file,
 *		a resource or any data source that can be represented as a
 *		stream of data. The Translator API is used to try to recognize
 *		the format and load it as bitmap data.
 *		If you need to explicitly specify the file format, use Load().
 *  \param	nFlags Bitmap flags, see os::Bitmap.
 *
 *  \sa Load(), os::Bitmap
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
BitmapImage::BitmapImage( StreamableIO * pcSource, uint32 nFlags )
{
	m = new BitmapImage::Private;
	m->m_nBitmapFlags = nFlags;
	Load( pcSource );
}

/**
 * Constructor
 *  \param	pData Pointer to an array of raw bitmap data.
 *  \param	cSize The size of the bitmap in pixels.
 *  \param	eColorSpace Color space, for instance CS_RGB32.
 *  \param	nFlags Bitmap flags, see os::Bitmap.
 *
 *  \sa os::color_space, os::Bitmap
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
BitmapImage::BitmapImage( const uint8 *pData, const IPoint & cSize, color_space eColorSpace, uint32 nFlags )
{
	m = new BitmapImage::Private;
	m->m_nBitmapFlags = nFlags;
	SetBitmapData( pData, cSize, eColorSpace, nFlags );
}

BitmapImage::~BitmapImage()
{
	delete m;
}

/** Get object type.
 * \par Description:
 *	Used for run-time type checking. Returs the class name, "BitmapImage".
 * \sa Image::ImageType()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
const String BitmapImage::ImageType( void ) const
{
	return String( "BitmapImage" );
}

/** Find out if the bitmap object is valid.
 * \par Description:
 *	Use to find out if the object contains a usable bitmap.
 * \sa Image::IsValid()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
bool BitmapImage::IsValid( void ) const
{
	return m->m_pcBitmap ? true : false;
}

/** Load bitmap from a stream (file, memory, resource etc.).
 * \par 	Description:
 *		This method loads and translates bitmaps in any format
 *		supported by the Translators from any data source that can
 *		be represented as a stream.
 * \param	pcSource A pointer to a StreamableIO object. Could be a file,
 *		a resource or any data source that can be represented as a
 *		stream of data. The Translator API is used to try to recognize
 *		the format and load it as bitmap data.
 * \param	cType Used to specify a specific file format, if the automatic
 *		recognition is not enough.
 * \par		Example:
 * \code
 *		File cFile( "picture.png" );
 *		myImage->Load( &cFile, "image/png" );
 * \endcode
 * \sa Save(), os::File, os::StreamableIO, os::MemFile
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::Load( StreamableIO * pcSource, const String & cType )
{
	Translator *trans = NULL;
	TranslatorFactory *factory = TranslatorFactory::GetDefaultFactory();

	assert( pcSource );

	//factory->LoadAll();

	try
	{
		uint8 buffer[8192];
		uint frameSize = 0;
		int x = 0;
		int y = 0;

		BitmapFrameHeader frameHeader;

		for( ;; )
		{
			int bytesRead = pcSource->Read( buffer, sizeof( buffer ) );

			if( bytesRead == 0 )
				break;

			if( trans == NULL )
			{
				int error = factory->FindTranslator( "", TRANSLATOR_TYPE_IMAGE, buffer, bytesRead, &trans );

				if( error < 0 )
					return false;
			}

			trans->AddData( buffer, bytesRead, bytesRead != sizeof( buffer ) );

			while( true )
			{
				if( m->m_pcBitmap == NULL )
				{
					BitmapHeader BmHeader;

					if( trans->Read( &BmHeader, sizeof( BmHeader ) ) != sizeof( BmHeader ) )
						break;
					m->m_nBitmapFlags |= Bitmap::SHARE_FRAMEBUFFER;
					m->SetBitmap( new Bitmap( BmHeader.bh_bounds.Width() + 1, BmHeader.bh_bounds.Height(  ) + 1, CS_RGB32, m->m_nBitmapFlags ) );
					if( !m->m_pcBitmap )
						break;
				}

				if( frameSize == 0 )
				{
					if( trans->Read( &frameHeader, sizeof( frameHeader ) ) != sizeof( frameHeader ) )
						break;
					x = frameHeader.bf_frame.left * 4;
					y = frameHeader.bf_frame.top;
					frameSize = frameHeader.bf_data_size;
				}

				uint8 frameBuffer[8192];

				bytesRead = trans->Read( frameBuffer, std::min( frameSize, sizeof( frameBuffer ) ) );
				if( bytesRead <= 0 )
					break;

				frameSize -= bytesRead;
				uint8 *DstRaster = m->m_pcBitmap->LockRaster();
				int SrcOffset = 0;

				for( ; y <= frameHeader.bf_frame.bottom && bytesRead > 0; )
				{
					int len = std::min( bytesRead, frameHeader.bf_bytes_per_row - x );

					memcpy( DstRaster + y * m->m_pcBitmap->GetBytesPerRow() + x, frameBuffer + SrcOffset, len );
					if( x + len == frameHeader.bf_bytes_per_row )
					{
						x = 0;
						y++;
					}
					else
					{
						x += len;
					}
					bytesRead -= len;
					SrcOffset += len;
				}
			}
		}
		if( trans )
			delete trans;
		if( m->m_pcBitmap )
			return 0;
		else
			return -1;
	}
	catch( ... )
	{
		return -1;
	}
	return -1;
}

/** Write bitmap to a stream.
 * \par		Description:
 *		This method translates and writes bitmaps to a stream in any
 *		Translator supported format. (Note! At the time of writing
 *		there are no Translators capable of translating from the
 *		internal format to any other format.)
 * \param	pcDest A pointer to a StreamableIO object.
 * \param	cType String that describes the file format, eg "image/png".
 * \par		Example:
 * \code
 *		File cFile( "picture.png" );
 *		myImage->Save( &cFile, "image/png" );
 * \endcode
 * \note This method can only be used if the bitmap was created with the SHARE_FRAMEBUFFER flag.
 * \sa Load(), os::File, os::StreamableIO, os::MemFile
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::Save( StreamableIO * pcDest, const String & cType )
{
	if( !(m->m_nBitmapFlags & Bitmap::SHARE_FRAMEBUFFER) ) {
		dbprintf( "BitmapImage::Save() called without SHARE_FRAMEBUFFER flag!\n" );
		return( -1 );
	}


	Translator *trans = NULL;
	TranslatorFactory *factory = TranslatorFactory::GetDefaultFactory();

//	factory->LoadAll();

	try
	{
		uint8 buffer[8192];

		int error = factory->FindTranslator( TRANSLATOR_TYPE_IMAGE, cType, NULL, 0, &trans );

		if( error < 0 )
			return false;

		//      printf("Found translator\n");

		BitmapHeader cBmHeader;

		cBmHeader.bh_header_size = sizeof( cBmHeader );
		cBmHeader.bh_data_size = ( int )( GetSize().x * GetSize(  ).y ) * 4;
		cBmHeader.bh_flags = 0;
		cBmHeader.bh_bounds.left = 0;
		cBmHeader.bh_bounds.top = 0;
		cBmHeader.bh_bounds.right = ( int )GetSize().x - 1;
		cBmHeader.bh_bounds.bottom = ( int )GetSize().y - 1;
		cBmHeader.bh_frame_count = 1;
		cBmHeader.bh_bytes_per_row = 4 * ( int )GetSize().x;
		cBmHeader.bh_color_space = CS_RGB32;

		trans->AddData( &cBmHeader, sizeof( cBmHeader ), 0 );

		BitmapFrameHeader cFrameHeader;

		cFrameHeader.bf_header_size = sizeof( cFrameHeader );
		cFrameHeader.bf_data_size = ( int )( GetSize().x * GetSize(  ).y ) * 4;
		cFrameHeader.bf_time_stamp = 0;
		cFrameHeader.bf_color_space = CS_RGB32;
		cFrameHeader.bf_bytes_per_row = 4 * ( int )GetSize().x;
		cFrameHeader.bf_frame.left = 0;
		cFrameHeader.bf_frame.top = 0;
		cFrameHeader.bf_frame.right = ( int )GetSize().x - 1;
		cFrameHeader.bf_frame.bottom = ( int )GetSize().y - 1;

		trans->AddData( &cFrameHeader, sizeof( cFrameHeader ), 0 );

		//      printf("Adding bitmap: %ld bytes\n", cFrameHeader.bf_data_size);

		uint8 *DstRaster = m->m_pcBitmap->LockRaster();

		trans->AddData( DstRaster, cFrameHeader.bf_data_size, true );

		int size;

		do
		{
			size = trans->Read( buffer, sizeof( buffer ) );
			if( size > 0 )
			{
				pcDest->Write( buffer, size );
			}
		}
		while( size == sizeof( buffer ) );
		
		if( trans )
			delete trans;
	}
	catch( ... )
	{
		return -1;
	}
	return -1;
}

void BitmapImage::Draw( const Point & cPos, View * pcView )
{
	if( !m->m_pcBitmap || !pcView )
		return;

/*	dbprintf("Drawing Image: %p, %d x %d to view: %p\n",  m->m_pcBitmap,
		(int)GetSize().x, (int)GetSize().y, pcView );*/

	Rect cSource( 0, 0, GetSize().x - 1, GetSize(  ).y - 1 );
	Rect cDest = cSource + Point( ( int )cPos.x, ( int )cPos.y );

	pcView->DrawBitmap( m->m_pcBitmap, cSource, cDest );
}

void BitmapImage::Draw( const Rect & cSource, const Rect & cDest, View * pcView )
{
	if( !m->m_pcBitmap || !pcView )
		return;

	pcView->DrawBitmap( m->m_pcBitmap, cSource, cDest );
}

/** Scale the bitmap
 * \par		Description:
 *		Scales the bitmap to the specified size. Returns an error
 *		if memory for a new bitmap can not be allocated.
 *		SetSize may provide better quality scaling than Draw(), which
 *		is optimized for speed rather than quality.
 *		The image will remain unchanged if memory allocation fails.
 * \param	cSize The new size.
 * \sa GetSize(), ResizeCanvas()
 * \bug		Works only with CS_RGB(A)32
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::SetSize( const Point & cSize )
{
	Bitmap *bmap = new Bitmap( (int)cSize.x, (int)cSize.y, GetColorSpace(), m->m_nBitmapFlags );

	if( bmap )
	{
		if( m->m_pcBitmap )
		{
			Scale( m->m_pcBitmap, bmap, filter_lanczos3 );
		}

		m->SetBitmap( bmap );
	}

	return -1;
}

/** Get bitmap size
 * \par		Description:
 *		Returns the size of the internal bitmap object.
 * \retval	Actual Bitmap size.
 * \sa SetSize()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Point BitmapImage::GetSize( void ) const
{
	if( m->m_pcBitmap )
		return Point( m->m_pcBitmap->GetBounds().Width(  ) + 1, m->m_pcBitmap->GetBounds(  ).Height(  ) + 1 );
	else
		return Point( 0, 0 );
}

/** Obtain a View for rendering into the bitmap
 * \par		Description:
 *		Returns a pointer to a View, that can be used for rendering into the
 *		bitmap. This is useful for double buffering.
 * \note	When finished drawing to the View, you need to call Sync(). The
 *	drawing operations might not take place until you have done so.
 * \note	The BitmapImage should be initialized with the flag ACCEPT_VIEWS.
 * If it isn't, GetView() will try to re-allocate the bitmap, changing this flag.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
View* BitmapImage::GetView()
{
	if( !m->m_pcView ) {
		m->AssertBitmapFlags( Bitmap::ACCEPT_VIEWS );
		m->m_pcView = new View( m->m_pcBitmap->GetBounds(), "BitmapImage::m::m_pcView" );
		m->m_pcBitmap->AddChild( m->m_pcView );
	}

	return m->m_pcView;
}

/** Change the size of the bitmap
 * \par		Description:
 *		Changes the size of the bitmap to the specified size. Returns an error
 *		if memory for a new bitmap can not be allocated.
 *		Note that the image data is lost. To change the size of the image, use
 *		SetSize() instead.
 * \param	cSize The new size.
 * \sa SetSize(), GetSize()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void BitmapImage::ResizeCanvas( const Point& cSize )
{
	Bitmap *bmap = new Bitmap( (int)cSize.x, (int)cSize.y, GetColorSpace(), m->m_nBitmapFlags );

	if( bmap )
	{
		m->SetBitmap( bmap );
	}
}

/** Direct access to pixels
 * \par		Description:
 *		Returns a pointer to a row of raw pixel data. 
 * \note	The BitmapImage should be initialized with the flag SHARE_FRAMEBUFFER.
 * If it isn't, this method will try to re-allocate the bitmap, changing this flag.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
uint32* BitmapImage::operator[]( int row )
{
	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );
	return (uint32*)( m->m_pcBitmap->LockRaster() + m->m_pcBitmap->GetBytesPerRow() * row );
}

/** Flush the render queue, and wait til the rendering is done.
 * \par Description:
 *	Call this method before accessing bitmap data that you have created by
 *	rendering to the View associated with this BitmapImage.
 * \sa Window::Sync(), Bitmap::Sync(), GetView()
 * \author	Henrik Isaksson
 *****************************************************************************/
void BitmapImage::Sync()
{
	if( m->m_pcBitmap ) {
		m->m_pcBitmap->Sync();
	}
}

/** Flush the render queue.
 * \par Description:
 * See Window::Sync().
 * \sa Window::Sync(), Bitmap::Sync(), GetView()
 * \author	Henrik Isaksson
 *****************************************************************************/
void BitmapImage::Flush()
{
	if( m->m_pcBitmap ) {
		m->m_pcBitmap->Flush();
	}
}

/** Get bitmap colour space
 * \par		Description:
 *		Returns the colour space for the image.
 * \retval	The bitmap's colour space, one of the values in os::color_space.
 * \sa os::color_space, SetColorSpace()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
color_space BitmapImage::GetColorSpace() const
{
	if( m->m_pcBitmap )
		return m->m_pcBitmap->GetColorSpace();
	return CS_RGB32;
}

/** Set colour space
 * \par		Description:
 *		Transform the bitmap to a different colour space. This method
 *		may reduce the quality of the image data!
 * \param	eColorSpace The colour space, eg. CS_RGB32.
 * \retval	0 for success, -1 for failure.
 * \bug		Not implemented.
 * \todo	Implement BitmapImage::SetColorSpace
 * \sa os::color_space, GetColorSpace()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::SetColorSpace( color_space eColorSpace )
{
	/* TODO: Change colour space */
	return -1;
}

status_t BitmapImage::ApplyFilter( const Message & cFilterData )
{
	switch ( cFilterData.GetCode() )
	{
	case F_GRAY:
		return GrayFilter();
	case F_HIGHLIGHT:
		return HighlightFilter();
	case F_ALPHA_TO_OVERLAY:
		return AlphaToOverlay( TRANSPARENT_RGB32 );
	case F_GLOW:
		{
			Color32_s c1 = 0xFFFFFFFF, c2 = 0x3FDDAA00;
			int32 radius = 4;

			cFilterData.FindColor32( "innerColor", &c1 );
			cFilterData.FindColor32( "outerColor", &c2 );
			cFilterData.FindInt32( "radius", &radius );
			return GlowFilter( c1, c2, radius );
		}
	case F_COLORIZE:
		{
			Color32_s color( 0xFFFF0000 );

			cFilterData.FindColor32( "color", &color );
			return ColorizeFilter( color );
		}
	default:
		return -1;
	}
	return 0;
}

/** Create a greyed image
 * \par		Description:
 *		Removes colour information and makes the image brighter,
 *		primarily intended for GUI components, where it will be used
 *		to create "disabled" icons.
 * \note	The greying algorithm may change in future versions. On
 *		certain systems it may also be adapted to suit their displays.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::GrayFilter( void )
{
	uint8 *bytes;
	uint32 numbytes;
	Color32_s col( get_default_color( COL_NORMAL ) );
	uint8 r = ( col.red + 255 ) / 2;
	uint8 g = ( col.green + 255 ) / 2;
	uint8 b = ( col.blue + 255 ) / 2;

	if( !m->m_pcBitmap )
		return -2;

	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );

	numbytes = m->m_pcBitmap->GetBytesPerRow() * ( int )( m->m_pcBitmap->GetBounds(  ).Height(  ) + 1 );

	bytes = m->m_pcBitmap->LockRaster();
	if( bytes )
	{
		uint32 i;

		for( i = 0; i < numbytes; i += 4 )
		{
			uint8 average = ( bytes[i] + bytes[i + 1] + bytes[i + 2] ) / 3;

			bytes[i] = ( uint8 )CLAMP255( ( average + r ) / 2 );
			bytes[i + 1] = ( uint8 )CLAMP255( ( average + g ) / 2 );
			bytes[i + 2] = ( uint8 )CLAMP255( ( average + b ) / 2 );
		}
	}
	m->m_pcBitmap->UnlockRaster();

	return 0;
}

/** Make the image highlighted
 * \par		Description:
 *		Make the image highlighted, for use with icons, for instance.
 * \note	The highlighting algorithm may change in future versions. On
 *		certain systems it may also be adapted to suit their displays.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::HighlightFilter( void )
{
	uint8 *bytes;
	uint32 numbytes;
	Color32_s col( get_default_color( COL_SEL_WND_BORDER ) );
	uint8 r = col.red;
	uint8 g = col.green;
	uint8 b = col.blue;

	if( !m->m_pcBitmap )
		return -2;

	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );

	numbytes = m->m_pcBitmap->GetBytesPerRow() * ( int )( m->m_pcBitmap->GetBounds(  ).Height(  ) + 1 );

	bytes = m->m_pcBitmap->LockRaster();
	if( bytes )
	{
		uint32 i;

		for( i = 0; i < numbytes; i += 4 )
		{
			bytes[i] = ( uint8 )CLAMP255( ( bytes[i] + r ) / 2 );
			bytes[i + 1] = ( uint8 )CLAMP255( ( bytes[i + 1] + g ) / 2 );
			bytes[i + 2] = ( uint8 )CLAMP255( ( bytes[i + 2] + b ) / 2 );
		}
	}
	m->m_pcBitmap->UnlockRaster();

	return 0;
}

status_t BitmapImage::ColorizeFilter( Color32_s cColor )
{
	uint8 *bytes;
	uint32 numbytes;
	uint8 r = cColor.red;
	uint8 g = cColor.green;
	uint8 b = cColor.blue;

	if( !m->m_pcBitmap )
		return -2;

	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );

	numbytes = m->m_pcBitmap->GetBytesPerRow() * ( int )( m->m_pcBitmap->GetBounds(  ).Height(  ) + 1 );

	bytes = m->m_pcBitmap->LockRaster();
	if( bytes )
	{
		uint32 i;

		for( i = 0; i < numbytes; i += 4 )
		{
			bytes[i] = ( uint8 )CLAMP255( ( bytes[i] + r ) / 2 );
			bytes[i + 1] = ( uint8 )CLAMP255( ( bytes[i + 1] + g ) / 2 );
			bytes[i + 2] = ( uint8 )CLAMP255( ( bytes[i + 2] + b ) / 2 );
		}
	}
	m->m_pcBitmap->UnlockRaster();

	return 0;
}

/** Add glow effect to the image
 * \par		Description:
 *		Adds a "glowing" outline to the image.
 * \param cInnerColor The colour nearest the edge of the picture.
 * \param cOuterColor The colour at the edge of the glow. Tip: set a low
 *	  opacity here, to blend the glow with the background.
 * \param nRadius The width of the glow.
 * \note This function requires an image with correct alpha channel data.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::GlowFilter( Color32_s cInnerColor, Color32_s cOuterColor, int nRadius )
{
	if( nRadius < 1 )
		nRadius = 1;
	float red = ( cInnerColor.red - cOuterColor.red ) / ( float )nRadius;
	float green = ( cInnerColor.green - cOuterColor.green ) / ( float )nRadius;
	float blue = ( cInnerColor.blue - cOuterColor.blue ) / ( float )nRadius;
	float alpha = ( cInnerColor.alpha - cOuterColor.alpha ) / ( float )nRadius;

	if( !m->m_pcBitmap )
		return -2;

	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );

	uint8 *pSrc = m->m_pcBitmap->LockRaster();

	if( !pSrc )
		return -2;

	int32 nWidth = ( uint32 )GetSize().x;
	int32 nHeight = ( uint32 )GetSize().y;
	int32 nBytesPerRow = m->m_pcBitmap->GetBytesPerRow();

	Bitmap *pcDestBmap;

	pcDestBmap = new Bitmap( nWidth, nHeight, GetColorSpace(), m->m_nBitmapFlags );
	if( !pcDestBmap )
		return -2;

	uint8 *pDest = pcDestBmap->LockRaster();

	int32 x, y;

	memset( pDest, 0x00, pcDestBmap->GetBytesPerRow() * nHeight );

	for( y = 0; y < nHeight; y++ )
	{
		uint8 *d = &pDest[y * nBytesPerRow];

		for( x = 0; x < nWidth; x++ )
		{
			int i;

			for( i = 0; i < nRadius; i++ )
			{
				if( ( x + i ) < nWidth && pSrc[y * nBytesPerRow + ( x + i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( x - i ) >= 0 && pSrc[y * nBytesPerRow + ( x - i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( y + i ) < nHeight && pSrc[( y + i ) * nBytesPerRow + x * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( y - i ) >= 0 && pSrc[( y - i ) * nBytesPerRow + x * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( x + i ) < nWidth && ( y + i ) < nHeight && pSrc[( y + i ) * nBytesPerRow + ( x + i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( x + i ) < nWidth && ( y - i ) >= 0 && pSrc[( y - i ) * nBytesPerRow + ( x + i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( x - i ) >= 0 && ( y + i ) < nHeight && pSrc[( y + i ) * nBytesPerRow + ( x - i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
				else if( ( x - i ) >= 0 && ( y - i ) >= 0 && pSrc[( y - i ) * nBytesPerRow + ( x - i ) * 4 + 3] > 0x3F )
				{
					d[3]++;
				}
			}
			d += 4;
		}
	}

	for( y = 0; y < nHeight; y++ )
	{
		uint8 *d = &pDest[y * nBytesPerRow];

		for( x = 0; x < nWidth; x++ )
		{
			uint8 d3 = d[3];

			if( d3 )
			{
				//if( d3 > nRadius ) d3 = nRadius;
				d[0] = ( uint8 )( cOuterColor.blue + d3 * blue );
				d[1] = ( uint8 )( cOuterColor.green + d3 * green );
				d[2] = ( uint8 )( cOuterColor.red + d3 * red );
				d[3] = ( uint8 )( cOuterColor.alpha + d3 * alpha );
			}
			uint8 b = pSrc[y * nBytesPerRow + x * 4 + 3];
			uint8 a = 255 - b;

			d[0] = ( d[0] * a + pSrc[y * nBytesPerRow + x * 4 + 0] * b ) >> 8;
			d[1] = ( d[1] * a + pSrc[y * nBytesPerRow + x * 4 + 1] * b ) >> 8;
			d[2] = ( d[2] * a + pSrc[y * nBytesPerRow + x * 4 + 2] * b ) >> 8;
			d[3] = ( d[3] * a + pSrc[y * nBytesPerRow + x * 4 + 3] * b ) >> 8;
			d += 4;
		}
	}

	m->m_pcBitmap->UnlockRaster();
	pcDestBmap->UnlockRaster();
	m->SetBitmap( pcDestBmap );

	return 0;
}

/** Lock the internal Bitmap object
 * \par		Description:
 *		Lock the internal Bitmap object so it can be safely
 *		manipulated.
 * \par		Example:
 * \code
 *	Bitmap	*myBitmap = myImage->LockBitmap();
 *	if( myBitmap ) {
 *
 *		...
 *
 *		myImage->UnlockBitmap();
 *	}
 * \endcode
 * \note	Don't forget to Unlock the Bitmap by calling UnlockBitmap().
 * \retval	Pointer to the locked Bitmap. If the internal Bitmap is not valid, returns NULL (see IsValid()).
 * \sa UnlockBitmap(), IsValid(), os::Bitmap
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Bitmap *BitmapImage::LockBitmap( void )
{
	return m->m_pcBitmap;
}

/** Unlock the internal Bitmap object
 * \par		Description:
 *		Unlock the internal Bitmap object
 * \sa LockBitmap(), os::Bitmap
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void BitmapImage::UnlockBitmap( void )
{
}

/** Convert alpha channel to overlay
 * \par		Description:
 *		Convert an image with alpha transparency to overlay
 *		transparency. Overlay may provide better rendering performance,
 *		but does not support gradual transparency; it's either fully
 *		transparent or entirely	opaque.
 * \par		The conversion is done by setting all pixels with >25%
 *		transparency to TRANSPARENT_RGB32, which is pure white.
 *		Any pixel that is pure white, will have it's colour changed.
 *		(The blue component is tweaked +/- one LSB).
 * \param	cTransparentColor The colour that will be transparent.
 *		Typically you will use the default, TRANSPARENT_RGB32.
 *		
 * \sa os::View, os::Bitmap, os::View::DrawBitmap()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t BitmapImage::AlphaToOverlay( uint32 cTransparentColor )
{
	uint32 *data;
	uint32 numbytes;

	uint32 tcol = cTransparentColor & 0xFFFFFF00;

	if( !m->m_pcBitmap )
		return -2;

	if( GetColorSpace() != CS_RGB32 )
	{
		return -2;
	}

	m->AssertBitmapFlags( Bitmap::SHARE_FRAMEBUFFER );

	numbytes = m->m_pcBitmap->GetBytesPerRow() * ( int )( m->m_pcBitmap->GetBounds(  ).Height(  ) + 1 );

	data = ( uint32 * )m->m_pcBitmap->LockRaster();
	if( data )
	{
		uint32 i;

		for( i = 0; i < numbytes / 4; i++ )
		{
			if( ( ( uint8 )( data[i] >> 24 ) ) < 0x3F )
			{
				data[i] = cTransparentColor;
			}
			else if( ( data[i] & 0xFFFFFF00 ) == tcol )
			{
				data[i] ^= 0x00010000;
			}
		}
	}
	m->m_pcBitmap->UnlockRaster();

	return 0;
}

/** Set raw bitmap data
 * \par		Description:
 *		Set raw bitmap data.
 *  \param	pData Pointer to an array of raw bitmap data.
 *  \param	cSize The size of the bitmap in pixels.
 *  \param	eColorSpace Color space, for instance CS_RGB32.
 *  \param	nFlags Bitmap flags, see os::Bitmap. SHARE_FRAMEBUFFER is assumed.
 *
 *  \sa os::color_space, os::Bitmap
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void BitmapImage::SetBitmapData( const uint8 *pData, const IPoint & cSize, color_space eColorSpace, uint32 nFlags )
{
	m->m_nBitmapFlags = (nFlags | Bitmap::SHARE_FRAMEBUFFER);
	Bitmap *bmap = new Bitmap( cSize.x, cSize.y, eColorSpace, m->m_nBitmapFlags );
	uint8 *dest = bmap->LockRaster();

	memcpy( dest, pData, ( int )( bmap->GetBytesPerRow() * cSize.y ) );
	m->SetBitmap( bmap );
}

/** Copy the contents of another bitmap.
 * \par		Description:
 *		Copy the contents of another bitmap.
 * \note The source bitmap must be created with SHARE_FRAMEBUFFER flag. If not, the assignment has no effect.
 *  \sa os::Bitmap
 *****************************************************************************/
BitmapImage & BitmapImage::operator=( const BitmapImage & cSource )
{
	uint8 *source;

	source = cSource.m->m_pcBitmap->LockRaster();
	if( source == NULL ) return *this;    /* Source does not have SHARE_FRAMEBUFFER flag; can't access the raster data */
	SetBitmapData( source, IPoint( ( int )cSource.GetSize().x, ( int )cSource.GetSize(  ).y ), cSource.GetColorSpace(  ), m->m_nBitmapFlags );
	return *this;
}

// ----------------------------------------------------------------------------
// --- Bitmap scaling functions from IconEdit ---------------------------------
// ----------------------------------------------------------------------------

// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
// Base on code from Graphical Gems.
// Ported to AtheOS by Kurt Skauen 29/10-1999
//-----------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>


//#include "bitmapscale.h"
//-----------------------------------------------------------------------------

/** \internal */
struct contrib
{
	int sourcepos;
	float floatweight;
	int32 weight;
};

/** \internal */
struct contriblist
{
//      contriblist(int cnt) { contribcnt=0; contribs=new contrib[cnt]; }
	~contriblist()
	{
		delete[]contribs;
	}
	int contribcnt;
	contrib *contribs;
};

typedef float ( scale_filterfunc ) ( float );

static float Filter_Filter( float );
static float Filter_Box( float );
static float Filter_Triangle( float );
static float Filter_Bell( float );
static float Filter_BSpline( float );
static float Filter_Lanczos3( float );
static float Filter_Mitchell( float );

int clamp( int v, int l, int h )
{
	return v < l ? l : v > h ? h : v;
}

//-----------------------------------------------------------------------------

contriblist *CalcFilterWeight( float scale, float filterwidth, int srcsize, int dstsize, scale_filterfunc * filterfunc )
{
	contriblist *contriblists = new contriblist[dstsize];

	float size;
	float fscale;

	if( scale < 1.0f )
	{
		size = filterwidth / scale;
		fscale = scale;
	}
	else
	{
		size = filterwidth;
		fscale = 1.0f;
	}

	for( int i = 0; i < dstsize; i++ )
	{
		contriblists[i].contribcnt = 0;
		contriblists[i].contribs = new contrib[( int )( size * 2 + 1 )];
		float center = ( float )i / scale;
		float start = ceil( center - size );
		float end = floor( center + size );
		float totweight = 0.0f;

		for( int j = ( int )start; j <= end; j++ )
		{
			int sourcepos;

			if( j < 0 )
				sourcepos = -j;
			else if( j >= srcsize )
				sourcepos = ( srcsize - j ) + srcsize - 1;
			else
				sourcepos = j;
			int newcontrib = contriblists[i].contribcnt++;

			contriblists[i].contribs[newcontrib].sourcepos = sourcepos;

			float weight = filterfunc( ( center - ( float )j ) * fscale ) * fscale;

			totweight += weight;
			contriblists[i].contribs[newcontrib].floatweight = weight;
		}
		totweight = 1.0f / totweight;
		for( int j = 0; j < contriblists[i].contribcnt; j++ )
		{
			contriblists[i].contribs[j].weight = ( int )( contriblists[i].contribs[j].floatweight * totweight * 65530 );
		}
		uint val = 0;

		for( int j = 0; j < contriblists[i].contribcnt; j++ )
		{
			val += contriblists[i].contribs[j].weight;
//                      printf( "%d ", contriblists[i].contribs[j].weight );
		}
//              printf( "%X\n", val );

	}

	return contriblists;
}

//-----------------------------------------------------------------------------

void Scale( Bitmap * srcbitmap, Bitmap * dstbitmap, bitmapscale_filtertype filtertype, float filterwidth )
{
	assert( dstbitmap != NULL );
	assert( srcbitmap != NULL );

	assert( srcbitmap->LockRaster() != NULL );  /* This can occur if the bitmap wasn't created with SHARE_FRAMEBUFFER flag */
	assert( dstbitmap->LockRaster() != NULL );

//      assert( dstbitmap->ColorSpace() == B_RGB32 );
//      assert( srcbitmap->ColorSpace() == B_RGB32 );

	scale_filterfunc *filterfunc;
	float default_filterwidth;

	switch ( filtertype )
	{
	case filter_filter:
		filterfunc = Filter_Filter;
		default_filterwidth = 1.0f;
		break;
	case filter_box:
		filterfunc = Filter_Box;
		default_filterwidth = 0.5f;
		break;		// was 0.5
	case filter_triangle:
		filterfunc = Filter_Triangle;
		default_filterwidth = 1.0f;
		break;
	case filter_bell:
		filterfunc = Filter_Bell;
		default_filterwidth = 1.5f;
		break;
	case filter_bspline:
		filterfunc = Filter_BSpline;
		default_filterwidth = 2.0f;
		break;
	case filter_lanczos3:
		filterfunc = Filter_Lanczos3;
		default_filterwidth = 3.0f;
		break;
	case filter_mitchell:
		filterfunc = Filter_Mitchell;
		default_filterwidth = 2.0f;
		break;

	default:
		filterfunc = Filter_Filter;
		default_filterwidth = 1.0f;
		break;
	}

	if( filterwidth == 0.0f )
		filterwidth = default_filterwidth;

	int srcbitmap_width = int ( srcbitmap->GetBounds().Width(  ) ) + 1;
	int srcbitmap_height = int ( srcbitmap->GetBounds().Height(  ) ) + 1;
	int dstbitmap_width = int ( dstbitmap->GetBounds().Width(  ) ) + 1;
	int dstbitmap_height = int ( dstbitmap->GetBounds().Height(  ) ) + 1;
	int tmpbitmap_width = dstbitmap_width;
	int tmpbitmap_height = srcbitmap_height;

	uint32 *srcbitmap_bits = ( uint32 * )srcbitmap->LockRaster();
	uint32 *dstbitmap_bits = ( uint32 * )dstbitmap->LockRaster();
	uint32 *tmpbitmap_bits = new uint32[tmpbitmap_width * tmpbitmap_height];

	int srcbitmap_bpr = srcbitmap->GetBytesPerRow() / 4;
	int dstbitmap_bpr = dstbitmap->GetBytesPerRow() / 4;
	int tmpbitmap_bpr = tmpbitmap_width;

	float xscale = float ( dstbitmap_width ) / float ( srcbitmap_width );
	float yscale = float ( dstbitmap_height ) / float ( srcbitmap_height );

//--
	contriblist *contriblists = CalcFilterWeight( xscale, filterwidth, srcbitmap_width, dstbitmap_width, filterfunc );

	for( int iy = 0; iy < tmpbitmap_height; iy++ )
	{
		uint32 *src_bits = srcbitmap_bits + iy * srcbitmap_bpr;
		uint32 *dst_bits = tmpbitmap_bits + iy * tmpbitmap_bpr;

		for( int ix = 0; ix < tmpbitmap_width; ix++ )
		{
			int32 rweight;
			int32 gweight;
			int32 bweight;
			int32 aweight;

			rweight = gweight = bweight = aweight = 0;

			for( int ix2 = 0; ix2 < contriblists[ix].contribcnt; ix2++ )
			{
				const contrib & scontrib = contriblists[ix].contribs[ix2];
				uint32 color = src_bits[scontrib.sourcepos];
				int32 weight = scontrib.weight;

				rweight += ( ( color ) & 0xff ) * weight;
				gweight += ( ( color >> 8 ) & 0xff ) * weight;
				bweight += ( ( color >> 16 ) & 0xff ) * weight;
				aweight += ( ( color >> 24 ) & 0xff ) * weight;
			}

			rweight = clamp( rweight >> 16, 0, 255 );
			gweight = clamp( gweight >> 16, 0, 255 );
			bweight = clamp( bweight >> 16, 0, 255 );
			aweight = clamp( aweight >> 16, 0, 255 );
			dst_bits[ix] = ( rweight ) | ( gweight << 8 ) | ( bweight << 16 ) | ( aweight << 24 );
		}
	}

	delete[]contriblists;
//--

	contriblists = CalcFilterWeight( yscale, filterwidth, srcbitmap_height, dstbitmap_height, filterfunc );

	// help cache coherency:
	uint32 *bitmaprow = new uint32[tmpbitmap_height];

	for( int ix = 0; ix < dstbitmap_width; ix++ )
	{
		for( int iy = 0; iy < tmpbitmap_height; iy++ )
			bitmaprow[iy] = tmpbitmap_bits[ix + iy * tmpbitmap_bpr];

		for( int iy = 0; iy < dstbitmap_height; iy++ )
		{
			int32 rweight;
			int32 gweight;
			int32 bweight;
			int32 aweight;

			rweight = gweight = bweight = aweight = 0;

			for( int iy2 = 0; iy2 < contriblists[iy].contribcnt; iy2++ )
			{
				const contrib & scontrib = contriblists[iy].contribs[iy2];
				uint32 color = bitmaprow[scontrib.sourcepos];
				int32 weight = scontrib.weight;

				rweight += ( ( color ) & 0xff ) * weight;
				gweight += ( ( color >> 8 ) & 0xff ) * weight;
				bweight += ( ( color >> 16 ) & 0xff ) * weight;
				aweight += ( ( color >> 24 ) & 0xff ) * weight;
			}
			rweight = clamp( rweight >> 16, 0, 255 );
			gweight = clamp( gweight >> 16, 0, 255 );
			bweight = clamp( bweight >> 16, 0, 255 );
			aweight = clamp( aweight >> 16, 0, 255 );
			dstbitmap_bits[ix + iy * dstbitmap_bpr] = ( rweight ) | ( gweight << 8 ) | ( bweight << 16 ) | ( aweight << 24 );
		}
	}

	delete[]bitmaprow;
	delete[]contriblists;
	delete[]tmpbitmap_bits;
}

//-----------------------------------------------------------------------------

static float Filter_Filter( float t )
{
	/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
	if( t < 0.0f )
		t = -t;
	if( t < 1.0f )
		return ( 2.0f * t - 3.0f ) * t * t + 1.0f;
	return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Box( float t )
{
	if( ( t > -0.5f ) && ( t <= 0.5f ) )
		return 1.0f;
	return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Triangle( float t )
{
	if( t < 0.0f )
		t = -t;
	if( t < 1.0f )
		return 1.0f - t;
	return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_Bell( float t )	/* box (*) box (*) box */
{
	if( t < 0.0f )
		t = -t;
	if( t < 0.5f )
		return 0.75f - ( t * t );
	if( t < 1.5f )
	{
		t = t - 1.5f;
		return 0.5f * ( t * t );
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------

static float Filter_BSpline( float t )	/* box (*) box (*) box (*) box */
{
	float tt;

	if( t < 0.0f )
		t = -t;
	if( t < 1.0f )
	{
		tt = t * t;
		return ( 0.5f * tt * t ) - tt + ( 2.0f / 3.0f );
	}
	else if( t < 2.0f )
	{
		t = 2.0f - t;
		return ( 1.0f / 6.0f ) * ( t * t * t );
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------

static inline float sinc( float x )
{
	x *= 3.1415926f;
	if( x != 0 )
		return sin( x ) / x;
	return 1.0f;
}

static float Filter_Lanczos3( float t )
{
#if 0
	if( t < 0.0f )
		t = -t;
	if( t < 3.0 )
		return sinc( t ) * sinc( t / 3.0f );
	return 0.0f;
#else
	if( t == 0.0f )
		return 1.0f * 1.0f;
	if( t < 3.0f )
	{
		t *= 3.1415926;
//              return sin(t)/t * sin(t/3.0f)/(t/3.0f);
		return 3.0f * sin( t ) * sin( t * ( 1.0f / 3.0f ) ) / ( t * t );
	}
	return 0.0f;
#endif
}

//-----------------------------------------------------------------------------

#define	B (1.0f / 3.0f)
#define	C (1.0f / 3.0f)

static float Filter_Mitchell( float t )
{
	float tt;

	tt = t * t;
	if( t < 0.0f )
		t = -t;
	if( t < 1.0f )
	{
		t = ( ( 12.0f - 9.0f * B - 6.0f * C ) * ( t * tt ) ) + ( ( -18.0f + 12.0f * B + 6.0f * C ) * tt ) + ( 6.0f - 2.0f * B );
		return t / 6.0f;
	}
	else if( t < 2.0f )
	{
		t = ( ( -1.0f * B - 6.0f * C ) * ( t * tt ) ) + ( ( 6.0f * B + 30.0f * C ) * tt ) + ( ( -12.0f * B - 48.0f * C ) * t ) + ( 8.0f * B + 24.0f * C );
		return t / 6.0f;
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
