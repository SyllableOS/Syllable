#include <stdio.h>
#include <setjmp.h>

#include <atheos/kdebug.h>

#include <gui/icon.h>
#include <gui/bitmap.h>		// BitsPerPixel
#include <util/circularbuffer.h>
#include <translation/translator.h>

#include <vector>
#include <new>
#include <string>

using namespace os;

class IconTrans : public Translator
{
public:
    IconTrans( bool bWriteMode = false );
    virtual ~IconTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
  
private:
    bool		m_bWriteMode;
    bool		m_bFirstFrame;
    BitmapHeader	m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;
    CircularBuffer	m_cOutBuffer;
    CircularBuffer	m_cInBuffer;
    IconDir		m_sIconDir;
    IconHeader		m_sIconHdr;
    
    enum {
        STATE_INIT,
        STATE_ICONHDR,
        STATE_FRAMEHDR,
        STATE_READING
    } m_eState;
};

IconTrans::IconTrans( bool bWriteMode )
{
	m_bWriteMode = bWriteMode;
	m_eState = STATE_INIT;
}

IconTrans::~IconTrans()
{

}

status_t IconTrans::AddData( const void* pData, size_t nLen, bool bFinal )
{
	m_cInBuffer.Write( pData, nLen );
	if( m_bWriteMode ) {
		bool repeat;

		// Data in TRANSLATOR_TYPE_IMAGE format has been added, translate it to icon format

		// Find a BitmapHeader structure:
		if( m_eState == STATE_INIT && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapHeader ) ) {
			m_cInBuffer.Read( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );
			m_eState = STATE_FRAMEHDR;
			m_sIconDir.nIconMagic = ICON_MAGIC;
			m_sIconDir.nNumImages = m_sBitmapHeader.bh_frame_count;
			m_cOutBuffer.Write( &m_sIconDir, sizeof( m_sIconDir ) );
		}

		// Read frames, one by one:
		do {
			repeat = false;

			if( m_eState == STATE_FRAMEHDR && m_cInBuffer.Size() > (ssize_t)sizeof( BitmapFrameHeader ) ) {
				m_cInBuffer.Read( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );
				m_sIconHdr.nBitsPerPixel = BitsPerPixel( m_sCurrentFrame.bf_color_space );
				m_sIconHdr.nWidth = m_sCurrentFrame.bf_frame.right - m_sCurrentFrame.bf_frame.left + 1;
				m_sIconHdr.nHeight = m_sCurrentFrame.bf_frame.bottom - m_sCurrentFrame.bf_frame.top + 1;
				m_cOutBuffer.Write( &m_sIconHdr, sizeof( m_sIconHdr ) );
				m_sIconDir.nNumImages--;
				m_eState = STATE_READING;
			}
			
			while( m_eState == STATE_READING && m_cInBuffer.Size() ) {
				uint8	buffer[ 4096 ];
				uint32	nSize = m_sCurrentFrame.bf_data_size > sizeof( buffer ) ? sizeof( buffer ) : m_sCurrentFrame.bf_data_size;

				nSize = nSize > m_cInBuffer.Size() ? m_cInBuffer.Size() : nSize ;
	
				if( nSize ) {
					m_cInBuffer.Read( buffer, nSize );
					m_cOutBuffer.Write( buffer, nSize );
					m_sCurrentFrame.bf_data_size -= nSize;
				} else if( m_sIconDir.nNumImages ) {
					m_eState = STATE_FRAMEHDR;
					repeat = true;
				}
			}
		} while(repeat);


	} else {
		bool repeat;

		// Data in icon format has been added, translate it to TRANSLATOR_TYPE_IMAGE format

		// Find an IconDir structure:
		if( m_eState == STATE_INIT && m_cInBuffer.Size() > (ssize_t)sizeof( IconDir ) ) {
			m_cInBuffer.Read( &m_sIconDir, sizeof( m_sIconDir ) );
			m_eState = STATE_ICONHDR;
			m_bFirstFrame = true;
		}

		// Read frames, one by one:
		do {
			repeat = false;

			if( m_eState == STATE_ICONHDR && m_cInBuffer.Size() > (ssize_t)sizeof( IconHeader ) ) {
				m_cInBuffer.Read( &m_sIconHdr, sizeof( m_sIconHdr ) );
				if( m_bFirstFrame ) {
					m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
					m_sBitmapHeader.bh_data_size = ( m_sIconHdr.nBitsPerPixel * m_sIconHdr.nWidth * m_sIconHdr.nHeight ) / 8;
					m_sBitmapHeader.bh_flags = 0;
					m_sBitmapHeader.bh_bounds.left = 0;
					m_sBitmapHeader.bh_bounds.top = 0;
					m_sBitmapHeader.bh_bounds.right = m_sIconHdr.nWidth - 1;
					m_sBitmapHeader.bh_bounds.bottom = m_sIconHdr.nHeight - 1;
					m_sBitmapHeader.bh_frame_count = m_sIconDir.nNumImages;
					m_sBitmapHeader.bh_bytes_per_row = ( m_sIconHdr.nBitsPerPixel * m_sIconHdr.nWidth + 7 ) / 8;
					m_sBitmapHeader.bh_color_space = m_sIconHdr.nBitsPerPixel == 32 ? CS_RGBA32 : CS_GRAY8;
					m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );
					m_bFirstFrame = false;
				}
				m_sCurrentFrame.bf_header_size = sizeof( m_sCurrentFrame );
				m_sCurrentFrame.bf_data_size = ( m_sIconHdr.nBitsPerPixel * m_sIconHdr.nWidth * m_sIconHdr.nHeight + 7 ) / 8;
				m_sCurrentFrame.bf_time_stamp = 0;
				m_sCurrentFrame.bf_color_space = m_sIconHdr.nBitsPerPixel == 32 ? CS_RGBA32 : CS_GRAY8;
				m_sCurrentFrame.bf_bytes_per_row = ( m_sIconHdr.nBitsPerPixel * m_sIconHdr.nWidth + 7 ) / 8;
				m_sCurrentFrame.bf_frame.left = 0;
				m_sCurrentFrame.bf_frame.top = 0;
				m_sCurrentFrame.bf_frame.right = m_sIconHdr.nWidth - 1;
				m_sCurrentFrame.bf_frame.bottom = m_sIconHdr.nHeight - 1;
				m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );
	
				m_sBitmapHeader.bh_frame_count--;
				m_eState = STATE_READING;
			}
			
			while( m_eState == STATE_READING && m_cInBuffer.Size() ) {
				uint8	buffer[ 4096 ];
				uint32	nSize = m_sCurrentFrame.bf_data_size > sizeof( buffer ) ? sizeof( buffer ) : m_sCurrentFrame.bf_data_size;

				nSize = nSize > m_cInBuffer.Size() ? m_cInBuffer.Size() : nSize ;
	
				if( nSize ) {
					m_cInBuffer.Read( buffer, nSize );
					m_cOutBuffer.Write( buffer, nSize );
					m_sCurrentFrame.bf_data_size -= nSize;
				} else if( m_sBitmapHeader.bh_frame_count ) {
					m_eState = STATE_ICONHDR;
					repeat = true;
				}
			}
		} while( repeat );
	}
	return 0;
}

