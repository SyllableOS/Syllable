#include <stdio.h>
#include <setjmp.h>

#include <atheos/kdebug.h>
#include <gui/bitmap.h>		// BitsPerPixel()
#include <util/circularbuffer.h>
#include <translation/translator.h>

#include <vector>
#include <new>
#include <png.h>

using namespace os;

class PNGTrans : public Translator
{
public:
    PNGTrans( bool bWriteMode = false );
    virtual ~PNGTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();


    int InfoCallback();
    int RowCallback( int nPass, int y, png_bytep pRow );
    int EndCallback();
    void WriteCallback( png_bytep pData, int nSize );
    
private:
    void AddProcessedData( void* pData, int nLen ) {
    	static int total = 0;
	m_cOutBuffer.Write( pData, nLen );
    }
    void AllocRowBuffer( int nPixels ) {
	if( m_pRowBuffer) delete m_pRowBuffer;
    	m_pRowBuffer = new png_byte[nPixels * 3];
    	m_nRowSize = nPixels * 3;
    }
    bool AppendToRowBuffer( void );

    BitmapHeader	m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;
    CircularBuffer  	m_cOutBuffer;
    CircularBuffer  	m_cInBuffer;		// Only for writing
    
    int		   	m_nNumPasses;
    bool		m_bIsInterlaced;
    enum {
        STATE_INIT,
        STATE_START_DECOMPRESS,
        STATE_DECOMPRESSING,
        STATE_DO_OUTPUT_SCAN,
        STATE_FRAMEHDR,
        STATE_READING
    } m_eState;

    png_structp    	m_psPNGStruct;
    png_infop	   	m_psPNGInfo;
    png_byte*		m_pRowBuffer;
    int			m_nRowPos;
    int			m_nRowSize;
    
    bool		m_bWriteMode;
};

static void write_callback( png_structp png_ptr, png_bytep data, png_size_t size )
{
    PNGTrans* psTrans = (PNGTrans*)png_get_io_ptr( png_ptr );
    psTrans->WriteCallback( data, size );
}

static void info_callback( png_structp png_ptr, png_infop info )
{
    PNGTrans* psTrans = (PNGTrans*)png_get_progressive_ptr( png_ptr );
    psTrans->InfoCallback();
}

static void row_callback( png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass )
{
    PNGTrans* psTrans = (PNGTrans*)png_get_progressive_ptr( png_ptr );

//    png_progressive_combine_row(png_ptr, old_row, new_row);
    psTrans->RowCallback( pass, row_num, new_row );
/* this function is called for every row in the image.  If the
 * image is interlacing, and you turned on the interlace handler,
 * this function will be called for every row in every pass.
 * Some of these rows will not be changed from the previous pass.
 * When the row is not changed, the new_row variable will be NULL.
 * The rows and passes are called in order, so you don't really
 * need the row_num and pass, but I'm supplying them because it
 * may make your life easier.
 *
 * For the non-NULL rows of interlaced images, you must call
 * png_progressive_combine_row() passing in the row and the
 * old row.  You can call this function for NULL rows (it will
 * just return) and for non-interlaced images (it just does the
 * memcpy for you) if it will make the code easier.  Thus, you
 * can just do this for all cases:
 */


/* where old_row is what was displayed for previous rows.  Note
 * that the first pass (pass == 0 really) will completely cover
 * the old row, so the rows do not have to be initialized.  After
 * the first pass (and only for interlaced images), you will have
 * to pass the current row, and the function will combine the
 * old row and the new row.
 */
}

static void end_callback(png_structp png_ptr, png_infop info)
{
    PNGTrans* psTrans = (PNGTrans*)png_get_progressive_ptr( png_ptr );
    psTrans->EndCallback();
/* this function is called when the whole image has been read,
 * including any chunks after the image (up to and including
 * the IEND).  You will usually have the same info chunk as you
 * had in the header, although some data may have been added
 * to the comments and time fields.
 *
 * Most people won't do much here, perhaps setting a flag that
 * marks the image as finished.
 */
}



