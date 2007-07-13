#include <stdio.h>
#include <setjmp.h>

#include <atheos/kdebug.h>

#include <util/circularbuffer.h>
#include <translation/translator.h>

#include <vector>

extern "C" {
#define XMD_H
#include <jpeglib.h>
#undef const
}


using namespace os;



const int max_buf = 65536;


static int min( int a, int b ) {
    return( (a<b) ? a : b );
}


struct InputStream : public jpeg_source_mgr {
    InputStream();

    uint8 m_pBuffer[max_buf];

    int 	 m_nValidBufferSize;
    unsigned int m_nBytesToSkip;
    bool 	 m_bEOF;
    bool 	 m_bFinalPass;
    bool	 m_bDecodingDone;
};

struct JPEGError : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

void jpgt_error_exit( j_common_ptr pcInfo )
{
    JPEGError* psError = static_cast<JPEGError*>(pcInfo->err);
    
    char zErrMsg[ JMSG_LENGTH_MAX ];
    (*pcInfo->err->format_message)( pcInfo, zErrMsg );

    dbprintf( "Warning: jpeg translator: %s\n", zErrMsg );
    longjmp( psError->setjmp_buffer, 1 );
}


static void jpgt_decompress_dummy( j_decompress_ptr )
{
}

static boolean jpgt_fill_input_buffer( j_decompress_ptr pcInfo )
{
    InputStream* src = static_cast<InputStream*>(pcInfo->src);

    if ( src->m_bEOF ) {
	  /* Insert a fake EOI marker - as per jpeglib recommendation */
	src->m_pBuffer[0] = (JOCTET) 0xFF;
	src->m_pBuffer[1] = (JOCTET) JPEG_EOI;
	src->bytes_in_buffer = 2;
	return( true );
    } else {
	return( false );  /* I/O suspension mode */
    }
}

static void jpgt_skip_input_data( j_decompress_ptr pcInfo, long nLen )
{
    if ( nLen <= 0 ) {
	return;
    }

    InputStream* src = static_cast<InputStream*>(pcInfo->src);
    src->m_nBytesToSkip += nLen;

    uint nBytesToSkip = min(src->bytes_in_buffer, src->m_nBytesToSkip);


    if ( nBytesToSkip < src->bytes_in_buffer ) {
	memcpy( src->m_pBuffer, src->next_input_byte + nBytesToSkip, src->bytes_in_buffer - nBytesToSkip );
    }
    src->bytes_in_buffer  -= nBytesToSkip;
    src->m_nValidBufferSize  = src->bytes_in_buffer;
    src->m_nBytesToSkip -= nBytesToSkip;

      /* adjust data for jpeglib */
    pcInfo->src->next_input_byte = (JOCTET *) src->m_pBuffer;
    pcInfo->src->bytes_in_buffer = (size_t) src->m_nValidBufferSize;
}


InputStream::InputStream()
{
    init_source	      = jpgt_decompress_dummy;
    fill_input_buffer = jpgt_fill_input_buffer;
    skip_input_data   = jpgt_skip_input_data;
    resync_to_restart = jpeg_resync_to_restart;
    term_source	      = jpgt_decompress_dummy;
    
    bytes_in_buffer  = 0;
    m_nValidBufferSize = 0;
    m_nBytesToSkip = 0;
    m_bEOF	     = 0;
    next_input_byte  = m_pBuffer;
    m_bFinalPass     = false;
    m_bDecodingDone  = false;
}


class JPEGTrans : public Translator
{
public:
    JPEGTrans();
    virtual ~JPEGTrans();

    virtual void     SetConfig( const Message& cConfig );
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
private:
    BitmapHeader       m_sBitmapHeader;
    BitmapFrameHeader  m_sCurrentFrame;
    CircularBuffer     m_cOutBuffer;
    bool	       m_bTopHeaderValid;
    bool	       m_bTopHeaderRead;
    bool	       m_bDone;
    uint8*	       m_pLineBuffer;
    uint32*	       m_pConvertedBuffer;    
    enum {
        STATE_INIT,
        STATE_START_DECOMPRESS,
        STATE_DECOMPRESSING,
        STATE_DO_OUTPUT_SCAN,
    } m_eState;

    // structs for the jpeglib
    struct jpeg_decompress_struct m_sDecompressStruct;
    struct JPEGError	m_sJPGError;
    struct InputStream  m_sJPGSource;
    
};




JPEGTrans::JPEGTrans()
{
    m_pLineBuffer       = NULL;
    m_pConvertedBuffer	= NULL;
    m_eState		= STATE_INIT;
    m_bTopHeaderValid   = false;
    m_bTopHeaderRead    = false;
    m_bDone		= false;
    
    memset(&m_sDecompressStruct, 0, sizeof(m_sDecompressStruct));
    m_sDecompressStruct.err = jpeg_std_error(&m_sJPGError);

    jpeg_create_decompress(&m_sDecompressStruct);
    
    m_sDecompressStruct.err = jpeg_std_error(&m_sJPGError);
    m_sJPGError.error_exit  = jpgt_error_exit;
    m_sDecompressStruct.src = &m_sJPGSource;
    
}