ssize_t IconTrans::AvailableDataSize()
{
    return( m_cOutBuffer.Size() );
}

ssize_t IconTrans::Read( void* pData, size_t nLen )
{
    int nCurLen = m_cOutBuffer.Read( pData, nLen );
    if ( nCurLen == 0 /*&& m_bDone == false*/ ) {
	return( ERR_SUSPENDED );
    }
    return( nCurLen );
}

void IconTrans::Abort()
{
}

void IconTrans::Reset()
{
}

class IconReaderNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
	if ( nLen < 4 ) {
	    return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
	}
	if ( *((int *)pData) == ICON_MAGIC ) {
	    return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { "image/atheos-icon", TRANSLATOR_TYPE_IMAGE, 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new IconTrans );
    }
};

class IconWriterNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen ) {
    	if( cSrcType == TRANSLATOR_TYPE_IMAGE && cDstType == "image/atheos-icon" ) {
		return( 0 );
	}
	return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }
    virtual TranslatorInfo GetTranslatorInfo()
    {
	static TranslatorInfo sInfo = { TRANSLATOR_TYPE_IMAGE, "image/atheos-icon", 1.0f };
	return( sInfo );
    }
    virtual Translator*	   CreateTranslator() {
	return( new IconTrans( true ) );
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
	static IconReaderNode sRdNode;
	static IconWriterNode sWrNode;
	switch( nIndex ) {
		case 1:
			return( &sWrNode );
		default:
			return( &sRdNode );
	}
}
    
};