PNGTrans::PNGTrans( bool bWriteMode )
{
	m_eState		= STATE_INIT;
	m_bIsInterlaced		= false;
	m_bWriteMode		= bWriteMode;
	m_pRowBuffer		= NULL;
	m_nRowPos		= 0;

	//dbprintf( "PNG: WriteMode = %d\n", bWriteMode );

	if( bWriteMode ) {
		m_psPNGStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		if ( m_psPNGStruct == NULL ) {
			throw std::bad_alloc();
		}

		m_psPNGInfo = png_create_info_struct( m_psPNGStruct );

		if ( m_psPNGInfo == NULL )
		{
			png_destroy_write_struct( &m_psPNGStruct, (png_infopp)NULL);
			throw std::bad_alloc();
		}

		if ( setjmp( m_psPNGStruct->jmpbuf ) ) {
			png_destroy_write_struct( &m_psPNGStruct, (png_infopp)NULL);
			throw std::bad_alloc();
		}

		png_set_write_fn( m_psPNGStruct, (void *)this, write_callback, NULL);
		
		//dbprintf( "PNG: Write mode initialized\n" );
	} else {
		m_psPNGStruct = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		if ( m_psPNGStruct == NULL ) {
			throw std::bad_alloc();
		}

		m_psPNGInfo = png_create_info_struct( m_psPNGStruct );

		if ( m_psPNGInfo == NULL )
		{
			png_destroy_read_struct( &m_psPNGStruct, &m_psPNGInfo, (png_infopp)NULL);
			throw std::bad_alloc();
		}

		if ( setjmp( m_psPNGStruct->jmpbuf ) ) {
			png_destroy_read_struct( &m_psPNGStruct, &m_psPNGInfo, (png_infopp)NULL );
			throw std::bad_alloc();
		}
   
		/* This one's new.  You will need to provide all three
		 * function callbacks, even if you aren't using them all.
		 * If you aren't using all functions, you can specify NULL
		 * parameters.  Even when all three functions are NULL,
		 * you need to call png_set_progressive_read_fn().
		 * These functions shouldn't be dependent on global or
		 * static variables if you are decoding several images
		 * simultaneously.  You should store stream specific data
		 * in a separate struct, given as the second parameter,
		 * and retrieve the pointer from inside the callbacks using
		 * the function png_get_progressive_ptr(png_ptr).
		 */
		png_set_progressive_read_fn( m_psPNGStruct, (void *)this, info_callback, row_callback, end_callback );
	}
}

PNGTrans::~PNGTrans()
{
	if( m_bWriteMode ) {
		png_destroy_write_struct( &m_psPNGStruct, (png_infopp)NULL);
		if( m_pRowBuffer) delete m_pRowBuffer;
	} else {
		png_destroy_read_struct( &m_psPNGStruct, &m_psPNGInfo, (png_infopp)NULL );
	}
}


