#include <stdio.h>
#include <setjmp.h>

#include <atheos/kdebug.h>

#include <util/circularbuffer.h>
#include <translation/translator.h>

#include <vector>
#include <new>

extern "C" {
#include <gif_lib.h>
}


using namespace os;



class GIFTrans : public Translator
{
public:
    GIFTrans();
    virtual ~GIFTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();

    int GetInput( void* pData, int nLen );
   
private:
    void DecodeData();
    void AddLine( ColorMapObject* ColorMap, int y, GifPixelType* pLine );

    
    BitmapHeader       m_sBitmapHeader;
    BitmapFrameHeader  m_sCurrentFrame;
    CircularBuffer     m_cOutBuffer;
    CircularBuffer     m_cInBuffer;
    int		       m_nNumLoadedLines;
    int		       m_nLastReadRow;
    
    bool	       m_bTopHeaderValid;
    bool	       m_bTopHeaderRead;
    bool	       m_bFrameHeaderValid;
    bool	       m_bFrameHeaderRead;
    uint8*	       m_pLineBuffer;
    int		       m_nNumPasses;
    uint32	       m_nTransCol;
    bool	       m_bEOF;
    enum {
        STATE_INIT,
        STATE_START_DECOMPRESS,
        STATE_DECOMPRESSING,
        STATE_DO_OUTPUT_SCAN,
    } m_eState;

    GifFileType* m_psGIFStruct;
};


static int get_input( GifFileType* psGIFStruct, GifByteType* pData, int nLen )
{
    return( ((GIFTrans*)psGIFStruct->UserData)->GetInput( pData, nLen ) );
}


GIFTrans::GIFTrans()
{
    m_pLineBuffer       = NULL;
    m_eState		= STATE_INIT;
    m_bTopHeaderValid   = false;
    m_bTopHeaderRead    = false;
    m_bFrameHeaderValid = false;
    m_bFrameHeaderRead  = false;
    m_nNumLoadedLines   = 0;
    m_nLastReadRow      = 0;
    m_nTransCol	       	= ~0;
    m_psGIFStruct = NULL;
    m_bEOF = false;
};

GIFTrans::~GIFTrans()
{
}

int GIFTrans::GetInput( void* pData, int nLen )
{
    return( m_cInBuffer.Read( pData, nLen ) );
}

status_t GIFTrans::AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
    return( 0 );
}



ssize_t GIFTrans::AvailableDataSize()
{
    return( m_cOutBuffer.Size() );
}


void GIFTrans::AddLine( ColorMapObject* ColorMap, int y, GifPixelType* pLine )
{
    int nSpaceNeeded = m_sBitmapHeader.bh_bytes_per_row;
    if (m_psGIFStruct->Image.Interlace) {
	m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 = Rect( 0, y, m_sBitmapHeader.bh_bounds.right, y );
	m_sCurrentFrame.bf_bytes_per_row = (m_sCurrentFrame.bf_frame.Width() + 1) * 4;
	m_sCurrentFrame.bf_data_size     = m_sCurrentFrame.bf_bytes_per_row * (m_sCurrentFrame.bf_frame.Height() + 1);
	nSpaceNeeded += sizeof(m_sCurrentFrame);
    }
    if (m_psGIFStruct->Image.Interlace) {
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof(m_sCurrentFrame) );
    }
    for ( int j = 0; j < m_sBitmapHeader.bh_bounds.Width() + 1 ; ++j ) {
	GifColorType* ColorMapEntry = &ColorMap->Colors[pLine[j]];
	uint8 nAlpha;
	if ( pLine[j] == m_nTransCol /*m_psGIFStruct->SBackGroundColor*/ ) {
	    nAlpha = 0x00;
	} else {
	    nAlpha = 0xff;
	}
	uint8 anBuffer[] = { ColorMapEntry->Blue, ColorMapEntry->Green, ColorMapEntry->Red, nAlpha };
	m_cOutBuffer.Write( anBuffer, 4 );
    }
    m_nNumLoadedLines++;
}