JPEGTrans::~JPEGTrans()
{
    delete[] m_pLineBuffer;
    delete[] m_pConvertedBuffer;    
    jpeg_destroy_decompress(&m_sDecompressStruct);
}

void JPEGTrans::SetConfig( const Message& cConfig )
{
}

status_t JPEGTrans::AddData( const void* pData, size_t _nLen, bool bFinal )
{
    m_bDone = bFinal;
    if( m_sJPGSource.m_bEOF ) {
        return( 0 );
    }
    if( setjmp( m_sJPGError.setjmp_buffer ) ) {
	  // FIXME:: Signal the error
        return -1;
    }
    size_t nLen = _nLen;
    
    while( nLen > 0 || (bFinal && m_sJPGSource.m_bEOF == false) ) {
	int nCurLen = min( nLen, max_buf - m_sJPGSource.m_nValidBufferSize );
	nLen -= nCurLen;

	  // filling buffer with the new data
	memcpy( m_sJPGSource.m_pBuffer + m_sJPGSource.m_nValidBufferSize, pData, nCurLen );
	m_sJPGSource.m_nValidBufferSize += nCurLen;

	if ( m_sJPGSource.m_nBytesToSkip ) {
	    int nBytesToSkip = min( m_sJPGSource.m_nValidBufferSize, m_sJPGSource.m_nBytesToSkip );

	    if ( nBytesToSkip < m_sJPGSource.m_nValidBufferSize) {
		memcpy( m_sJPGSource.m_pBuffer, m_sJPGSource.m_pBuffer + nBytesToSkip, m_sJPGSource.m_nValidBufferSize - nBytesToSkip );
	    }
	
	    m_sJPGSource.m_nValidBufferSize -= nBytesToSkip;
	    m_sJPGSource.m_nBytesToSkip -= nBytesToSkip;

	      // still more bytes to skip
	    if ( m_sJPGSource.m_nBytesToSkip ) {
		if( nCurLen <= 0 ) {
		    printf( "Error: JPEGTrans::AddData() got negative m_nBytesToSkip\n" );
		}
		return( 0 );
	    }
	}
	m_sDecompressStruct.src->next_input_byte = (JOCTET*) m_sJPGSource.m_pBuffer;
	m_sDecompressStruct.src->bytes_in_buffer = (size_t) m_sJPGSource.m_nValidBufferSize;

	if( m_eState == STATE_INIT ) {
	    if( jpeg_read_header( &m_sDecompressStruct, true ) != JPEG_SUSPENDED ) {
		m_eState = STATE_START_DECOMPRESS;
	    } else {
		if ( bFinal ) {
		    m_sJPGSource.m_bEOF = true;
		}
	    }
	}

	if( m_eState == STATE_START_DECOMPRESS ) {
	    m_sDecompressStruct.buffered_image      = true;
	    m_sDecompressStruct.do_fancy_upsampling = false;
	    m_sDecompressStruct.do_block_smoothing  = false;
	    m_sDecompressStruct.dct_method          = JDCT_FASTEST;

	      // FALSE: IO suspension
	    if( jpeg_start_decompress(&m_sDecompressStruct) ) {
		m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
		m_sBitmapHeader.bh_data_size = 0;
		m_sBitmapHeader.bh_bounds = IRect( 0, 0, m_sDecompressStruct.output_width - 1, m_sDecompressStruct.output_height - 1 );
		m_sBitmapHeader.bh_frame_count = 1;
		m_sBitmapHeader.bh_bytes_per_row = m_sDecompressStruct.output_width * 4;
		m_sBitmapHeader.bh_color_space = CS_RGB32;


		m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
		m_sCurrentFrame.bf_time_stamp    = 0;
		m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
		m_sCurrentFrame.bf_frame	 = m_sBitmapHeader.bh_bounds;
		m_sCurrentFrame.bf_bytes_per_row = m_sDecompressStruct.output_width * 4;
		m_sCurrentFrame.bf_data_size     = m_sCurrentFrame.bf_bytes_per_row * (m_sCurrentFrame.bf_frame.Height() + 1);

		m_bTopHeaderValid = true;
		
		m_pLineBuffer = new uint8[m_sDecompressStruct.output_width * m_sDecompressStruct.output_components];
		if( m_sDecompressStruct.output_components == 3 )
			m_pConvertedBuffer = new uint32[m_sDecompressStruct.output_width];
		m_eState = STATE_DECOMPRESSING;
	    } else {
		if ( bFinal ) {
		    m_sJPGSource.m_bEOF = true;
		}
	    }
	}

	if( m_eState == STATE_DECOMPRESSING ) {
	    m_sDecompressStruct.buffered_image = true;
	    jpeg_start_output(&m_sDecompressStruct, m_sDecompressStruct.input_scan_number);
	    m_eState = STATE_DO_OUTPUT_SCAN;
	    m_cOutBuffer.Write( &m_sCurrentFrame, sizeof(m_sCurrentFrame) );
	}

	if ( m_eState == STATE_DO_OUTPUT_SCAN ) {
	    if( m_sJPGSource.m_bDecodingDone ) {
		return( 0 );
	    }
	
	    while( m_sDecompressStruct.output_scanline < m_sDecompressStruct.output_height ) {
		if ( jpeg_read_scanlines(&m_sDecompressStruct, &m_pLineBuffer, 1) != 1 ) {
		    break;
		}
		if ( m_sDecompressStruct.jpeg_color_space == JCS_GRAYSCALE ) {
		    for ( uint x = 0 ; x < m_sDecompressStruct.output_width ; ++x ) {
			for ( int i = 0 ; i < 3 ; ++i ) {
			    m_cOutBuffer.Write( m_pLineBuffer + x, 1 );
			}
			uint8 nAlpha = 0xff;
			m_cOutBuffer.Write( &nAlpha, 1 );
		    }
		} else if ( m_sDecompressStruct.num_components == 3 ) {
		    for ( uint x = 0 ; x < m_sDecompressStruct.output_width ; ++x ) {
		    m_pConvertedBuffer[x] = m_pLineBuffer[x*3+2] | ( m_pLineBuffer[x*3+1] << 8 )
		    | ( m_pLineBuffer[x*3] << 16 ) | 0xff000000;
		    }
			m_cOutBuffer.Write( (uint8*)m_pConvertedBuffer, 4 * m_sDecompressStruct.output_width );		    
		} else {
		    for ( uint x = 0 ; x < m_sDecompressStruct.output_width ; ++x ) {
			for ( int i = 0 ; i < 4 ; ++i ) {
			    m_cOutBuffer.Write( m_pLineBuffer + x*4+(3-i), 1 );
			}
		    }
		}
	    }

	    if ( m_sDecompressStruct.output_scanline >= m_sDecompressStruct.output_height ) {
		jpeg_finish_output(&m_sDecompressStruct);
		m_sJPGSource.m_bFinalPass = jpeg_input_complete(&m_sDecompressStruct);
		m_sJPGSource.m_bDecodingDone = m_sJPGSource.m_bFinalPass && m_sDecompressStruct.input_scan_number == m_sDecompressStruct.output_scan_number;

		if ( m_sJPGSource.m_bDecodingDone == false ) {
		    m_eState = STATE_DECOMPRESSING;
		}
	    }

	    if ( m_eState == STATE_DO_OUTPUT_SCAN && m_sJPGSource.m_bDecodingDone ) {
		jpeg_finish_decompress(&m_sDecompressStruct);
		m_sJPGSource.m_bEOF = true;
		return 0;
	    }
	}
	if ( m_sJPGSource.bytes_in_buffer ) {
	    memcpy( m_sJPGSource.m_pBuffer, m_sJPGSource.next_input_byte, m_sJPGSource.bytes_in_buffer );
	}
	m_sJPGSource.m_nValidBufferSize = m_sJPGSource.bytes_in_buffer;
    }
    return( 0 );
}