int PNGTrans::InfoCallback()
{
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    
    png_get_IHDR( m_psPNGStruct, m_psPNGInfo, &width, &height, &bit_depth, &color_type,
		  &interlace_type, NULL, NULL);

    m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
    m_sBitmapHeader.bh_data_size = 0;
    m_sBitmapHeader.bh_bounds = IRect( 0, 0, width - 1, height - 1 );
    m_sBitmapHeader.bh_frame_count = 1;
    m_sBitmapHeader.bh_bytes_per_row = width * 4;
    m_sBitmapHeader.bh_color_space = CS_RGB32;

    AddProcessedData( &m_sBitmapHeader, sizeof(m_sBitmapHeader) );

    m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
    m_sCurrentFrame.bf_time_stamp    = 0;
    m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
    m_sCurrentFrame.bf_frame	 = m_sBitmapHeader.bh_bounds;
    m_sCurrentFrame.bf_bytes_per_row = (m_sCurrentFrame.bf_frame.Width() + 1) * 4;
    m_sCurrentFrame.bf_data_size     = m_sCurrentFrame.bf_bytes_per_row * (m_sCurrentFrame.bf_frame.Height() + 1);
    
    if ( interlace_type == PNG_INTERLACE_NONE ) {
	
	AddProcessedData( &m_sCurrentFrame, sizeof(m_sCurrentFrame) );
	m_bIsInterlaced = false;
    } else {
	m_bIsInterlaced = true;
    }
      /* expand paletted colors into true RGB triplets */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_expand( m_psPNGStruct );

      /* expand paletted or RGB images with transparency to full alpha channels
       * so the data will be available as RGBA quartets */
    if ( png_get_valid( m_psPNGStruct, m_psPNGInfo, PNG_INFO_tRNS) )
	png_set_expand( m_psPNGStruct );

      /* Set the background color to draw transparent and alpha images over.
       * It is possible to set the red, green, and blue components directly
       * for paletted images instead of supplying a palette index.  Note that
       * even if the PNG file supplies a background, you are not required to
       * use it - you should use the (solid) application background if it has one.
       */
    double screen_gamma;
//    png_color_16* image_background;

//    if ( png_get_bKGD( m_psPNGStruct, m_psPNGInfo, &image_background ) ) {
//	png_set_background( m_psPNGStruct, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0 );
//    }
    screen_gamma = 2.2;  /* A good guess for PC monitors */

    double image_gamma;
    if ( png_get_gAMA( m_psPNGStruct, m_psPNGInfo, &image_gamma) )
	png_set_gamma( m_psPNGStruct, screen_gamma, image_gamma);
    else
	png_set_gamma( m_psPNGStruct, screen_gamma, 0.45);


      /* flip the RGB pixels to BGR (or RGBA to BGRA) */
    png_set_bgr( m_psPNGStruct );

      /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
//  png_set_swap_alpha( m_psPNGStruct );

      /* swap bytes of 16 bit files to least significant byte first */
//  png_set_swap( m_psPNGStruct );

      /* Add filler (or alpha) byte (before/after each RGB triplet) */
    png_set_filler( m_psPNGStruct, 0xff, PNG_FILLER_AFTER);

    png_set_gray_to_rgb( m_psPNGStruct );

      /* Turn on interlace handling.  REQUIRED if you are not using
       * png_read_image().  To see how to handle interlacing passes,
       * see the png_read_row() method below.
       */
    m_nNumPasses = png_set_interlace_handling( m_psPNGStruct );

      /* optional call to gamma correct and add the background to the palette
       * and update info structure.  REQUIRED if you are expecting libpng to
       * update the palette for you (ie you selected such a transform above).
       */
    png_read_update_info( m_psPNGStruct, m_psPNGInfo);
    return 0;
}

int PNGTrans::RowCallback( int nPass, int y, png_bytep pRow )
{
    if ( pRow == NULL ) {
	return( 0 );
    }
    if ( m_bIsInterlaced ) {
	m_sCurrentFrame.bf_frame.top	 = y;
	m_sCurrentFrame.bf_frame.bottom	 = y;
	m_sCurrentFrame.bf_data_size     = m_sCurrentFrame.bf_bytes_per_row;
	
	AddProcessedData( &m_sCurrentFrame, sizeof(m_sCurrentFrame) );
    }
    AddProcessedData( pRow, m_sBitmapHeader.bh_bytes_per_row );
    return 0;
}

int PNGTrans::EndCallback()
{
    return 0;
}

void PNGTrans::WriteCallback( png_bytep pData, int nSize )
{
	//dbprintf( "WriteCallback( %p, %d)\n", pData, nSize );
	AddProcessedData( pData, nSize );
}

bool PNGTrans::AppendToRowBuffer( void )
{
	uint8	buffer[1024];

	// Free space in rowbuffer
	uint32	nSize = m_nRowSize - m_nRowPos;
	// Adjust to available amount of data (so we don't get a deadlock)
	nSize = nSize > (uint32)m_cInBuffer.Size() ? m_cInBuffer.Size() : nSize;
	// Longword alignment, 24 bpp => 32 bpp size conversion
	nSize = ((int)( nSize / 3 )) * 4;
	// Adjust to available buffer size
	nSize = nSize > sizeof( buffer ) ? sizeof(buffer) : nSize;

//	dbprintf("pos: %ld, size: %ld\n", m_nRowPos, m_nRowSize);


	// Read and convert RGB32 => RGB24
//	dbprintf("m_cInBuffer.Read( %p, %ld )...", buffer, nSize);
	m_cInBuffer.Read( buffer, nSize );
//	dbprintf("Done!\n");
	int i, j;
	for( i = 0, j = 0; i < nSize; i+=4, j+=3 ) {
		m_pRowBuffer[ m_nRowPos + j     ] = buffer[ i     ];
		m_pRowBuffer[ m_nRowPos + j + 1 ] = buffer[ i + 1 ];
		m_pRowBuffer[ m_nRowPos + j + 2 ] = buffer[ i + 2 ];
	}
	
	m_nRowPos += (nSize/4)*3;
	if( m_nRowPos >= m_nRowSize ) {
//		dbprintf("New row\n");
		m_nRowPos = 0;
		return true;
	} else {
		return false;
	}
}