void GIFTrans::DecodeData()
{
    if ( m_psGIFStruct != NULL ) {
	return;
    }
    m_psGIFStruct = DGifOpen( this, get_input );
    int ImageNum = 0;
    GifPixelType* pLineBuffer = new GifPixelType[m_psGIFStruct->SWidth * 4];
    GifRecordType RecordType;
    do {
	if (DGifGetRecordType( m_psGIFStruct, &RecordType) == GIF_ERROR) {
	    goto error;
	}
	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
	    {
		if ( DGifGetImageDesc( m_psGIFStruct ) == GIF_ERROR ) {
		    goto error;
		}
		int Row = m_psGIFStruct->Image.Top; /* Image Position relative to Screen. */
		int Col = m_psGIFStruct->Image.Left;
		int Width = m_psGIFStruct->Image.Width;
		int Height = m_psGIFStruct->Image.Height;

		m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
		m_sBitmapHeader.bh_data_size = 0;
		m_sBitmapHeader.bh_bounds = IRect( 0, 0, Width - 1, Height - 1 );
		m_sBitmapHeader.bh_frame_count = 1;
		m_sBitmapHeader.bh_bytes_per_row = Width * 4;
		m_sBitmapHeader.bh_color_space = CS_RGB32;
		
		m_bTopHeaderValid = true;
		
		
		ImageNum++;

		if (m_psGIFStruct->Image.Left + m_psGIFStruct->Image.Width > m_psGIFStruct->SWidth ||
		    m_psGIFStruct->Image.Top + m_psGIFStruct->Image.Height > m_psGIFStruct->SHeight) {
		    fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
		    goto error;
		}
		if (m_psGIFStruct->Image.Interlace) {
		    int InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
		    int InterlacedJumps[]  = { 8, 8, 4, 2 }; /* be read - offsets and jumps... */
		    
		      /* Need to perform 4 passes on the images: */
		    for ( int i = 0; i < 4; i++) {
			for ( int j = Row + InterlacedOffset[i]; j < Row + Height ; j += InterlacedJumps[i] ) {
			    memset( pLineBuffer, 255/* m_psGIFStruct->SBackGroundColor*/, m_psGIFStruct->SWidth );
			    if (DGifGetLine(m_psGIFStruct, pLineBuffer + Col, Width) != GIF_ERROR) {
				AddLine( (m_psGIFStruct->Image.ColorMap ? m_psGIFStruct->Image.ColorMap : m_psGIFStruct->SColorMap), j, pLineBuffer );
			    } else {
				goto error;
			    }
			}
		    }
		}
		else {
		    m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
		    m_sCurrentFrame.bf_time_stamp    = 0;
		    m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
		    m_sCurrentFrame.bf_frame	     = Rect( m_psGIFStruct->Image.Left, m_psGIFStruct->Image.Top,
							     m_psGIFStruct->Image.Left + m_psGIFStruct->Image.Width - 1,
							     m_psGIFStruct->Image.Top + m_psGIFStruct->Image.Height - 1 );
		    m_sCurrentFrame.bf_bytes_per_row = (m_sCurrentFrame.bf_frame.Width() + 1) * 4;
		    m_sCurrentFrame.bf_data_size     = m_sCurrentFrame.bf_bytes_per_row * (m_sCurrentFrame.bf_frame.Height() + 1);
		    
		    m_cOutBuffer.Write( &m_sCurrentFrame, sizeof(m_sCurrentFrame) );
    
		    
		    for ( int i = 0; i < Height; i++ ) {
			memset( pLineBuffer, m_psGIFStruct->SBackGroundColor, m_psGIFStruct->SWidth );
			if ( DGifGetLine(m_psGIFStruct, pLineBuffer + Col, Width) != GIF_ERROR ) {
			    AddLine( (m_psGIFStruct->Image.ColorMap ? m_psGIFStruct->Image.ColorMap : m_psGIFStruct->SColorMap), Row++, pLineBuffer );
			} else {
			    goto error;
			}
		    }
		}
		break;
	    }
	    case EXTENSION_RECORD_TYPE:
	    {
		GifByteType *Extension;
		int	     ExtCode;
		  /* Skip any extension blocks in file: */
		if (DGifGetExtension(m_psGIFStruct, &ExtCode, &Extension) == GIF_ERROR) {
		    goto error;
		}
		if ( ExtCode == GRAPHICS_EXT_FUNC_CODE ) {
//		    if (DGifGetExtensionNext(m_psGIFStruct, &Extension) == GIF_ERROR) {
//			goto error;
//		    }
		    if ( Extension[1] & 0x01 ) {
			m_nTransCol = Extension[4];
		    }
		}
		while (Extension != NULL) {
		    if (DGifGetExtensionNext(m_psGIFStruct, &Extension) == GIF_ERROR) {
			goto error;
		    }
		}
		break;
	    }
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		    /* Should be traps by DGifGetRecordType. */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE && ImageNum == 0);
    delete[] pLineBuffer;
    DGifCloseFile( m_psGIFStruct );
    return;
error:
    DGifCloseFile( m_psGIFStruct );
    delete[] pLineBuffer;
    return;
}

ssize_t GIFTrans::Read( void* pData, size_t nLen )
{
    if ( m_bEOF == false ) {
	return( ERR_SUSPENDED );
    }
    DecodeData();
    int nBytesRead = 0;
    if ( m_bTopHeaderValid == false ) {
	return( 0 );
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

    int nCurLen = m_cOutBuffer.Read( pData, nLen );
    nBytesRead += nCurLen;
    return( nBytesRead );
}

void GIFTrans::Abort()
{
}

void GIFTrans::Reset()
{
}

class GIFTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( nLen < 3 ) {
	    return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
	}
	const uint8* p = (const uint8*)pData;
	if( p[0] == 'G' && p[1] == 'I' && p[2] == 'F' ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { "image/gif", TRANSLATOR_TYPE_IMAGE, 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new GIFTrans );
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
    static GIFTransNode sNode;
    return( &sNode );
}
    
};