ssize_t JPEGTrans::AvailableDataSize()
{
    return( m_cOutBuffer.Size() );
}

ssize_t JPEGTrans::Read( void* pData, size_t nLen )
{
    int nBytesRead = 0;
    if ( m_bTopHeaderValid == false ) {
	return( ERR_SUSPENDED );
    }
    if (  m_bTopHeaderRead == false ) {
	if ( nLen < sizeof( BitmapHeader ) ) {
	    errno = EINVAL;
	    return( -1 );
	}
	memcpy( pData, &m_sBitmapHeader, sizeof(m_sBitmapHeader) );
	pData = ((char*)pData) + sizeof(m_sBitmapHeader);
	nLen       -= sizeof(m_sBitmapHeader);
	nBytesRead += sizeof(m_sBitmapHeader);
	m_bTopHeaderRead = true;
    }

    uint nCurLen = m_cOutBuffer.Read( pData, nLen );
    nBytesRead += nCurLen;
    if ( nBytesRead == 0 && m_bDone == false ) {
	return( ERR_SUSPENDED );
    }
    return( nBytesRead );
}

void JPEGTrans::Abort()
{
}

void JPEGTrans::Reset()
{
}

class JPEGTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( nLen < 3 ) {
	    return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
	}
	const uint8* p = (uint8*)pData;
	if( p[0] == 0377 && p[1] == 0330 && p[2] == 0377 ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { "image/jpeg", TRANSLATOR_TYPE_IMAGE, 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new JPEGTrans );
    }
};



extern "C" {
#if 0
}
#endif

int get_translator_count()
{
    return( 1 );
}

TranslatorNode* get_translator_node( int nIndex )
{
    static JPEGTransNode sNode;
    return( &sNode );
}
    
};