status_t PNGTrans::AddData( const void* pData, size_t nLen, bool bFinal )
{
	if( !m_bWriteMode ) {
	//	dbprintf("PNG-Read: AddData()\n");
		if ( setjmp( m_psPNGStruct->jmpbuf ) ) {
			png_destroy_read_struct( &m_psPNGStruct, &m_psPNGInfo, (png_infopp)NULL );
			return -1;
		}
	    
		png_process_data( m_psPNGStruct, m_psPNGInfo, (png_bytep)pData, nLen );
		return 0;
	} else {
		m_cInBuffer.Write( pData, nLen );
	//	dbprintf("PNG-Write: Data added!\n");
		if( m_eState == STATE_INIT && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapHeader ) ) {
			m_cInBuffer.Read( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );
			m_eState = STATE_FRAMEHDR;
		}

		if( m_eState == STATE_FRAMEHDR && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapFrameHeader ) ) {
			m_cInBuffer.Read( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );
			// TODO: Verify depth, convert if not RGB32
			png_set_IHDR( m_psPNGStruct, m_psPNGInfo,
				m_sCurrentFrame.bf_frame.right - m_sCurrentFrame.bf_frame.left + 1,
				m_sCurrentFrame.bf_frame.bottom - m_sCurrentFrame.bf_frame.top + 1,
				8, PNG_COLOR_TYPE_RGB, //_ALPHA,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
		//	png_set_filler( m_psPNGStruct, 0xff, PNG_FILLER_AFTER);
			png_set_bgr( m_psPNGStruct );
			png_write_info( m_psPNGStruct, m_psPNGInfo );
			AllocRowBuffer( m_sCurrentFrame.bf_frame.right - m_sCurrentFrame.bf_frame.left + 1 );
			m_eState = STATE_READING;
		}
		
		while( m_eState == STATE_READING && m_cInBuffer.Size() ) {
			if( AppendToRowBuffer() ) {
				// Overflow = one row is ready
			//	dbprintf("PNG-Write: Found Row\n");
				png_write_rows( m_psPNGStruct, &m_pRowBuffer, 1);
			}
		}

		if( bFinal ) {
			png_write_end( m_psPNGStruct, m_psPNGInfo);
		}

		return 0;
	}
}

ssize_t PNGTrans::AvailableDataSize()
{
    return( m_cOutBuffer.Size() );
}

ssize_t PNGTrans::Read( void* pData, size_t nLen )
{
    ssize_t nCurLen = m_cOutBuffer.Read( pData, nLen );
    if ( nCurLen == 0 /*&& m_bDone == false*/ ) {
	return( ERR_SUSPENDED );
    }
    return( nCurLen );
}

void PNGTrans::Abort()
{
}

void PNGTrans::Reset()
{
}

class PNGReaderNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( nLen < 4 ) {
	    return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
	}
	if ( png_check_sig( (png_bytep)pData, nLen ) ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { "image/png", TRANSLATOR_TYPE_IMAGE, 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new PNGTrans );
    }
};

class PNGWriterNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( cSrcType == TRANSLATOR_TYPE_IMAGE && cDstType == "image/png" ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { TRANSLATOR_TYPE_IMAGE, "image/png", 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new PNGTrans( true ) );
    }
};

extern "C" {
#if 0
}
#endif

int get_translator_count()
{
    return( 2 );
}

TranslatorNode* get_translator_node( int nIndex )
{
    static PNGWriterNode sWrNode;
    static PNGReaderNode sRdNode;
    switch( nIndex ) {
    	case 1:
	    return( &sWrNode );
	default:
	    return( &sRdNode );
    }
}
    
};


